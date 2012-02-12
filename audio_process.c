/*
 *   audio_process.c
 */

//* Standard Linux headers **
#include     <stdio.h>		// Always include stdio.h
#include     <stdlib.h>		// Always include stdlib.h
#include     <string.h>		// Defines memcpy

//* Application headers **
#include     "debug.h"		// DBG and ERR macros
#include     "audio_process.h"


// Here's where we processing the audio
// Format is left and right interleaved.

int audio_process(short *outputBuffer, short *inputBuffer, int samples) {
    //memcpy((char *)outputBuffer, (char *)inputBuffer, 2*samples);

// Samples are left and right channels interleaved.
	int i;
    	//DBG("samples = %d\n", samples);

	for(i=0; i<samples; i+=2) {
	outputBuffer[i]=inputBuffer[i];
	outputBuffer[i+1]=inputBuffer[i+1];
	}

	//for(i=0;i<samples;i+=502){
	printf("0:%d\n",outputBuffer[0]);
	printf("1:%d\n",outputBuffer[1]);	
	//}	
	fflush(stdout);
    return 0;
}
