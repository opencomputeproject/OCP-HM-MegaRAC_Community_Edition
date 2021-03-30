/*****************************************************************
 *
 * adcapp.c
 * application to acess ADC adriver
 *
 * Author: Rama Rao Bisa <ramab@ami.com>
 *
 * Copyright (C) <2019> <American Megatrends International LLC>
 *
 *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <stdint.h>
#include "EINTR_wrappers.h"
#include "adc.h"
#include "adcifc.h"

typedef enum {
	GET_ADC_VALUE,
	END_OF_FUNCLIST

}e_adc_actions;

e_adc_actions action = END_OF_FUNCLIST;

static void ShowUsuage ( void )
{
	printf ("ADC Test Tool - Copyright (c) 2009-2015 American Megatrends Inc.\n");
	printf( "Usage : addapp NumOfMaxChannels <option> \n" );
	printf( "option: \n" );
	printf( "\t--read-adc-channel 	\tGet ADC value for all the ADC channels\n" );
	printf( "\n" );
}

static int process_arguments( int argc, char **argv )
{
	int i=2;
	if( strcmp( argv[ i ], "--read-adc-channel" ) == 0 )
	{
		action = GET_ADC_VALUE;
	}
	else
	{
		action = END_OF_FUNCLIST;
	}
	return 0;
}

int
main ( int argc , char* argv [] )
{

	get_adc_value_t get_adc_value;
	int max_adc_channels;
	int i,ret_val;
	unsigned short reading=0;
	if ( !(argc >= 3 ) )
	{
		ShowUsuage () ;
		return 0;
	}
	max_adc_channels = (unsigned char)strtol( argv[1], NULL, 10);
	process_arguments( argc , argv);
	if ( (END_OF_FUNCLIST == action))
	{
		ShowUsuage ();
		return 0;
	}

	switch ( action )
	{
		case GET_ADC_VALUE:
			for (i = 0; i < max_adc_channels; i++)
			{	
				get_adc_value.channel_num  = i;
				get_adc_value.channel_value = 0;

				ret_val = get_adc_val(get_adc_value.channel_num, &reading);

				if (ret_val == -1)
				{
					printf ("Read ADC channel failed\n");
					return -1;
				}
				printf ("ADC Channel Value =%5d for channel %d\n", reading, get_adc_value.channel_num);
			}
			break;

		default:
			printf("Invalid ADC Function Call ");
			break;
	}

	return 0;
}
