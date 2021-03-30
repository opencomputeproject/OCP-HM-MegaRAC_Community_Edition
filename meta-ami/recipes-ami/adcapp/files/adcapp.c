#include<stdio.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define No_of_arg 2
#define no_of_adc_chn 16
#define len 10
#define file_path_len 80
#define no_of_bits 10

#define SUCCESS	0
#define FAILURE -1

typedef enum {
	GET_SAMP_FREQ,
	GET_SCALE,
	GET_ADC_VALUE,
	END_OF_FUNCLIST

}e_adc_actions;

e_adc_actions action = END_OF_FUNCLIST;

const char *scale_file = "/sys/devices/platform/ahb/ahb\:apb/1e6e9000.adc/iio\:device0/in_voltage_scale";
const char *frq_file = "/sys/devices/platform/ahb/ahb\:apb/1e6e9000.adc/iio\:device0/in_voltage_sampling_frequency";
char path [ file_path_len ] = { 0 };
char val [ no_of_bits ] = { 0 };
static void show_usage ( void )
{
	printf ( "ADC Test Tool - Copyright (c)  American Megatrends Inc.\n" );
	printf( "Usage : addapp <option> \n" );
	printf( "\t--get-sampling-freq: \tGet ADC sampling frequncy\n" );
	printf( "\t--get-scale:      \tGet ADC scale\n" );
	printf( "\t--read-adc-channel:\tGet ADC value for all the ADC channels\n" );
	printf( "\n" );
}

static void process_arguments ( int argc, char **argv )
{
	int i=1;

	if ( strcmp ( argv[ i ], "--get-sampling-freq" ) == 0 )
	{
		action = GET_SAMP_FREQ;
	}
	else if ( strcmp ( argv[ i ], "--get-scale" ) == 0 )
	{
		action = GET_SCALE;
	}
	else if ( strcmp ( argv[ i ], "--read-adc-channel" ) == 0 )
	{
		action = GET_ADC_VALUE;
	}
	else
	{
		action = END_OF_FUNCLIST;
	}
}

static void get_adc_vtg ( void )
{
	int fd, vtg, i;

	for ( i=0; i < no_of_adc_chn; i++ )
	{	
		snprintf ( path, file_path_len-1 , "/sys/devices/platform/ahb/ahb\:apb/1e6e9000.adc/iio\:device0/in_voltage%d_raw", i );
		fd = open ( path, O_RDWR );
		if ( fd < 0 )
		{
			printf ( "Error reading ADC Channel %d Voltage value \n", i );
			continue ;
		}

		if ( 0 == read ( fd, val, len ) )
		{
			printf ( "Error reading ADC Channel %d Voltage value \n", i );
			continue ;
		}
		vtg = atoi ( val );
		printf ( "ADC Channel Voltage =%dmv for channel %d\n", vtg, i );
		close ( fd );
	}
}


int main ( int argc, char ** argv )
{
	int fd = NULL;

	if ( ! ( argc >= No_of_arg ) )
	{
		show_usage () ;
		goto exit;
	}

	process_arguments ( argc , argv );

	if ( ( END_OF_FUNCLIST == action ) )
	{
		show_usage ();
		goto exit;
	}

	switch ( action )
	{
		case GET_SAMP_FREQ:

			fd = open ( frq_file, O_RDWR );

			if ( fd < 0 )
			{
				printf ( "Error reading ADC sampling frequency\n" );
				goto exit ;
			}

			if ( 0 == read ( fd, val, len ) )
			{
				printf ( "Error reading ADC sampling frequency\n" );
				goto clean ;
			}

			printf ( "ADC sampling_frequency =%s\n",val );

			break;

		case GET_SCALE:

			fd = open ( scale_file, O_RDWR );

			if ( fd < 0 )
			{
				printf ( "Error reading ADC Channel Scale \n" );
				goto exit ;
			}

			if ( 0 == read ( fd, val, len ) )
			{
				printf ( "Error reading ADC Channel Scale \n" );
				goto clean ;
			}

			printf ( "ADC Channel Scale = %s\n", val );

			break;

		case GET_ADC_VALUE:

			get_adc_vtg();

			break;

		default:

			printf ( "Invalid Gpio Function Call " );

			break;

	}

clean :

	if ( fd )
	{
		close ( fd );
	}

exit :

	return SUCCESS;


}
