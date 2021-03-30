/*
 * Simple interface library for fan control operations
 * This file provides interface functions to support pwmtachtool.
 * Copyright (C) <2019>  <American Megatrends International LLC>
 *
 */

#ifndef __PWMTACH_IOCTL_H__
#define __PWMTACH_IOCTL_H__


typedef struct
{
	unsigned char id;
	unsigned int value;
}__attribute__((packed)) pwmtach_property_t;

typedef struct 
{
	char	device_name[16];
	unsigned int dev_id;
	unsigned int num_fans;
	unsigned char fannumber;
	unsigned int rpmvalue;
	unsigned int min_rpm;
	unsigned int max_rpm;
	unsigned char prevdutycycle;
	unsigned char dutycycle;
	unsigned int prescalervalue;
	unsigned int counterresvalue;
	unsigned int tachnumber;
	unsigned int pwmnumber;
	unsigned char status;
	pwmtach_property_t property;
	void* fanproperty_dataptr;
	void* fanmap_dataptr;
}  __attribute__((packed)) pwmtach_data_t;

#define ENABLE_PWM_CHANNEL                     _IOW('P', 0, int)
#define DISABLE_PWM_CHANNEL                    _IOW('P', 1, int)
#define ENABLE_TACH_CHANNEL                    _IOW('P', 2, int)
#define DISABLE_TACH_CHANNEL                   _IOW('P', 3, int)
#define SET_DUTY_CYCLE_BY_PWM_CHANNEL          _IOW('P', 4, int)
#define SET_DUTY_CYCLE_VALUE_BY_PWM_CHANNEL    _IOW('P', 5, int)
#define GET_TACH_VALUE_BY_TACH_CHANNEL         _IOR('P', 6, int)
#define ENABLE_PWM                             _IOW('P', 7, int)
#define ENABLE_ALL_PWM                         _IOW('P', 8, int)
#define ENABLE_TACH                            _IOW('P', 9, int)
#define ENABLE_ALL_TACH                        _IOW('P', 10, int)
#define DISABLE_PWM                            _IOW('P', 11, int)
#define DISABLE_ALL_PWM                        _IOW('P', 12, int)
#define DISABLE_TACH                           _IOW('P', 13, int)
#define DISABLE_ALL_TACH                       _IOW('P', 14, int)
#define GET_TACH_STATUS                        _IOR('P', 15, int)
#define GET_PWM_STATUS                         _IOR('P', 16, int)
#define GET_PWM_CHANNEL_STATUS                 _IOR('P', 17, int)
#define GET_TACH_VALUE                         _IOW('P', 18, int)
#define SET_DUTY_CYCLE                         _IOW('P', 19, int)
#define INIT_PWMTACH                           _IOW('P', 20, int)        
#define CONFIGURE_FANMAP_TABLE                 _IOW('P', 21, int)
#define CONFIGURE_FANPROPERTY_TABLE            _IOW('P', 22, int)
#define SHOW_FANMAP_TABLE                      _IOR('P', 23, int)
#define SHOW_FANPROPERTY_TABLE                 _IOR('P', 24, int)
#define GET_FAN_RPM_RANGE                      _IOR('P', 25, int)
#define GET_DUTY_CYCLE                         _IOR('P', 26, int)
#define SET_TACH_PROPERTY                      _IOW('P', 27, int)
#define GET_TACH_PROPERTY                      _IOR('P', 28, int)
#define SET_PWM_PROPERTY                       _IOW('P', 29, int)
#define GET_PWM_PROPERTY                       _IOR('P', 30, int)
#define CLEAR_TACH_ERROR                       _IOW('P', 31, int)
#define CLEAR_PWM_ERRORS                       _IOW('P', 32, int)
#define END_OF_FUNC_TABLE                      _IOW('P', 33, int)

typedef pwmtach_data_t pwmtach_ioctl_data;

#endif // __PWMTACH_IOCTL_H__
