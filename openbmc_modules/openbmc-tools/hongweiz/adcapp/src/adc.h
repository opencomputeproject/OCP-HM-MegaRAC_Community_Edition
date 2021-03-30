/*****************************************************************
 *
 * adcapp.c
 * application specific driver header.
 *
 * Author: Rama Rao Bisa <ramab@ami.com>
 *
 * Copyright (C) <2019> <American Megatrends International LLC>
 *
 *****************************************************************/

#ifndef __ADC_H__
#define __ADC_H__

#define READ_ADC_CHANNEL            _IOC(_IOC_WRITE,'K',0x100,0x3FFF)
#define READ_ADC_REF_VOLATGE        _IOC(_IOC_WRITE,'K',0x101,0x3FFF)
#define READ_ADC_RESOLUTION         _IOC(_IOC_WRITE,'K',0x102,0x3FFF)
#define ENABLE_EXT_REF_VOLTAGE      _IOC(_IOC_WRITE,'K',0x103,0x3FFF)
#define DISABLE_EXT_REF_VOLTAGE     _IOC(_IOC_WRITE,'K',0x104,0x3FFF)
#define ENABLE_ADC_CHANNEL          _IOC(_IOC_WRITE,'K',0x105,0x3FFF)
#define DISABLE_ADC_CHANNEL         _IOC(_IOC_WRITE,'K',0x106,0x3FFF)

#define PACKED __attribute__ ((packed))

typedef struct
{
	uint8_t 	channel_num;
	uint16_t	channel_value;

} PACKED get_adc_value_t;
#if 0
typedef struct
{
	int (*adc_read_channel) (uint16_t *adc_value, int channel);
	int (*adc_get_resolution) (uint16_t *adc_resolution);
	int (*adc_get_reference_voltage) (uint16_t *adc_ref_volatge);
	int (*adc_reboot_notifier) (void);
}adc_hal_operations_t;

typedef struct
{
} adc_core_funcs_t;


struct adc_hal
{
	adc_hal_operations_t *padc_hal_ops;
};

struct adc_dev
{
	struct adc_hal *padc_hal;
};
#endif

#endif /* !__ADC_H__ */
