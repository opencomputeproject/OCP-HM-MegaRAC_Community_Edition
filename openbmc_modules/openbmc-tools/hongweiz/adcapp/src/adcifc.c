 /*****************************************************************
 *
 * adcapp.c
 * Simple interface to read and write adc.
 *
 * Author: Rama Rao Bisa <ramab@ami.com>
 *
 * Copyright (C) <2019> <American Megatrends International LLC>
 *
 *****************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "adc.h"
#include "adcifc.h"
#include "EINTR_wrappers.h"

/** \file adcifc.c
  \brief Source for all adc interface code
 */

static int adc_directory_check()
{
	int retval = 0;
	struct stat sb;
	if (!(stat("/sys/bus/iio/devices/iio:device0", &sb) == 0 && S_ISDIR(sb.st_mode)))
	{
		printf("\"/sys/bus/iio/devices/iio:device0\" is not exist!\n");
		retval = -1;
	}
	return retval;
}

static int sys_get_adc_vol( get_adc_value_t *argp )
{
	int retval = -1;
	int fd;
	int tmp;
	char stringArray[50];
	char val[5];
	if(argp->channel_num > 15)	retval = -1;
	retval = adc_directory_check();
	if(retval != 0)
	{
		return retval;
	}
	snprintf(stringArray, sizeof(stringArray), "%s%s%d%s", "/sys/bus/iio/devices/iio:device0","/in_voltage", argp->channel_num,"_raw");
	retval = access(stringArray,F_OK);
	if(retval != 0)
	{
		return retval;
	}
	fd = sigwrap_open(stringArray, O_RDONLY);
	if (fd < 0) {
		return fd;
	}
	read(fd, val, 5);
	tmp = atoi(val);
	(void)sigwrap_close(fd);
	argp->channel_value = (uint16_t)(tmp);
	retval = 0;
	return( retval );
}

/**
 * get_adc_val
 *
 **/
int get_adc_val( int channel , unsigned short *data)
{
	get_adc_value_t adc_arg;

	/* Set the adc channel to read */
	adc_arg.channel_num   = channel;
	adc_arg.channel_value = 0;

	if ( -1 == sys_get_adc_vol(&adc_arg)) { return -1; }
	*data = (unsigned short)(adc_arg.channel_value);

	return ( 0 );
}
