/*****************************************************************
******************************************************************
***                                                            ***
***        (C)Copyright 2018, American Megatrends Inc.         ***
***                                                            ***
***                    All Rights Reserved                     ***
***                                                            ***
***       5555 Oakbrook Parkway, Norcross, GA 30093, USA       ***
***                                                            ***
***                     Phone 770.246.8600                     ***
***                                                            ***
******************************************************************
******************************************************************/
unsigned char  CalculateModule100(unsigned char *Buffer, unsigned long Size);
unsigned char  ValidateModule100(unsigned char *Buffer, unsigned long Size);

unsigned long  CalculateCRC32(unsigned char *Buffer, unsigned long Size);
#if defined (__x86_64__) || defined (WIN64)
void BeginCRC32(unsigned int *crc32);
#else
void BeginCRC32(unsigned long *crc32);
#endif

#if defined (__x86_64__) || defined (WIN64)
void DoCRC32(unsigned int *crc32, unsigned char Data);
#else
void DoCRC32(unsigned long *crc32, unsigned char Data);
#endif

#if defined (__x86_64__) || defined (WIN64)
void EndCRC32(unsigned int *crc32);
#else
void EndCRC32(unsigned long *crc32);
#endif

unsigned char CalculateCRC8( const void *data, int length );

