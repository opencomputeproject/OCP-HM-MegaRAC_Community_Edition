/****************************************************************
 ****************************************************************
 **                                                            **
 **    (C)Copyright 2017-2018, American Megatrends Inc.        **
 **                                                            **
 **            All Rights Reserved.                            **
 **                                                            **
 **        6145-F, Northbelt Parkway, Norcross,                **
 **                                                            **
 **        Georgia - 30071, USA. Phone-(770)-246-8600.         **
 **                                                            **
 ****************************************************************
 ****************************************************************/
/*****************************************************************
 *
 * peci application to issue peci commands 
 *
 * Author: Samvinesh Christopher (vineshc@ami.com)
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

#include "peciifc.h"

typedef enum  {
	UNKNOWN = 0, PING, GET_TEMP, GET_DIB, RD_PKG, WR_PKG, RD_MSR, WR_MSR, RD_PCI_CONFIG, WR_PCI_CONFIG, RD_PCI_CONFIG_LOCAL, WR_PCI_CONFIG_LOCAL
}peci_operation;

#define CONVERT_TO_PLUS(value)	(value > 0)? value : - value
#define GET_REAL_TEMPERATURE(raw_value) (((CONVERT_TO_PLUS(raw_value)%64)*100)/64)

void
Usage(char *progname)
{

        printf("Usage :");
        printf("\t%s -d <device_number> -t <target> -m <domain> <option> [data]\n",progname);
        printf("\t\t-d : (Optional)PECI device number [0 - default]\n");
        printf("\t\t-t : (Optional)PECI Target address [48 - default]\n");
	printf("\t\t-f : (Optional)Displays the complete PECI Response\n");
	printf("\t\t-m <1/0> : (Optional)PECI Domain [0 - default]\n");
        printf("\t\tOptions : \n");
        printf("\t\t-p : ping the target\n");
        printf("\t\t-g : get the temperature\n");
        printf("\t\t-D : Get the Device info\n");
        printf("\t\t-R PCS/MSR : Provide Read operation\n");
        printf("\t\t-W PCS/MSR : Provide Write operation\n");
        printf("\t\t\t\t PCS - Package configuration space\n");
        printf("\t\t\t\t MSR - Model Specific Registers\n");
        printf("\t\t-H <host_id> : Host ID[7:1] & Retry[0]\n");
        printf("\t\t-I <index> : Index/Processor ID\n");
        printf("\t\t-P <parameter> : Parameter/MSR Address in hex\n");
        printf("\t\t-O <BYTE/WORD/DWORD/QWORD> : Operation\n");
	printf("\t\t-a <1/0> : Enable/Disable the AWFCS\n");
	printf("\t\t-C <Configuration_Address> : Read PCI Configuration Space\n");
	printf("\t\t-w <Configuration_Address> : Write to PCI Configuration Space\n");
	printf("\t\tConfiguration_Address : Reserved[31:28], Bus[27:20], Device[19:15], Function[14:12] & Register[11:0]\n");
	printf("\t\t-l : read/write Local PCI Configuration Space\n");
        printf("\t\tdata : Data for write in hex serarated by space like AB CD\n");
        printf("\t\tping example:\t %s -p\n\n",progname);
        printf("Manual Usage :\n");
        printf("\t%s -d <device_number> -t <target> -r <length> <command list>\n",progname);
        printf("\t\t-d : PECI device number \n");
        printf("\t\t-t : PECI Target address \n");
        printf("\t\t-r : Response Length\n");
        printf("\n");

	return;
}

static void dump_peci_rsp(unsigned char *buf, int len)
{
	int loop;

	printf("The Full PECI Response of Length %d Bytes is: \n", (int)len);
	for (loop = 0; loop < len; loop++)
	{
		printf("0x%02x ", buf[loop]);
	}
	printf("\n");
}

int 
main(int argc, char *argv [])
{
	int target = -1;
	int domain = -1;
	int read_len = -1;
	int xmit_feedback = 0;
	int local_pci = 0;
	int dev_id = 0;
	int write_len = 0, c, i;
	char temp_data[16] = {0};
       char temp_reqdata[16] = {0};
	unsigned char read_buf[16] = {0}, write_buf[16] = {0};
	int temperature = 0;
	unsigned short raw_temperature = 0;
	peci_operation op = UNKNOWN;
	unsigned char host = 0, index = 0, option = 0;
	unsigned short param = 0;
	unsigned long	pci_config_addr = 0;
	int 					awfcs_flag = 0;
	peci_rdpkgconfig_req_t rdpkg_in;
	peci_wrpkgconfig_req_t wrpkg_in;
	peci_rdiamsr_req_t rdi_in;
//	peci_wriamsr_req_t wri_in;
	peci_rdpciconfig_req_t	rdpci_in;
	peci_wrpciconfig_req_t	wrpci_in;
	int 					RetVal = 0;
	unsigned char temp = 0;

	/* Parse the input arguments */
	while ((c = getopt(argc,argv,":fpgDd:r:t:m:R:W:H:I:P:O:a:C:w:l")) != -1)
	{
		switch (c)
		{
			case 'd':
				dev_id = atoi(optarg);
				break;
		   	case 't':
				target = atoi(optarg);
				break;
			case 'm':
				domain = atoi(optarg);
				break;
			case 'f':
				xmit_feedback = 1;
				break;
			case 'l':
				local_pci = 1;
				break;
			case 'r':
				read_len = atoi(optarg);
				break;
			case 'p':
				if (op != UNKNOWN)
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				op = PING;
				break;
			case 'g':
				if (op != UNKNOWN)
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				op = GET_TEMP;
				break;
			case 'D':
				if (op != UNKNOWN)
				{
					Usage(argv[0]);
					RetVal = 0;
					goto exit_gracefully;
				}
				op = GET_DIB;
				break;
			case 'R' :
				if (op != UNKNOWN)
				{
					Usage(argv[0]);
					RetVal = 0;
					goto exit_gracefully;
				}
				else if (0 == strcmp (optarg, "PCS"))
				{
					op = RD_PKG;
				}
				else if  (0 == strcmp (optarg, "MSR"))
				{
					op = RD_MSR;
				}
				else
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				break;
			case 'W' :
				if (op != UNKNOWN)
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				else if (0 == strcmp (optarg, "PCS"))
				{
					op = WR_PKG;
				}
				else if  (0 == strcmp (optarg, "MSR"))
				{
					op = WR_MSR;
				}
				else
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				break;
			case 'H' :
				host = (unsigned char)strtol( optarg, NULL, 16 );
				break;
			case 'I' :
				index = (unsigned char)strtol( optarg, NULL, 16 );
				break;
			case 'O' :
				if (0 == strcmp(optarg, "BYTE"))
				{
					option = BYTE;
				}
				else if (0 == strcmp(optarg, "WORD"))
				{
					option = WORD;
				}
				else if (0 == strcmp(optarg, "DWORD"))
				{
					option = DWORD;
				}
				else if (0 == strcmp(optarg, "QWORD"))
				{
					option = QWORD;
				}
				else
				{
					Usage(argv[0]);
					RetVal = 1;
					goto exit_gracefully;
				}
				break;
			case 'P' :
				strncpy((char*) temp_data, optarg, 2);
				param = (unsigned char)strtol( temp_data, NULL, 16 );
				param = param << 8;
				strncpy((char*) temp_data, optarg + 2, 2);
				param |= (unsigned char)strtol( temp_data, NULL, 16 );
				break;
			case 'a' :
				awfcs_flag = atoi(optarg);
				break;
			case 'C' :
				op = RD_PCI_CONFIG;
				strncpy((char*) temp_data, optarg, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 24;
				strncpy((char*) temp_data, optarg + 2, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 16;
				strncpy((char*) temp_data, optarg + 4, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 8;
				strncpy((char*) temp_data, optarg + 6, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp;
                                break;
			case 'w' :
				op = WR_PCI_CONFIG;
				strncpy((char*) temp_data, optarg, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 24;
				strncpy((char*) temp_data, optarg + 2, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 16;
				strncpy((char*) temp_data, optarg + 4, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp << 8;
				strncpy((char*) temp_data, optarg + 6, 2);
				temp = (unsigned long)strtol( temp_data, NULL, 16);
				pci_config_addr |= temp;
				break;
		   	case ':':
				printf("ERROR: Option -%c needs an operand\n",optopt);	
				Usage(argv[0]);
				RetVal = 1;
				goto exit_gracefully;
		   	case '?':
				printf("ERROR: Unrecognized option -%c\n",optopt);
				Usage(argv[0]);
				RetVal = 1;
				goto exit_gracefully;
		}
	}

	if(UNKNOWN != op)
	{
		if(target == -1)
			target = 48;

		if (domain == -1) {
			domain = 0;
		}

		if(PING == op)
		{
			if(0 == peci_cmd_ping(dev_id, target))
			{
				printf("Ping Success\n");
				RetVal = 0;
				goto exit_gracefully;
			}
			else 
			{
				printf("\tERR : Ping Command failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}
		
		if(GET_TEMP == op)
		{
			if(0 == peci_cmd_read_temp(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, temp_data))
			{
				if (xmit_feedback)
				{
					dump_peci_rsp((unsigned char*) temp_data, read_len);
				}
				else
				{
					raw_temperature = (unsigned char) temp_data[1] << 8 | (unsigned char) temp_data[0];

					if (raw_temperature & 0x8000)
						temperature = -1;
					else
						temperature = 1;

					// Raw Temperature, do 2's complement on it (~x + 1) and then
					// shift right by 6 bits to remove the fractional value
					// This value is with reference to Tcc obtained from the CPU
					temperature *= ((unsigned short)(~raw_temperature + 1) >> 6);
					printf("\tHuman readable temperature w.r.t Tcc : %d Celsius\n\n", temperature);
				}

				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{ 
				printf("\tERR : Get Temperature failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}
		
		if(GET_DIB == op)
		{
			if(0 == peci_cmd_get_dib(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char*)temp_data))
			{
				if (xmit_feedback)
				{
					dump_peci_rsp((unsigned char*) temp_data, read_len);
				}
				else
				{
					printf("\tDevice Info : %x\n", ((peci_getdib_res_t *)temp_data)->device_info);
					printf("\tRevision Number : %x\n", ((peci_getdib_res_t *)temp_data)->revision_no);
				}

				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{ 
				printf("\tERR : Get DIB failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}
		
		if(RD_PKG == op) 
		{
			rdpkg_in.option = option;
			rdpkg_in.host_id = host;
			rdpkg_in.index = index;
			rdpkg_in.parameter = param;

                    memcpy(&temp_reqdata[0],&rdpkg_in.option,sizeof(peci_rdpkgconfig_req_t));
			
			if(0 == peci_cmd_rdpkgconfig(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data))
			{
				if (xmit_feedback)
				{
					dump_peci_rsp((unsigned char*) temp_data, read_len);
				}
				else
				{
					printf ("\tCompletion Code : %x\n", ((peci_rdpkgconfig_res_t *)temp_data)->completion_code);
					if (BYTE == option)
					{
						printf ("\tData : %x\n", ((peci_rdpkgconfig_res_t *)temp_data)->data.peci_byte);
					}
					else if (WORD == option)
					{
						printf ("\tData : %x\n", ((peci_rdpkgconfig_res_t *)temp_data)->data.peci_word);
					}
					else if (DWORD == option)
					{
						printf ("\tData : %lx\n", ((peci_rdpkgconfig_res_t *)temp_data)->data.peci_dword);
					}
				}

				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : RdPkgConfig command failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}
		
		if(WR_PKG == op)
		{
			for (write_len = 0;optind < argc;optind++)
			{
				write_buf[write_len++] = strtol(argv[optind], NULL, 16);
			}
			
			wrpkg_in.option = option;
			wrpkg_in.host_id = host;
			wrpkg_in.index = index;
			wrpkg_in.parameter = param;

			if (BYTE == option)
			{
				wrpkg_in.data.peci_byte = write_buf[0];
			}
			else if (WORD == option)
			{
				wrpkg_in.data.peci_word = write_buf[0] << 8 | write_buf[1];
			}
			else if (DWORD == option)
			{
				wrpkg_in.data.peci_dword = write_buf[0] << 24 | write_buf[1] << 16 | 
											write_buf[2] << 8 | write_buf[3];
			}

                     memcpy(&temp_reqdata[0],&wrpkg_in.option,sizeof(peci_wrpkgconfig_req_t));
			
			if(0 == peci_cmd_wrpkgconfig(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data))
			{
				printf ("\tCompletion Code : %x\n", ((peci_wrpkgconfig_res_t *)temp_data)->completion_code);
				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : WrPkgConfig command failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}
		
		if(RD_MSR == op)
		{
			rdi_in.option = option;
			rdi_in.host_id = host;
			rdi_in.processor_id = index;
			rdi_in.msr_addr = param;

                     memcpy(&temp_reqdata[0],&rdi_in.option,sizeof(peci_rdiamsr_req_t));
			
			if(0 == peci_cmd_rdiamsr(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data))
			{
				if (xmit_feedback)
				{
					dump_peci_rsp((unsigned char*) temp_data, read_len);
				}
				else
				{
					printf ("\tCompletion Code : %x\n", ((peci_rdiamsr_res_t *)temp_data)->completion_code);
					if (BYTE == option)
					{
						printf ("\tData : %x\n", ((peci_rdiamsr_res_t *)temp_data)->data.peci_byte);
					}
					else if (WORD == option)
					{
						printf ("\tData : %x\n", ((peci_rdiamsr_res_t *)temp_data)->data.peci_word);
					}
					else if (DWORD == option)
					{
						printf ("\tData : %lx\n", ((peci_rdiamsr_res_t *)temp_data)->data.peci_dword);
					}
					else if (QWORD == option)
					{
						printf ("\tData : ");
						for (i = 0; i < QWORD; i++)
						{
							printf ("%x ", ((peci_rdiamsr_res_t *)temp_data)->data.peci_qword[i]);
						}
						printf ("\n");
					}
				}

				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : RdPkgConfig command failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}

		if (WR_MSR == op)
		{
			for (write_len = 0;optind < argc;optind++)
			{
				write_buf[write_len++] = strtol(argv[optind], NULL, 16);
			}

#if 0
			wri_in.option = option;
			wri_in.host_id = host;
			wri_in.processor_id = index;
			wri_in.msr_addr = param;

			if (BYTE == option)
			{
				wri_in.data.peci_byte = write_buf[0];
			}
			else if (WORD == option)
			{
				wri_in.data.peci_word = write_buf[0] << 8 | write_buf[1];
			}
			else if (DWORD == option)
			{
				wri_in.data.peci_dword = write_buf[0] << 24 | write_buf[1] << 16 | 
											write_buf[2] << 8 | write_buf[3];
			}
#endif

                     memcpy(&temp_reqdata[0],&rdi_in.option,sizeof(peci_wriamsr_req_t));
			
			if(0 == peci_cmd_wriamsr(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len,(unsigned char *)temp_reqdata, (unsigned char *)temp_data))
			{
				printf ("\tCompletion Code : %x\n", ((peci_wriamsr_res_t *)temp_data)->completion_code);
				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : WrIaMsr command failed\n");
				RetVal = 1;
				goto exit_gracefully;
			}
		}

		if (RD_PCI_CONFIG == op)
		{
			int res;
			char *cmd;

			rdpci_in.option = option;
			rdpci_in.host_id = host;
			rdpci_in.pci_config_addr = pci_config_addr;

                     memcpy(&temp_reqdata[0],&rdpci_in.option,sizeof(peci_rdpciconfig_req_t));

			if (local_pci) {
				cmd = "RdPciConfigLocal";
				res = peci_cmd_rdpciconfiglocal(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data);
			} else {
				cmd = "RdPciConfig";
				res = peci_cmd_rdpciconfig(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data);
			}
			if(res == 0)
			{
                                if (xmit_feedback)
                                {
					dump_peci_rsp((unsigned char*) temp_data, read_len);
                                }
                                else
                                {
					printf ("\tCompletion Code : %x\n", ((peci_rdpciconfig_res_t *)temp_data)->completion_code);
					if (BYTE == option)
					{
						printf ("\tData : %x\n", ((peci_rdpciconfig_res_t *)temp_data)->data.peci_byte);
					}
					else if (WORD == option)
					{
						printf ("\tData : %x\n", ((peci_rdpciconfig_res_t *)temp_data)->data.peci_word);
					}
					else if (DWORD == option)
					{
						printf ("\tData : %lx\n", ((peci_rdpciconfig_res_t *)temp_data)->data.peci_dword);
					}
					else if (QWORD == option)
					{
						printf ("\tData : ");
						for (i = 0; i < QWORD; i++)
						{
							printf ("%x ", ((peci_rdpciconfig_res_t *)temp_data)->data.peci_qword[i]);
						}
						printf ("\n");
					}
				}

				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : %s command failed\n", cmd);
				RetVal = 1;
				goto exit_gracefully;
			}
		}

		if (WR_PCI_CONFIG == op)
		{
			int res;
			char *cmd;

			if (local_pci)
				op = WR_PCI_CONFIG_LOCAL;

			for (write_len = 0;optind < argc;optind++)
			{
				write_buf[write_len++] = strtol(argv[optind], NULL, 16);
			}

			if (BYTE == option)
			{
				wrpci_in.data.peci_byte = write_buf[0];
			}
			else if (WORD == option)
			{
				wrpci_in.data.peci_word = write_buf[0] << 8 | write_buf[1];
			}
			else if (DWORD == option)
			{
				wrpci_in.data.peci_dword = write_buf[0] << 24 | write_buf[1] << 16 | 
											write_buf[2] << 8 | write_buf[3];
			}

			wrpci_in.option = option;
			wrpci_in.host_id = host;
			wrpci_in.pci_config_addr = pci_config_addr;

                     memcpy(&temp_reqdata[0],&wrpci_in.option,sizeof(peci_wrpciconfig_req_t));
			
			if (local_pci) {
				cmd = "WrPciConfigLocal";
				res = peci_cmd_wrpciconfiglocal(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data);
			} else {
				cmd = "WrPciConfig";
				res = peci_cmd_wrpciconfig(dev_id, target, domain, xmit_feedback, awfcs_flag, &read_len, (unsigned char *)temp_reqdata, (unsigned char *)temp_data);
			}
			if(res == 0)
			{
				printf ("\tCompletion Code : %x\n", ((peci_wrpciconfig_res_t *)temp_data)->completion_code);
				RetVal = 0;
				goto exit_gracefully;
			}
			else
			{
				printf("\tERR : %s command failed\n", cmd);
				RetVal = 1;
				goto exit_gracefully;
			}
		}
	
	}

	/* Check if we got the required arguemnts */
	if ((target == -1) || (read_len == -1))
	{
		Usage(argv[0]);
		RetVal = 1;
		goto exit_gracefully;
	}

	/* Fill the PECI Packet */

	for (write_len = 0;optind < argc;optind++)
	   write_buf[write_len++] = atoi(argv[optind]);

	i = peci_generic_cmd  ( dev_id,
				target,
				domain,
				xmit_feedback,
				awfcs_flag,
				(char *)write_buf,
				write_len,
				(char *)read_buf,
				read_len );
			     
	if(i != 0)
	{
		printf("PECI Generic command failed\n");
		RetVal = -1;
		goto exit_gracefully;
	}

	printf("PECI Generic command passed\n");
	
	for(i=0;i<read_len;i++)
		printf("Received PECI Data[%d] = 0x%02x\n",i,read_buf[i]);

exit_gracefully:

	return RetVal;
}

