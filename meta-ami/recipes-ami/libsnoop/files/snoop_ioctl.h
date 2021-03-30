/*****************************************************************
 **                                                             **
 **     (C) Copyright 2009-2015, American Megatrends Inc.       **
 **                                                             **
 **             All Rights Reserved.                            **
 **                                                             **
 **         5555 Oakbrook Pkwy Suite 200, Norcross,             **
 **                                                             **
 **         Georgia - 30093, USA. Phone-(770)-246-8600.         **
 **                                                             **
 ****************************************************************/

#ifndef __SNOOP_IOCTL_H__
#define __SNOOP_IOCTL_H__

#define READ_PREVIOUS_CODES  _IOR('s', 0, int)
#define READ_CURRENT_CODES   _IOR('s', 1, int)
#define ENABLE_SNOOP_IRQ     _IOR('s', 2, int)
#define DISABLE_SNOOP_IRQ    _IOR('s', 3, int)

#endif

