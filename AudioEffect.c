#include <gst/gst.h>
#include <glib.h>
#include "debug.h"

GstElement *pipeline, *effect, *effect, *conv1, *conv2, *source, *sink;


void start_pipeline(int effect_num)
{
	GMainLoop *loop;
	/* Initialization */
	gst_init (0, NULL);
	loop = g_main_loop_new (NULL, FALSE);


	/* Create constant gstreamer elements (we wont change these ever)*/
	DBG("[pipe]:\tCreating pipeline elements\n");
	pipeline = gst_pipeline_new ("filter-pipeline");
	source = gst_element_factory_make ("alsasrc", "audio-source");
	conv1 = gst_element_factory_make ("audioconvert", "converter src");
	conv2 = gst_element_factory_make ("audioconvert", "converter sink");
	sink = gst_element_factory_make ("alsasink", "audio-sink");

	if (!pipeline || !source || !conv1 || !conv2 || !sink) {
		g_printerr ("One element could not be created. Exiting.\n");
		return;
	}

	/* Create effects elements */
	DBG("[pipe]:\tSetting up default filter parameters\n");
	switch( effect_num )
	{
		case 0:
			effect = gst_element_factory_make("audioconvert", "pass-thru");
			break;
		case 1:
			effect = gst_element_factory_make ("audiocheblimit", "cheb-LPF");
			g_object_set (G_OBJECT (effect), "mode", 0, NULL);
			g_object_set (G_OBJECT (effect), "cutoff", (float)1000, NULL);
			break;
		case 2:
			effect = gst_element_factory_make ("audiochebband", "cheb-BPF");
			g_object_set (G_OBJECT (effect), "upper-frequency", (float)1050, NULL);
			g_object_set (G_OBJECT (effect), "lower-frequency", (float)950, NULL);
			break;
		case 3: 
			effect = gst_element_factory_make ("audioecho", "audio-echo");
			g_object_set (G_OBJECT (effect), "delay", 500000000, NULL);
			g_object_set (G_OBJECT (effect), "intensity", 0.6, NULL);
			g_object_set (G_OBJECT (effect), "feedback", 0.4, NULL);
			break;
		case 4:
			effect = gst_element_factory_make ("audioinvert" , "audio-invert");
			g_object_set (G_OBJECT (effect), " invert", 0.4, NULL);
			break;
		case 5:
			effect = gst_element_factory_make ("audiokaraoke", "audio-karaoke");
			break;

	}

	/* audio-source -> converter -> cheb filter -> converter -> alsa-output */
	DBG("[pipe]:\tBuilding the pipeline\n");
	gst_bin_add_many (GST_BIN (pipeline),
	source, conv1, effect, conv2, sink, NULL);
	gst_element_link_many (source, conv1, effect, conv2, sink, NULL);

	DBG("[pipe]:\tNow playing\n");
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	g_main_loop_run(loop);
}

void stop_pipeline(){
	DBG("[pipe]:\tStopping playback\n");
	gst_element_set_state (pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (pipeline));
}

void restart_pipeline(int effect_num){
	stop_pipeline();
	start_pipeline(effect_num);
}


void configure_LPF(float cutoff){
	DBG("[pipe]:\tSetting LP cutoff to %f\n", cutoff);
	g_object_set (G_OBJECT (effect), "cutoff", cutoff, NULL);
}

void configure_BPF(float center, float bandwidth){
	float upper, lower;

	upper = center+bandwidth/2;
	if( upper < 0 )
		upper = 0;
	else if( upper > 100000 )
		upper = 100000;

	lower = center-bandwidth/2;
	if( lower < 0 )
		lower = 0; 
	else if( lower > 100000 )
		 lower = 100000;

	DBG("[pipe]:\tSetting BP upper to %f, lower to %f\n", upper, lower);
	g_object_set (G_OBJECT (effect), "upper-frequency", upper, NULL);
	g_object_set (G_OBJECT (effect), "lower-frequency", lower, NULL);
}

void configure_ECHO(float delay, float intensity, float feedback){
	DBG("[pipe]:\tSetting audioecho delay to %f us, intensity of echo to %f, and intensity of feedback to %f\n", delay, intensity, feedback);
	g_object_set (G_OBJECT (effect), "delay", delay, NULL);
	g_object_set (G_OBJECT (effect), "intensity", intensity, NULL);
	g_object_set (G_OBJECT (effect), "feedback", feedback, NULL);
}

void configure_invert(float invert){
	g_object_set (G_OBJECT (effect), " invert", invert, NULL);

}

int main (int argc, char *argv[]){
        int choice;	
 	g_print ("usage: please input <choice> (choice from 0 to 4)\n");
	g_print ("0--original\n");
	g_print ("1--lowpass\n");
	g_print ("2--bandpass\n");
	g_print ("3--audioecho-reverb\n");
	g_print ("4--audioinvert\n");
		
	scanf("%d", &choice);
        start_pipeline(choice);
	
	return 0;
}
