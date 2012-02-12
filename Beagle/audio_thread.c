/*
 *   audio_thread.c
 */

// Modfied for ALSA output 5-May-2011, Mark A. Yoder

// Based on Basic PCM audio (http://www.suse.de/~mana/alsa090_howto.html#sect02)http://www.suse.de/~mana/alsa090_howto.html#sect03

//* Standard Linux headers **
#include     <stdio.h>                          // Always include stdio.h
#include     <stdlib.h>                         // Always include stdlib.h
#include     <fcntl.h>                          // Defines open, read, write methods
#include     <unistd.h>                         // Defines close and sleep methods
//#include     <sys/ioctl.h>                      // Defines driver ioctl method
//#include     <linux/soundcard.h>                // Defines OSS driver functions
#include     <string.h>                         // Defines memcpy
#include     <alsa/asoundlib.h>			// ALSA includes

//* Application headers **
#include     "debug.h"                          // DBG and ERR macros
#include     "audio_thread.h"                   // Audio thread definitions
#include     "audio_input_output.h"             // Audio driver input and output functions
#include     "audio_process.h"


// ALSA device
#define     OUT_DEVICE     "plughw:0,0"
#define     IN_DEVICE	   "plughw:1,0"

//* The sample rate of the audio codec **
#define     SAMPLE_RATE      48000

//* The gain (0-100) of the left channel **
#define     LEFT_GAIN        100

//* The gain (0-100) of the right channel **
#define     RIGHT_GAIN       100

//*  Parameters for audio thread execution **
#define     BLOCKSIZE        48000

//* Parameters for channels **
#define	    INCHANNELS   2
#define     OUTCHANNELS  2

//*******************************************************************************
//*  audio_thread_fxn                                                          **
//*******************************************************************************
//*  Input Parameters:                                                         **
//*      void *envByRef    --  a pointer to an audio_thread_env structure      **
//*                            as defined in audio_thread.h                    **
//*                                                                            **
//*          envByRef.quit -- when quit != 0, thread will cleanup and exit     **
//*                                                                            **
//*  Return Value:                                                             **
//*      void *            --  AUDIO_THREAD_SUCCESS or AUDIO_THREAD_FAILURE as **
//*                            defined in audio_thread.h                       **
//*******************************************************************************
void *audio_thread_fxn( void *envByRef ){

// Variables and definitions
// *************************
	// Thread parameters and return value
	audio_thread_env * envPtr = envByRef;                  // < see above >
	void             * status = AUDIO_THREAD_SUCCESS;      // < see above >

	// The levels of initialization for initMask
	#define     INPUT_ALSA_INITIALIZED      0x1
	#define     OUTPUT_ALSA_INITIALIZED     0x2
	#define     INPUT_BUFFER_ALLOCATED      0x4
	#define     OUTPUT_BUFFER_ALLOCATED     0x8

	unsigned  int   initMask =  0x0;	// Used to only cleanup items that were init'd

	// Input and output driver variables

	snd_pcm_t  *pcm_output_handle,*pcm_capture_handle;	// Handles for the PCM devices
	snd_pcm_uframes_t exact_bufsize;	// bufsize is in frames.  Each frame is 4 bytes

	int   blksize = BLOCKSIZE;	// Raw input or output frame size in bytes
	char *inputBuffer = NULL;
	char *outputBuffer = NULL;	// Output buffer for driver to read from

// Thread Create Phase -- secure and initialize resources
// ******************************************************

    // Initialize audio input device
    // ************************
    // Open an ALSA device channel for audio input
	exact_bufsize = blksize/BYTESPERFRAME;

	if( audio_io_setup( &pcm_capture_handle, IN_DEVICE, SAMPLE_RATE,
	SND_PCM_STREAM_CAPTURE, &exact_bufsize, INCHANNELS ) == AUDIO_FAILURE )
	{
		ERR( "Audio_input_setup failed in audio_thread_fxn\n\n" );
		status = AUDIO_THREAD_FAILURE;
		goto cleanup;
	}
	//DBG( "exact_bufsize = %d\n", (int) exact_bufsize);

	// Record that input OSS device was opened in initialization bitmask
	initMask |= INPUT_ALSA_INITIALIZED;

    // Initialize audio output device
    // ******************************
    // Initialize the output ALSA device
	//DBG( "pcm_output_handle before audio_output_setup = %d\n", (int) pcm_output_handle);
	exact_bufsize = blksize/BYTESPERFRAME;
	//DBG( "Requesting bufsize = %d\n", (int) exact_bufsize);
	if( audio_io_setup( &pcm_output_handle, OUT_DEVICE, SAMPLE_RATE, 
	SND_PCM_STREAM_PLAYBACK, &exact_bufsize, OUTCHANNELS) == AUDIO_FAILURE )
	{
		ERR( "audio_output_setup failed in audio_thread_fxn\n" );
		status = AUDIO_THREAD_FAILURE;
		goto  cleanup ;
	}
	//DBG( "pcm_output_handle after audio_output_setup = %d\n", (int) pcm_output_handle);
	//DBG( "blksize = %d, exact_bufsize = %d\n", blksize, (int) exact_bufsize);
	blksize = exact_bufsize;

    	// Record that input ALSA device was opened in initialization bitmask
    	initMask |= OUTPUT_ALSA_INITIALIZED;

	//*******ALLOCATE BUFFERS*********

	blksize = exact_bufsize*BYTESPERFRAME;
    	// Create input buffer to read into from OSS input device
    	if( ( inputBuffer = malloc( blksize) ) == NULL )
    	{
        	ERR( "Failed to allocate memory for input block (%d)\n", blksize );
        	status = AUDIO_THREAD_FAILURE;
        	goto  cleanup ;
    	}

    	//DBG( "Allocated input audio buffer of size %d to address %p\n", blksize, inputBuffer );
	// Record that the input buffer was allocated in initialization bitmask
    	initMask |= INPUT_BUFFER_ALLOCATED;

    	// Create output buffer to write from into ALSA output device
	if( ( outputBuffer = malloc( blksize ) ) == NULL )
	{
		ERR( "Failed to allocate memory for output block (%d)\n", blksize );
		status = AUDIO_THREAD_FAILURE;
		goto  cleanup ;
	}

	//DBG( "Allocated output audio buffer of size %d to address %p\n", blksize, outputBuffer );
  	// Record that the output buffer was allocated in initialization bitmask
    	initMask |= OUTPUT_BUFFER_ALLOCATED;

// Thread Execute Phase -- perform I/O and processing
// **************************************************
    // Get things started by sending some silent buffers out.

    	memset(outputBuffer, 0, blksize);		// Clear the buffer
	int i;
    	for(i=0; i<2; i++) {
	(snd_pcm_readi(pcm_capture_handle, inputBuffer,exact_bufsize));   
	}
//
// Processing loop
    	//DBG( "Entering audio_thread_fxn processing loop\n");
    	while( !envPtr->quit ){
	// Read buffer from audio file
		while( snd_pcm_readi(pcm_capture_handle, inputBuffer, exact_bufsize) < 0 ) {
			snd_pcm_prepare(pcm_capture_handle);
	    		ERR( "<<<<<<<<<<<<<<< Buffer Overrun >>>>>>>>>>>>>>>\n");
            		ERR( "Error reading the data from file descriptor %d\n", (int) pcm_capture_handle );
			goto cleanup;
        	}

		audio_process((short*)outputBuffer,(short*)inputBuffer, blksize/2);

	// Write output buffer into ALSA output device
		while (snd_pcm_writei(pcm_output_handle, outputBuffer, exact_bufsize) < 0) {
        		snd_pcm_prepare(pcm_output_handle);
        		//ERR( "<<<<<<<<<<<<<<< Buffer Underrun >>>>>>>>>>>>>>>\n");
        		status = AUDIO_THREAD_FAILURE;
        		//goto cleanup;
		}
}
DBG( "Exited audio_thread_fxn processing loop\n" );

// Thread Delete Phase -- free up resources allocated by this file
// ***************************************************************

cleanup:
	DBG( "Starting audio thread cleanup to return resources to system\n" );

// Close the audio drivers
// ***********************
//  - Uses the initMask to only free resources that were allocated.
//  - Nothing to be done for mixer device, as it was closed after init.

	// Close input OSS device
	if( initMask & INPUT_ALSA_INITIALIZED )
		if( audio_io_cleanup( pcm_capture_handle ) != AUDIO_SUCCESS )
		{
			ERR( "audio_input_cleanup() failed for file descriptor %d\n", (int) pcm_capture_handle );
			status = AUDIO_THREAD_FAILURE;
		}   

    	// Close output ALSA device
    	if( initMask & OUTPUT_ALSA_INITIALIZED )
        	if( audio_io_cleanup( pcm_output_handle ) != AUDIO_SUCCESS )
        	{
            		ERR( "audio_output_cleanup() failed for file descriptor %d\n", (int)pcm_output_handle );
            		status = AUDIO_THREAD_FAILURE;
        	}

// Free allocated buffers
// **********************
    	// Free input buffer
    	if( initMask & INPUT_BUFFER_ALLOCATED )
    	{
        	DBG( "Freeing audio input buffer at location %p\n", inputBuffer );
        	free( inputBuffer );
        	DBG( "Freed audio input buffer at location %p\n", inputBuffer );
    	}
    	// Free output buffer
    	if( initMask & OUTPUT_BUFFER_ALLOCATED )
    	{
		DBG( "Freeing audio output buffer at location %p\n", outputBuffer );
        	free( outputBuffer );
        	DBG( "Freed audio output buffer at location %p\n", outputBuffer );
    	}

// Return from audio_thread_fxn function
// *************************************
	
    	// Return the status at exit of the thread's execution
    	DBG( "Audio thread cleanup complete. Exiting audio_thread_fxn\n" );
    	return status;
}

