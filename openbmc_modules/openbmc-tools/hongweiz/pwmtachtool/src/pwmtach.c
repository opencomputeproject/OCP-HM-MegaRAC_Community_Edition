
/*
 * Simple interface library for fan control operations
 * This file provides interface functions to support pwmtachtool.
 * Copyright (C) <2019>  <American Megatrends International LLC>
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "libpwmtach.h"
#include "pwmtach_ioctl.h"
#include "EINTR_wrappers.h"
#include <stdlib.h>

void select_sleep(time_t sec,suseconds_t usec)
{
	struct timeval tv;

	tv.tv_sec = sec;
	tv.tv_usec = usec;

	while(sigwrap_select(0, NULL, NULL, NULL, &tv) < 0);
}

//support acessing driver using sysfs device file 
static char DevNodeFileName[50];
#define HWMON_DIR "/sys/class/hwmon"

//build the pwm and tach access device node file name, and mapping pwm/tach number starting from 1.
#define BUILD_PWM_NODE_NAME(buffer,DEV_ID,PWM_NUM)	    snprintf(buffer, sizeof(buffer), "%s%d%s%d", HWMON_DIR "/hwmon",DEV_ID, "/pwm", PWM_NUM+1)
#define BUILD_TACH_NODE_NAME(buffer,DEV_ID,TACH_NUM)	snprintf(buffer, sizeof(buffer), "%s%d%s%d%s", HWMON_DIR "/hwmon",DEV_ID, "/fan", TACH_NUM+1,"_input")
#define BUILD_FAN_REG_NAME(buffer,DEV_ID,FAN_NUM)	snprintf(buffer, sizeof(buffer), "%s%d%s%d%s", HWMON_DIR "/hwmon",DEV_ID, "/of_node/fan@", FAN_NUM,"/reg")

//predefine FAN RPM range, must defined at some where for configuration.
#define RPM_MAX         38600
#define RPM_MIN         7500
#define COUNTERRES_DEF  100

/* Check hwmon if exist or not */
static int pwmtach_directory_check(void)
{
	int retval = 0;
	struct stat sb;
	if (!(stat("/sys/class/hwmon", &sb) == 0 && S_ISDIR(sb.st_mode)))
	{
		printf("\"/sys/class/hwmon\" not exist!\n");
		retval = -1;
	}
	return retval;
}
//Notice: dutycycle_value is one byte (0-255)
static int SET_PWM_DUTYCYCLE_VALUE ( pwmtach_ioctl_data  *ppwmtach_arg )
{
	int retval = 0;
	unsigned char dutycycle_value;
	int fd;
	char duty_num[5];

	retval = pwmtach_directory_check();
	if(retval != 0)
	{
		return retval;
	}

	dutycycle_value = ppwmtach_arg->dutycycle;
	BUILD_PWM_NODE_NAME(DevNodeFileName,ppwmtach_arg->dev_id,ppwmtach_arg->pwmnumber);
	retval = access(DevNodeFileName,F_OK);
	if(retval != 0)
	{
		return retval;
	}

	fd = sigwrap_open(DevNodeFileName, O_WRONLY);
	if (fd < 0) {
		return fd;
	}

	snprintf(duty_num,5, "%d", dutycycle_value);

	if ( write(fd, duty_num, strlen (duty_num)) != (ssize_t )strlen(duty_num)){
		printf("%s: Error write dutycycle value %d to pwm %d\n",__FUNCTION__,dutycycle_value,ppwmtach_arg->pwmnumber);
		retval = -1;
	}
	(void)sigwrap_close(fd);

	return retval;
}

//Notice: dutycycle_percentage value should be between 1 to 99.
static int SET_PWM_DUTYCYCLE ( pwmtach_ioctl_data  *ppwmtach_arg)
{
	int retval = 0;
	unsigned char dutycycle_value;

	if(ppwmtach_arg->dutycycle > 100)
	{
		return -1;
	}

	dutycycle_value = (ppwmtach_arg->dutycycle*255)/100;
	ppwmtach_arg->dutycycle = dutycycle_value;
	retval = SET_PWM_DUTYCYCLE_VALUE(ppwmtach_arg);
	return retval;
}

static int GET_PWM_DUTYCYCLE ( pwmtach_ioctl_data  *ppwmtach_arg )
{
	int retval = 0;
	int fd;
	char duty_num[5];

	retval = pwmtach_directory_check();
	if(retval != 0)
	{//printf("%s,error 0\n",__FUNCTION__); 
		return retval;
	}

	BUILD_PWM_NODE_NAME(DevNodeFileName,ppwmtach_arg->dev_id,ppwmtach_arg->pwmnumber);
	retval = access(DevNodeFileName,F_OK);
	if(retval != 0)
	{printf("%s,error 2,%s not exist\n",__FUNCTION__,DevNodeFileName); 
		return retval;
	}
	fd = sigwrap_open(DevNodeFileName, O_RDONLY);
	if (fd < 0) {printf("%s,error 3\n",__FUNCTION__); 
		return fd;
	}
	read(fd, duty_num, 5);
	ppwmtach_arg->dutycycle = atoi(duty_num);
	printf("%s:dutycycle value %d to pwm %d\n",__FUNCTION__,ppwmtach_arg->dutycycle,ppwmtach_arg->pwmnumber);
	(void)sigwrap_close(fd);

	return retval;
}
int GET_TACH_SPEED (pwmtach_ioctl_data *ppwmtach_arg )
{
	int retval = 0;
	int fd;
	char data[6];

	retval = pwmtach_directory_check();
	if(retval != 0)
	{printf("%s,error 0\n",__FUNCTION__); 
		return retval;
	}
	BUILD_TACH_NODE_NAME(DevNodeFileName,ppwmtach_arg->dev_id,ppwmtach_arg->tachnumber);
	retval = access(DevNodeFileName,F_OK);
	if(retval != 0)
	{printf("%s,error 2,%s not exist\n",__FUNCTION__,DevNodeFileName); 
		return retval;
	}

	fd = sigwrap_open(DevNodeFileName, O_RDONLY);
	if (fd < 0) {printf("%s,error 3\n",__FUNCTION__); 
		return fd;
	}
	memset(data, 0, 6);
	read(fd, data, 6);
	ppwmtach_arg->rpmvalue = atoi(data);
	(void)sigwrap_close(fd);
	printf("%s:rpm value %d\n",__FUNCTION__,ppwmtach_arg->rpmvalue);
	return retval;
}
//mapping function of fan to tach
//using direct mapping as default
#define GET_TACH_NUMBER(FAN_NUMBER)     FAN_NUMBER
//mapping fan number to pwm number
//using information in fan@number reg item, to look up the pwm index
static int GET_PWM_NUMBER(pwmtach_ioctl_data *ppwmtach_arg)
{
	int retval = 0;
	int fd;
	int reg_val = 0;

	retval = pwmtach_directory_check();
	if(retval != 0)
	{printf("%s,error 0\n",__FUNCTION__); 
		return retval;
	}
	BUILD_FAN_REG_NAME(DevNodeFileName,ppwmtach_arg->dev_id,ppwmtach_arg->fannumber);
	retval = access(DevNodeFileName,F_OK);
	if(retval != 0)
	{printf("%s,error 2,%s not exist\n",__FUNCTION__,DevNodeFileName); 
		return retval;
	}

	fd = sigwrap_open(DevNodeFileName, O_RDONLY);
	if (fd < 0) {printf("%s,error 3\n",__FUNCTION__); 
		return fd;
	}
	read(fd, &reg_val, sizeof(int));
	if(reg_val < 0)
	{
		retval = -1;
	}else
	{
		retval = reg_val >> 24; //get the highest byte
		printf("%s:fan %d, pwm %d, val 0x%X\n",__FUNCTION__,ppwmtach_arg->fannumber,retval,reg_val);
		printf("%s\n",DevNodeFileName);
	}
	(void)sigwrap_close(fd);
	// printf("%s:rpm value %d\n",__FUNCTION__,ppwmtach_arg->rpmvalue);
	return retval;
}
static int pwmtach_action(  pwmtach_ioctl_data* argp, int command )
{
	int retval = 0;
	// printf("%s, Command 0x%X:Dev:%d,Pwm:0x%X,Fan:0x%x,Tach:0x%X\n",__FUNCTION__,command,argp->dev_id,argp->pwmnumber,argp->fannumber,argp->tachnumber);
	switch(command)
	{
		case SET_DUTY_CYCLE_BY_PWM_CHANNEL:
			retval = SET_PWM_DUTYCYCLE(argp);
			break;
		case SET_DUTY_CYCLE_VALUE_BY_PWM_CHANNEL:
		case SET_DUTY_CYCLE:
			retval = SET_PWM_DUTYCYCLE_VALUE(argp);
			break;
		case GET_TACH_VALUE_BY_TACH_CHANNEL:
			retval = GET_TACH_SPEED(argp);
			break;
		case GET_TACH_VALUE: //used to get fan speed
			argp->tachnumber = GET_TACH_NUMBER(argp->fannumber);
			retval = GET_TACH_SPEED(argp);
			break;
		case GET_DUTY_CYCLE:
			retval = GET_PWM_DUTYCYCLE(argp);
			break;
		case GET_FAN_RPM_RANGE:
			argp->max_rpm = RPM_MAX;
			argp->min_rpm = RPM_MIN;
			break;
		case INIT_PWMTACH: //assume that init complete
			argp->pwmnumber         = GET_PWM_NUMBER(argp);; //since we don't have the fan to pwm mapping, just using direct map for workarround.
			argp->counterresvalue   = COUNTERRES_DEF; //since driver don't support COUNTERRES, just using default value for workarround.
			retval = GET_PWM_DUTYCYCLE(argp);
			break;
		case END_OF_FUNC_TABLE:
		default:
			printf("%s, Command 0x%X not support!\n",__FUNCTION__,command);
			retval = -1;
	}
	return( retval );
}

int get_tach_speed ( unsigned int dev_id, unsigned char tach_number, unsigned int *rpm_value )
{
	pwmtach_ioctl_data pwmtach_arg;
	int retval = 0;

	pwmtach_arg.dev_id = dev_id;
	pwmtach_arg.tachnumber = tach_number;
	retval = pwmtach_action( &pwmtach_arg, GET_TACH_VALUE_BY_TACH_CHANNEL);
	if(retval != -1)
		*rpm_value = pwmtach_arg.rpmvalue;
	return retval;
}

//Notice: dutycycle_percentage value should be between 1 to 99.
int set_pwm_dutycycle ( unsigned int dev_id, unsigned char pwm_number, unsigned char dutycycle_percentage )
{
	pwmtach_ioctl_data pwmtach_arg;
	int retval = 0;

	pwmtach_arg.dev_id = dev_id;
	pwmtach_arg.pwmnumber = pwm_number;
	pwmtach_arg.dutycycle= dutycycle_percentage;
	retval = pwmtach_action( &pwmtach_arg, SET_DUTY_CYCLE_BY_PWM_CHANNEL);

	return retval;
}

//Notice: dutycycle_value is one byte (0-255)
int set_pwm_dutycycle_value ( unsigned int dev_id, unsigned char pwm_number, unsigned char dutycycle_value )
{
	pwmtach_ioctl_data pwmtach_arg;
	int retval = 0;

	pwmtach_arg.dev_id = dev_id;
	pwmtach_arg.pwmnumber = pwm_number;
	pwmtach_arg.dutycycle= dutycycle_value;
	retval = pwmtach_action( &pwmtach_arg, SET_DUTY_CYCLE_VALUE_BY_PWM_CHANNEL);

	return retval;
}

int get_pwm_dutycycle ( unsigned int dev_id, unsigned char pwm_number, unsigned char *dutycycle_percentage )
{
	pwmtach_ioctl_data pwmtach_arg;
	int retval = 0;

	pwmtach_arg.dev_id = dev_id;
	pwmtach_arg.pwmnumber = pwm_number;
	retval = pwmtach_action(&pwmtach_arg, GET_DUTY_CYCLE);
	if(retval != -1)
		*dutycycle_percentage = pwmtach_arg.dutycycle;
	return retval;

}

int set_fan_speed ( unsigned int dev_id, unsigned char fan_number, unsigned int rpm_value )
{
	int retval = 0;
	unsigned int retries = 20;
	unsigned char firsttime = 1;
	unsigned char duty_cycle_increasing = 0;
	unsigned char reached90percent = 0;
	unsigned char reached5percent = 0;
	unsigned long desiredrpm = rpm_value;
	pwmtach_ioctl_data          pwmtach_arg;
	pwmtach_data_t* indata = (pwmtach_data_t*) &pwmtach_arg;

	indata->dev_id = dev_id;
	indata->fannumber = fan_number;
	indata->rpmvalue = rpm_value;
	indata->counterresvalue = 0;
	indata->dutycycle = 0;
	indata->prevdutycycle = 0;

	retval = pwmtach_action( indata, GET_FAN_RPM_RANGE);
	if ((rpm_value < indata->min_rpm) || (rpm_value > indata->max_rpm))
	{
		printf("Out of range Fan Speed value for fan.\n");
		return -1;
	}
	retval = pwmtach_action ( indata, INIT_PWMTACH );

	while (retries--)
	{
		/* Wait for 1 seconds */
		select_sleep(0,1*1000*1000);
		if ((retval = pwmtach_action( indata, GET_TACH_VALUE )) != 0)
		{
			indata->dutycycle = indata->prevdutycycle;
			retval = pwmtach_action( indata, SET_DUTY_CYCLE);
			return -1;
		}
		else
		{
			indata->prevdutycycle = indata->dutycycle;
			if (indata->rpmvalue > (desiredrpm + 50))
			{
				if (indata->dutycycle <= ((indata->counterresvalue*10)/100))
				{
					if (reached5percent == 1)
					{
						printf("\nSpeed is set to minimum possible speed of %d RPM.\n",indata->rpmvalue);
						break;
					}
					reached5percent = 1;
				}
				else
				{
					indata->dutycycle -= ((5 * indata->counterresvalue)/100);
				}
			}
			else if (indata->rpmvalue < (desiredrpm - 50))
			{
				if (indata->dutycycle >= indata->counterresvalue)
				{
					if (reached90percent == 1)
					{
						printf("\nSpeed is set to maximum possible speed of %d RPM.\n",indata->rpmvalue);
						break;
					}
					reached90percent = 1;
				}
				else
				{
					indata->dutycycle += ((5 * indata->counterresvalue)/100);
				}
			}
			else
			{
				break;
			}

			retval = pwmtach_action (indata, SET_DUTY_CYCLE);
			printf("After update: dutycycle=%d, rpmvalue=%d\n", indata->dutycycle, indata->rpmvalue);

			if(indata->prevdutycycle < indata->dutycycle)
			{       /* Duty Cycle increasing */
				if ((firsttime == 0) && (duty_cycle_increasing == 0))
				{
					indata->dutycycle = indata->prevdutycycle;
					retval = pwmtach_action( indata, SET_DUTY_CYCLE);
					printf("\n");
					return 0;
				}
				duty_cycle_increasing = 1;
			}
			else
			{       /* Duty Cycle decreasing */
				if ((firsttime == 0) && (duty_cycle_increasing == 1))
				{
					indata->dutycycle = indata->prevdutycycle;
					retval = pwmtach_action( indata, SET_DUTY_CYCLE);
					printf("\n");
					return 0;
				}
				duty_cycle_increasing = 0;
			}
			if (firsttime == 1)
				firsttime = 0;
		}
		printf("retry %d : dt=%d, ps=%d, cr=%d\n", retries, indata->dutycycle, indata->prescalervalue, indata->counterresvalue);
	}
	return 0;

}

int get_fan_speed ( unsigned int dev_id, unsigned char fan_number, unsigned int *rpm_value )
{
	pwmtach_ioctl_data pwmtach_arg;
	int retval = 0;

	pwmtach_arg.dev_id = dev_id;
	pwmtach_arg.fannumber = fan_number;
	retval = pwmtach_action( &pwmtach_arg, GET_TACH_VALUE );
	if(retval != -1)
		*rpm_value = pwmtach_arg.rpmvalue;
	return retval;
}
