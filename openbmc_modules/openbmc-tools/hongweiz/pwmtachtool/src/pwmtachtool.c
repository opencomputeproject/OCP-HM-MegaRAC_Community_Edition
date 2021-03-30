
/*
* Pwmtachtool Application
* This application provides functions to get/set fan speed / PWM dutycycle.
* Copyright (C) <2019>  <American Megatrends International LLC>
*
*/

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
#include <limits.h>
#include "libpwmtach.h"

#define VERSION_STR "1.0"
typedef enum {
	SET_FAN_SPEED,
	GET_FAN_SPEED,
	SET_PWM_DUTYCYCLE,
	SET_PWM_DUTYCYCLE_VALUE,
	GET_PWM_DUTYCYCLE,
	END_OF_FUNCLIST
}ePwmTachactions;


ePwmTachactions action = END_OF_FUNCLIST;

static int verbose = 0;

static void ShowUsage ( void )
	/*@globals fileSystem@*/
	/*@modifies fileSystem@*/
{
	printf ("PWMTACH Test Tool (Version %s)\n",VERSION_STR);
	printf ("Copyright (c) 2009-2015 American Megatrends Inc.\n");	
	printf( "Usage : pwmtachtool <device_id> <command-option> <fannum>\n" );
	printf( "\t--set-fan-speed:         Set Fan's speed. Takes the RPM value as the last argument\n" );
	printf("\t\tparameters: <Fan_Number> <Fan_Speed>\n");
	printf( "\t--set-pwm-dutycycle:         Set Fan's dutycycle. dutycycle_percentage value should be between 1 to 100\n" );
	printf( "\t--set-pwm-dutycycle-value:   Set Fan's dutycycle. dutycycle_value should be between 0 to 255\n" );
	printf("\t\tparameters: <pwm_number> <dutycycle value>\n");
	printf( "\t--get-pwm-dutycycle:		Get Fan's dutycycle\n");
	printf( "\t--get-fan-speed:         Get Fan's speed\n" );
	printf( "\t--verbose:         Enable Debug messages\n" );
	printf( "\n" );
}

static void Verbose ( char * msg )
{
	if (verbose ) printf ( "%s\n" , msg );
}

static int process_arguments( int argc, char **argv,
		unsigned char* fan_num,unsigned int* rpm_value, 
		unsigned int* dev_id )
{
	int i = 1;

	if (argc < 3)
	{
		printf("need Device Name and Command to process request\n");
		return -1;
	}

	*dev_id = (unsigned char)strtol( argv[ i++ ], NULL, 10);

	if( strcmp( argv[ i ], "--set-fan-speed" ) == 0 )
	{
		if (argc < 5)
		{
			printf("need Fan Number and RPM value to process request\n");
			return -1;
		}
		*fan_num = (unsigned char)strtol( argv[ ++i ], NULL, 10);
		*rpm_value = (unsigned int)strtol( argv[ ++i ], NULL, 10);
		action = SET_FAN_SPEED;
	}
	else if( strcmp( argv[ i ], "--set-pwm-dutycycle" ) == 0 )
	{
		if (argc < 5)
		{
			printf("need Fan Number and Dutycycle value to process request\n");
			return -1;
		}
		*fan_num = (unsigned char)strtol( argv[ ++i ], NULL, 10);
		*rpm_value = (unsigned int)strtol( argv[ ++i ], NULL, 10);
		action = SET_PWM_DUTYCYCLE;
	}
	else if( strcmp( argv[ i ], "--set-pwm-dutycycle-value" ) == 0 )
	{
		if (argc < 5)
		{
			printf("need Fan Number and Dutycycle value to process request\n");
			return -1;
		}
		*fan_num = (unsigned char)strtol( argv[ ++i ], NULL, 10);
		*rpm_value = (unsigned int)strtol( argv[ ++i ], NULL, 10);
		action = SET_PWM_DUTYCYCLE_VALUE;
	}
	else if( strcmp( argv[i], "--get-pwm-dutycycle" ) == 0)
	{
		if (argc < 4)
		{
			printf("need PWM Number to process request\n");
			return -1;
		}
		*fan_num = (unsigned char)strtol( argv[ ++i ], NULL, 10);
		action = GET_PWM_DUTYCYCLE;
	}

	else if( strcmp( argv[ i ], "--get-fan-speed" ) == 0 )
	{
		if (argc < 4)
		{
			printf("need more parameters to process request\n");
			return -1;
		}
		*fan_num = (unsigned char)strtol( argv[ ++i ], NULL, 10);
		action = GET_FAN_SPEED;
	}

	else if( strcmp( argv[ i ], "--verbose" ) == 0 )
		verbose = 1;

	return 0;
}

int main ( int argc , char* argv [] )
{
	unsigned char fannum = 0, property_id = 0;
	unsigned int rpmvalue = 0;
	unsigned char dutycycle = 0;
	int Value = 0;
	int ret = 0;
	unsigned int dev_id = 0;

	if (argc < 2)
	{
		ShowUsage();
		return 0;
	}
	ret = process_arguments( argc , argv , &fannum, &rpmvalue, &dev_id );
	if (ret != 0)
	{ 
		return -1;
	}

	if (END_OF_FUNCLIST == action)
	{
		ShowUsage ();
		return 0;
	}

	switch ( action )
	{

		case SET_FAN_SPEED:
			Verbose   ("Inside Set Fan Speed \n");
			Value = set_fan_speed (dev_id, fannum, rpmvalue);
			if  ( -1 == Value )
			{
				printf ( "Set Fan Speed Failed \n"); 
				return -1;
			}
			printf ( "Fan Speed set Successfully\n");
			break;	
		case GET_FAN_SPEED:
			Verbose   ("Inside Get Fan Speed \n");
			Value = get_fan_speed (dev_id, fannum, &rpmvalue);
			if ( -1 == Value)
			{
				printf ( "Get Fan Speed Failed \n"); 
				return -1;
			}	
			printf("Fan %d speed is %d \n", fannum, rpmvalue);
			break;

		case SET_PWM_DUTYCYCLE:
			Verbose   ("Inside Set PWM Dutycycle \n");
			Value = set_pwm_dutycycle (dev_id, fannum, rpmvalue);
			if  ( -1 == Value )
			{
				printf ( "Set PWM Dutycycle Failed \n"); 
				return -1;
			}
			printf ( "Fan PWM set dutycycle Successfully\n");
			break;
		case SET_PWM_DUTYCYCLE_VALUE:
			Verbose   ("Inside Set PWM Dutycycle Value\n");
			Value = set_pwm_dutycycle_value (dev_id, fannum, rpmvalue);
			if  ( -1 == Value )
			{
				printf ( "Set PWM Dutycycle Value Failed \n"); 
				return -1;
			}
			printf ( "Fan PWM set dutycycle value Successfully\n");
			break;
		case GET_PWM_DUTYCYCLE:
			Verbose   ("Inside Get PWM Dutycycle \n");
			Value = get_pwm_dutycycle (dev_id, fannum, &dutycycle);
			if  ( -1 == Value )
			{
				printf ( "Set PWM Dutycycle Failed \n"); 
				return -1;
			}
			printf ( "PWM %d Dutycycle is %d\n",fannum, dutycycle);
			break;

		default:
			printf("Invalid PWMTACH Function Call\n");
			break;
	}
	return 0;
}
