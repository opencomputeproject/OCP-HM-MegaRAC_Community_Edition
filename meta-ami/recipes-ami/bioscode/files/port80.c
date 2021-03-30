#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <snoopifc.h>

#define MAX_SIZE 2048

static void ShowUsage ( void )
{
   printf("Bios Code Test Tool\n");
   printf("Usage : bioscode <snoop_channel> \n");
   printf("\t<snoop_channel>    specifies the SNOOP channel used to SNOOP data reading test\n");
   printf("\n");
}


static int process_arguments( int argc, char** argv, unsigned int* p_snoop_channel)
{
   int i = 1;
   if (argc < 2)
   {
	ShowUsage();
	return -1;
   }
   else
   {
	   *p_snoop_channel = (unsigned int)strtol( argv[i++], NULL, 10);
   }

   return 0;

}


int main( int argc, char** argv )
{
    int ret = 0;
	unsigned char PostCodeBuf[MAX_SIZE];
    unsigned int snoop_channel = 0;
    int RetVal, i;

    ret = process_arguments (argc, argv, &snoop_channel);
  
    if (ret != 0)
        return -1;


	RetVal = ReadCurrentBiosCodeByCh (PostCodeBuf, sizeof (PostCodeBuf), snoop_channel);
	if (-1 == RetVal)
	{
		printf ("Error Reading Current PostCode buffer...\n");
		return -1;
	}
	else if (RetVal)
	{
		printf ("Current Post Codes are ...\n");
		for (i = 0; i < RetVal; i++)
		{
			printf ("0x%02x ", PostCodeBuf[i]);
		}
		printf ("\n");
	}

	return 0;
}

