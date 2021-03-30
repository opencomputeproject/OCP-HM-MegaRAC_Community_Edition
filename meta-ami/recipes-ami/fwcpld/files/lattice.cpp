#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <termios.h>

#include <fstream>
#include <iostream>
#include <sys/mman.h>
#include "lattice.hpp"
#include "ast-jtag.hpp"
#include <syslog.h>

extern struct cpld_dev_info *cur_dev;
extern xfer_mode mode;
int debug = 0;
/*************************************************************************************/
void jed_file_paser_header(FILE *jed_fd)
{
	//file
	char tmp_buf[160];

	//Header paser
	while (fgets(tmp_buf, 120, jed_fd) != NULL) {
		if (debug) printf("%s \n", tmp_buf);
		if (tmp_buf[0] == 0x4C) { // "L"
			break;
		}
	}
}

int cpldcm()
{
        int i;
        unsigned int freq = 0;
        unsigned int jtag_freq = 0;
	syslog(LOG_WARNING,"### inside cplcm\n");
	std::ifstream erasefile("/tmp/erase");
	std::ifstream verifyfile("/tmp/verify");
	std::ifstream programfile("/tmp/program");

        if (erasefile || verifyfile || programfile ){
		system("/bin/ls -l /tmp/status");      
                return -1;
        }
	
	syslog(LOG_WARNING,"### before execl  cplcm\n");
	//execl("/bin/touch", "touch", "/tmp/status", (char*)0);
//	system("/bin/touch /tmp/status");      
	syslog(LOG_WARNING,"### after execl  cplcm\n");
        unsigned int dev_id;

	char dev_name[100] = "/dev/1e6e4000.jtag";

        if (ast_jtag_open(dev_name))
                exit(1);

        //show current ast jtag configuration
        jtag_freq = ast_get_jtag_freq();

        if (jtag_freq == 0) {
                syslog(LOG_WARNING,"Jtag freq error !! \n");
                exit(1);
        }

        if (freq) {
                ast_set_jtag_freq(freq);
                syslog(LOG_WARNING,"Mode : %s , JTAG Set Freq %d", mode ? "SW" : "HW", freq);
        } else {
                syslog(LOG_WARNING,"Mode : %s , JTAG Freq %d", mode ? "SW" : "HW", jtag_freq);
        }

        ast_jtag_run_test_idle(1, 0, 0);

        lattice_get_id_pub(&dev_id);

        for (i = 0; i < ARRAY_SIZE(lattice_device_list); i++) {
                if (dev_id == lattice_device_list[i].dev_id)
                        break;
        }

        if (i == ARRAY_SIZE(lattice_device_list)) {
                syslog(LOG_WARNING,"AST LATTICE Device - UnKnow : %x \n", dev_id);
                cur_dev = NULL;
                return -1;
        } else {
                cur_dev = &lattice_device_list[i];
                syslog(LOG_WARNING,"AST LATTICE Device : %s \n", cur_dev->name);
        }

	syslog(LOG_WARNING,"coming from cpldfw \n");
        return 0;
}

int lattice_get_id_pub(unsigned int *id)
{
        unsigned int tdo;
        unsigned int *ir_tdi_data;
        unsigned int *ir_tdo_data;
        ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
        ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
        //SIR   8       TDI  (16);
        *ir_tdi_data = IDCODE_PUB;
        tdo = ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

        ast_jtag_tdo_xfer(0, 32, id);

//      printf("get id : %x \n", *id);
        return 0;
}

void jed_file_paser(FILE *jed_fd, unsigned int len, unsigned int *dr_data)
{
	int i = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;
	int err_flag;

	if (debug) printf("file len = %d \n", len);
	//Jed row
	for (i = 0; i < len; i++) {
		input_char = fgetc(jed_fd);
		if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
//			printf("%c", input_char);
			if (input_char == 0x30) {
				input_bit = 0;
			} else {
				input_bit = 1;
			}
			if (debug) printf("%d", input_bit);
			dr_data[sdr_array] |= (input_bit << data_bit);
			data_bit++;
			bit_cnt++;

			if ((data_bit % 32) == 0) {
				if (debug) printf(" [%i] : %x \n", sdr_array, dr_data[sdr_array]);
				data_bit = 0;
				sdr_array++;
			}
			err_flag = 0;
		} else if (input_char == 0xd) {
			//printf(" ");
			i--;
//			printf("paser error [%x]\n", input_char);
		} else if (input_char == 0xa) {
			i--;
			//printf("\n");
//			printf("paser error [%x]\n", input_char);
		} else {
			//printf("paser errorxx [%x : %c] \n", input_char, input_char);
			printf("%c", input_char);
			err_flag = 1;
			break;
		}
	}
	if (debug) printf(" [%i] : %x , Total %d \n", sdr_array, dr_data[sdr_array], bit_cnt);
	
	if (bit_cnt != len) {
		if(err_flag) {
			printf("\n");
		} else {
			printf("File Error - bit_cnt %d, len %d\n", bit_cnt, len);
		}
	}
//	} while (input_char != 0x2A); // "*"

}

/* LC LC4064V-XXT4 */
int lc4064v_xxt4_cpld_program(FILE *jed_fd)
{
	int i;
	char tmp_buf[160];
	unsigned int *dr_data_r;
	unsigned int *dr_data_w;
	unsigned int *dr_data_addr;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int row  = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;
	int cmp_err = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));

	//! Enable the programming mode
	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Erase the device
	//! Shift in ISC ERASE(0x03) instruction
	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in DISCHARGE(0x14) instruction
	*ir_tdi_data = DISCHARGE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Full Address Program Fuse Map

	//! Shift in ISC ADDRESS INIT(0x21) instruction
	*ir_tdi_data = ISC_ADDRESS_INIT;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in ISC PROGRAM INCR(0x27) instruction
	*ir_tdi_data = ISC_PROG_INCR;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	row = 0;
	dr_data_w = static_cast<unsigned int *> (malloc((cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_w, 0, (cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int));
	dr_data_r = static_cast<unsigned int *> (malloc((cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_r, 0, (cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int));

	while (fgets(tmp_buf, 120, jed_fd) != NULL) {
		//printf("%s \n",tmp_buf);
		if (tmp_buf[0] == 0x4C) { // "L"
			do {
				input_char = fgetc(jed_fd);
				if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
					if (input_char == 0x30) {
						input_bit = 0;
					} else {
						input_bit = 1;
					}
					//printf("%d",input_bit);
					dr_data_w[sdr_array] |= (input_bit << data_bit);
					data_bit++;
					bit_cnt++;

					if ((data_bit % 32) == 0) {
						//printf("dr_data_w[%i] : %x \n",sdr_array, dr_data_w[sdr_array]);
						data_bit = 0;
						sdr_array++;
						dr_data_w[sdr_array] = 0;
					}
					if ((bit_cnt % cur_dev->dr_bits) == 0) {
						//printf("SHIFT DR to JTAG %d \n",bit_cnt);
						ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data_w);
						usleep(5000);
						sdr_array = 0;
						dr_data_w[0] = 0;
						row++;
					}
				}
			} while (input_char != 0x2A); // "*"
		}
	}
	printf("Program Done row = %d \n", row);

	if (row != cur_dev->row_num)
		printf("row mis-match !!\n");

	printf("Starting to Verify Device . . . This will take a few seconds\n");
	dr_data_addr = static_cast<unsigned int *> (malloc(((row + 1) / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_addr, 0, ((row + 1) / 32 + 1) * sizeof(unsigned int));

	//Move the address to verify data base
	*ir_tdi_data = ISC_ADDRESS_SHIFT;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	for (i = 0; i < row / 32; i++) {
		dr_data_addr[i] = 0;
//			printf("addr[i] %x ",i, dr_data_addr[i]);
	}
	dr_data_addr[i] = (1 << ((row % 32) - 1));
//		printf("addr[i] %x ",i, dr_data_addr[i]);
	ast_jtag_tdi_xfer(0, row, dr_data_addr);

	//Verify JTAG device by using the auto-incremen command
	*ir_tdi_data = ISC_READ_INCR;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

//		printf("READ From CPLD and Comp with JDE \n");
	sdr_array = 0;
	data_bit = 0;
	bit_cnt = 0;
	row = 0;

	fseek(jed_fd, 0, SEEK_SET);
	while (fgets(tmp_buf, 120, jed_fd) != NULL) {
//			printf("%s \n",tmp_buf);
		if (tmp_buf[0] == 0x4C) { // "L"
			do {
				input_char = fgetc(jed_fd);
				if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
					if (input_char == 0x30) {
						input_bit = 0;
					} else {
						input_bit = 1;
					}
///						 printf("%d",input_bit);
					dr_data_r[sdr_array] |= (input_bit << data_bit);
					data_bit++;
					bit_cnt++;

					if ((data_bit % 32) == 0) {
//							 printf("dr_data_r[%i] : %x \n",sdr_array, dr_data_r[sdr_array]);
						data_bit = 0;
						sdr_array++;
						dr_data_r[sdr_array] = 0;
					}
					if ((bit_cnt % cur_dev->dr_bits) == 0) {
						ast_jtag_tdo_xfer(0, cur_dev->dr_bits, dr_data_w);
						for (i = 0; i < (cur_dev->dr_bits / 32); i++) {
//								 printf("From CPLD [%d] %x \n",i, dr_data_w[i]);
							if (dr_data_w[i] != dr_data_r[i])
								cmp_err = 1;
						}
						sdr_array = 0;
						dr_data_r[0] = 0;
						row++;
					}
				}
			} while (input_char != 0x2A); // "*"
		}
	}

	//Finish program
	*ir_tdi_data = PROGRAM_DONE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	*ir_tdi_data = PROGRAM_DISABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	free(dr_data_addr);
	free(dr_data_w);
	free(dr_data_r);

	if (cmp_err)
		printf("Verify Error !!\n");
	else
		printf("Verify Done !!\n");

	return 0;
}

int lc4064v_xxt4_cpld_verify(FILE *jed_fd)
{
	int i;
	char tmp_buf[160];
	unsigned int *dr_data_r;
	unsigned int *dr_data_w;
	unsigned int *dr_data_addr;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int row  = 0;
	unsigned char input_char, input_bit;
	int sdr_array = 0, data_bit = 0, bit_cnt = 0;
	int cmp_err = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	//! Enable the programming mode

	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Full Address Verify Fuse Map

	//! Shift in ISC ADDRESS SHIFT(0x01) instruction
	*ir_tdi_data = ISC_ADDRESS_SHIFT;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	row = 0;
	fseek(jed_fd, 0, SEEK_SET);
	while (fgets(tmp_buf, 120, jed_fd) != NULL) {
		if (tmp_buf[0] == 0x4C) { // "L"
			do {
				input_char = fgetc(jed_fd);
				if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
					bit_cnt++;
					if ((bit_cnt % cur_dev->dr_bits) == 0) {
						row++;
					}
				}
			} while (input_char != 0x2A); // "*"
		}
	}
	syslog(LOG_WARNING,"row = %d \n", row);

	syslog(LOG_WARNING,"Starting to Verify Device . . . This will take a few seconds\n");

	dr_data_addr = static_cast<unsigned int *> (malloc(((row + 1) / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_addr, 0, ((row + 1) / 32 + 1) * sizeof(unsigned int));

	///////////////////////////////////////
	//SDR	95	TDI  (400000000000000000000000);
	for (i = 0; i < row / 32; i++) {
		dr_data_addr[i] = 0;
//			printf("addr[i] %x ",i, dr_data_addr[i]);
	}
	dr_data_addr[i] = (1 << ((row % 32) - 1));
//		printf("addr[i] %x ",i, dr_data_addr[i]);
	ast_jtag_tdi_xfer(0, row, dr_data_addr);
	///////////////////////////////////////

	//Verify JTAG device by using the auto-incremen command
	//! Shift in ISC READ INCR(0x2A) instruction
	*ir_tdi_data = ISC_READ_INCR;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	dr_data_w = static_cast<unsigned int *> (malloc((cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_w, 0, (cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int));
	dr_data_r = static_cast<unsigned int *> (malloc((cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int)));
	memset(dr_data_r, 0, (cur_dev->dr_bits / 32 + 1) * sizeof(unsigned int));

	//printf("READ From CPLD and Comp with JDE \n");
	sdr_array = 0;
	data_bit = 0;
	bit_cnt = 0;
	row = 0;

	fseek(jed_fd, 0, SEEK_SET);
	while (fgets(tmp_buf, 120, jed_fd) != NULL) {
		//			printf("%s \n",tmp_buf);
		if (tmp_buf[0] == 0x4C) { // "L"
			do {
				input_char = fgetc(jed_fd);
				if ((input_char == 0x30) || (input_char == 0x31)) { // "0", "1"
					if (input_char == 0x30) {
						input_bit = 0;
					} else {
						input_bit = 1;
					}
					/// 					 printf("%d",input_bit);
					dr_data_r[sdr_array] |= (input_bit << data_bit);
					data_bit++;
					bit_cnt++;

					if ((data_bit % 32) == 0) {
						//							 printf("dr_data_r[%i] : %x \n",sdr_array, dr_data_r[sdr_array]);
						data_bit = 0;
						sdr_array++;
						dr_data_r[sdr_array] = 0;
					}
					if ((bit_cnt % cur_dev->dr_bits) == 0) {
						ast_jtag_tdo_xfer(0, cur_dev->dr_bits, dr_data_w);
						for (i = 0; i < (cur_dev->dr_bits / 32); i++) {
							//								 printf("From CPLD [%d] %x \n",i, dr_data_w[i]);
							if (dr_data_w[i] != dr_data_r[i])
								cmp_err = 1;
						}
						sdr_array = 0;
						dr_data_r[0] = 0;
						row++;
					}
				}
			} while (input_char != 0x2A); // "*"
		}
	}

	free(dr_data_addr);
	free(dr_data_r);
	free(dr_data_w);

	if (cmp_err)
		syslog(LOG_WARNING,"Verify Error !!\n");
	else
		syslog(LOG_WARNING,"Verify Done !!\n");

	return 0;
}
/* Erase JTAG device
The 100ms delay is the experiment value to make sure that the CPLD's EEPROM is erased
*/
int lc4064v_xxt4_cpld_erase(void)
{
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));

	syslog(LOG_WARNING,"Starting to Erase Device . . .\n");
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//The 100ms is experiment result
	sleep(1);

	*ir_tdi_data = DISCHARGE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	*ir_tdi_data = PROGRAM_DISABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	return 0;
}

/*************************************************************************************/
/* LC LCMXO2280C */
int lcmxo2280c_cpld_program(FILE *jed_fd)
{
	int i;
	unsigned int *dr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;

	//file
	unsigned int *jed_data;

	unsigned int row  = 0;
	int cmp_err = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	dr_data = static_cast<unsigned int *> (malloc(((544 / 32) + 1) * sizeof(unsigned int)));
	jed_data = static_cast<unsigned int *> (malloc(((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(1, 0, 15);
	usleep(3000);

	//! Check the IDCODE

	//! Shift in IDCODE(0x16) instruction
	//SIR	8	TDI  (16);
	*ir_tdi_data = IDCODE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (FFFFFFFF)
	//		TDO  (0128D043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer(1, 32, dr_data);

	if (dr_data[0] != 0x128D043) {
		syslog(LOG_WARNING,"ID Fail : %08x [0x0128D043] \n", dr_data[0]);
		return -1;
	}

	//! Program Bscan register

	if (debug) printf("Program Bscan register \n");
	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = PRELOAD;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	544	TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//			 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((544 / 32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(1, 544, dr_data);

	//! Enable the programming mode
	if (debug) printf("Enable the programming mode \n");

	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Erase the device
	syslog(LOG_WARNING,"Erase the device : ");

	//! Shift in ISC SRAM ENABLE(0x55) instruction
	*ir_tdi_data = SRAM_ENABLE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in ISC ERASE(0x03) instruction
	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);


	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in ISC ERASE(0x03) instruction
	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//LOOP 100 ;
	//RUNTEST IDLE	5 TCK	1.00E-001 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (1);
	for (i = 0; i < 100 ; i++) {
		ast_jtag_run_test_idle(0, 0, 5);
		usleep(300000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(1, 1, dr_data);
		if (dr_data[0] == 1)
			break;
	}


//	if(dr_data[0] != 1) {
//		printf("ERASE Fail %d - [1]\n", dr_data[0]);
//		return -1;
//	}

	//! Read the status bit

	//! Shift in READ STATUS(0xB2) instruction
	*ir_tdi_data = READ_STATUS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (0);
	dr_data[0] = 0;
	ast_jtag_tdo_xfer(1, 1, dr_data);
	if (dr_data[0] != 0) {
		printf("Status Fail : %d [0], ", dr_data[0]);
		return -1;
	}

	//! ! Program Fuse Map

	//! Shift in ISC ADDRESS INIT(0x21) instruction
	*ir_tdi_data = ISC_ADDRESS_INIT;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	syslog(LOG_WARNING,"Done \n");

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_paser_header(jed_fd);

	syslog(LOG_WARNING,"Program.... : \n");
//	system("echo 1 > /sys/class/gpio/gpio104/value");

	for (row = 0 ; row < cur_dev->row_num; row++) {
		///////////////////////////////////////////////////////
		//! Shift in DATA SHIFT(0x02) instruction
		*ir_tdi_data = DATA_SHIFT;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
		usleep(3000);
		///////////////////////////////////////////////////////
		//printf("SHIFT DR to JTAG %d \n",bit_cnt);
		ast_jtag_tdi_xfer(1, cur_dev->dr_bits, dr_data);
		usleep(3000);
		///////////////////////////////////////////////////////
		//! Shift in LSCC PROGRAM INCR RTI(0x67) instruction
		*ir_tdi_data = LSCC_PROGRAM_INCR_RTI;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
		//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
		ast_jtag_run_test_idle(0, 0, 5);
		usleep(3000);
		memset(dr_data, 0, ((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, dr_data);
		///////////////////////////////////////////////////////
		//LOOP 10 ;
		//RUNTEST	DRPAUSE	1.00E-003 SEC;
		//SDR	1	TDI  (0)
		//		TDO  (1);
		//ENDLOOP ;
		for (i = 0; i < 10; i++) {
			ast_jtag_run_test_idle(0, 2, 0);
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(1, 1, dr_data);
			if (dr_data[0] == 1)
				break;
		}
		if (dr_data[0] == 0)
			syslog(LOG_WARNING,"row %d, Fail [%d] \n", row, dr_data[0]);
		else
			printf(".");

//		printf("\n");
	}

	syslog(LOG_WARNING,"row : %d, ", row);
	syslog(LOG_WARNING,"Done \n");

	//! Shift in INIT ADDRESS(0x21) instruction
	*ir_tdi_data = ISC_ADDRESS_INIT;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR	8	TDI  (FF);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Program USERCODE

	//! Shift in READ USERCODE(0x17) instruction
	*ir_tdi_data = UES_READ;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	32	TDI  (00000000);
	dr_data[0] = 0;
	ast_jtag_tdi_xfer(1, 32, dr_data);

	//! Shift in ISC PROGRAM USERCODE(0x1A) instruction
	//SIR	8	TDI  (1A);
	*ir_tdi_data = UES_PROGRAM;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//RUNTEST	IDLE	5 TCK	1.00E-002 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(2000);

	//! Read the status bit

	//! Shift in READ STATUS(0xB2) instruction
	//SIR 8	TDI  (B2);
	*ir_tdi_data = READ_STATUS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (0);
	dr_data[0] = 0;
	ast_jtag_tdo_xfer(1, 1, dr_data);
	if (dr_data[0] != 0) {
		printf("Read status Error : %d\n", dr_data[0]);
		return 1;
	}

	syslog(LOG_WARNING,"Verify Device . . . This will take a few seconds\n");

	//! Verify Fuse Map

	//! Shift in INIT ADDRESS(0x21) instruction
	*ir_tdi_data = ISC_ADDRESS_INIT;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in LSCC READ INCR RTI(0x6A) instruction
	*ir_tdi_data = LSCC_READ_INCR_RTI;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_paser_header(jed_fd);

	syslog(LOG_WARNING,"Verify Row : \n");
	cmp_err = 0;
	for (row = 0 ; row < cur_dev->row_num; row++) {
		syslog(LOG_WARNING,"%d ", row);
		memset(dr_data, 0, ((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int));
		memset(jed_data, 0, ((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, jed_data);
		ast_jtag_tdo_xfer(1, cur_dev->dr_bits, dr_data);

		for (i = 0; i < (cur_dev->dr_bits / 32 + 1); i++) {
			if (dr_data[i] != jed_data[i]) {
				syslog(LOG_WARNING,"JED : %x, SDR : %x \n", jed_data[i], dr_data[i]);
				cmp_err = 1;
			}
		}
		//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
		ast_jtag_run_test_idle(0, 0, 5);
		usleep(3000);
//		printf("\n");
		if (cmp_err) {
			break;
		}
	}
	syslog(LOG_WARNING,"\n");

	if (cmp_err) {
		syslog(LOG_WARNING,"Fail \n");
		return -1;
	}

	//! Verify USERCODE

	//! Shift in READ USERCODE(0x17) instruction
	//SIR 8	TDI  (17);
	*ir_tdi_data = UES_READ;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 32	TDI  (FFFFFFFF)
	//		TDO  (00000000);
	dr_data[0] = 0xffffffff;
	ast_jtag_tdo_xfer(1, 32, dr_data);

	//! Program DONE bit

	//! Shift in ISC PROGRAM DONE(0x2F) instruction
	*ir_tdi_data = PROGRAM_DONE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (1);
	dr_data[0] = 0;
	ast_jtag_tdo_xfer(1, 1, dr_data);
	syslog(LOG_WARNING,"Read status : %d [1], ", dr_data[0]);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	printf("Bypass : %x [0x1D] , ",tdo);

	//! Read the status bit
	//! Shift in READ STATUS(0xB2) instruction
	*ir_tdi_data = READ_STATUS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (0);
	dr_data[0] = 0;
	ast_jtag_tdo_xfer(1, 1, dr_data);
	syslog(LOG_WARNING,"Read status : %d [0], ", dr_data[0]);


	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x1E) instruction
	//SIR 8	TDI  (1E);
	*ir_tdi_data = PROGRAM_DISABLE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Verify SRAM DONE Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (1D);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	// printf("Bypass : %x [0x1D] , ", tdo);

	free(dr_data);
	free(jed_data);

	if (cmp_err)
		syslog(LOG_WARNING,"Verify Error !!\n");
	else
		syslog(LOG_WARNING,"Verify Done !!\n");

	return 0;
}

int lcmxo2280c_cpld_verify(FILE *jed_fd)
{
	int i;
	unsigned int *jed_data;
	unsigned int *dr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int row  = 0;
	int cmp_err = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	dr_data = static_cast<unsigned int *> (malloc(((544 / 32) + 1) * sizeof(unsigned int)));
	jed_data = static_cast<unsigned int *> (malloc(((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(1, 0, 15);
	usleep(3000);

	//! Check the IDCODE

	//! Shift in IDCODE(0x16) instruction
	//SIR	8	TDI  (16);
	*ir_tdi_data = IDCODE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (FFFFFFFF)
	//		TDO  (0128D043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer(1, 32, dr_data);

	if (dr_data[0] != 0x128D043) {
		syslog(LOG_WARNING,"ID Fail : %08x [0x0128D043] \n", dr_data[0]);
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = PRELOAD;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	544	TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//			 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, (544 / 32) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(1, 544, dr_data);

	//! Enable the programming mode

	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Verify Fuse Map

	//! Shift in LSCC RESET ADDRESS(0x21) instruction
	*ir_tdi_data = ISC_ADDRESS_INIT;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in LSCC READ INCR RTI(0x6A) instruction
	*ir_tdi_data = LSCC_READ_INCR_RTI;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	syslog(LOG_WARNING,"Starting to Verify Device . . . This will take a few seconds\n");

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_paser_header(jed_fd);

	syslog(LOG_WARNING,"Verify Row : \n");
	cmp_err = 0;
	row = 0;

	for (row = 0 ; row < cur_dev->row_num; row++) {
		syslog(LOG_WARNING,"%d \n", row);
		memset(dr_data, 0, ((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int));
		memset(jed_data, 0, ((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, jed_data);
		ast_jtag_tdo_xfer(1, cur_dev->dr_bits, dr_data);

		for (i = 0; i < (cur_dev->dr_bits / 32 + 1); i++) {
			if (dr_data[i] != jed_data[i]) {
				syslog(LOG_WARNING,"JED : %x, SDR : %x \n", jed_data[i], dr_data[i]);
				cmp_err = 1;
			}
		}
		//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
		ast_jtag_run_test_idle(0, 0, 5);
		usleep(3000);
//		printf("\n");
		if (cmp_err) {
			break;
		}
	}

	//! Verify USERCODE

	//! Shift in READ USERCODE(0x17) instruction
	*ir_tdi_data = UES_READ;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 32	TDI  (FFFFFFFF)
	//		TDO  (00000000);
	dr_data[0] = 0xffffffff;
	ast_jtag_tdo_xfer(1, 32, dr_data);

//	if(dr_data[0] != 0)
//		printf("USER Code %x \n ", dr_data[0]);

	//! Read the status bit

	//! Shift in READ STATUS(0xB2) instruction
	*ir_tdi_data = READ_STATUS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (0);
	dr_data[0] = 0;
	ast_jtag_tdo_xfer(1, 1, dr_data);
	if (dr_data[0] != 0)
		syslog(LOG_WARNING,"Read Status Fail %d \n", dr_data[0]);

	//! Verify Done Bit

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	if(tdo != 0x1D)
//		printf("BYPASS error %x \n", tdo);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x1E) instruction
	*ir_tdi_data = PROGRAM_DISABLE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	free(jed_data);
	free(dr_data);

	if (cmp_err)
		syslog(LOG_WARNING,"Verify Error !!\n");
	else
		syslog(LOG_WARNING,"Verify Done !!\n");

	return 0;
}

int lcmxo2280c_cpld_erase(void)
{
	int i = 0;
	unsigned int *sdr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int data = 0;
	unsigned int sdr_array = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(1, 0, 15);
	usleep(3000);

	//! Check the IDCODE
	//! Shift in IDCODE(0x16) instruction
	//SIR	8	TDI  (16);
	*ir_tdi_data = IDCODE;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (FFFFFFFF)
	//		TDO  (0128D043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer(1, 32, &data);

	if (data != 0x128D043) {
		printf("ID Fail : %08x [0x0128D043] \n", data);
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = PRELOAD;
	ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	544	TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//			 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	sdr_array = 544 / 32;
	sdr_data = static_cast<unsigned int *> (malloc(sdr_array * sizeof(unsigned int)));
	memset(sdr_data, 0xff, sdr_array * sizeof(unsigned int));
	ast_jtag_tdi_xfer(1, 544, sdr_data);
	free(sdr_data);

	//! Enable the programming mode

	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Erase the device

	//! Shift in ISC SRAM ENABLE(0x55) instruction
	*ir_tdi_data = SRAM_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in ISC ERASE(0x03) instruction
	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in ISC ENABLE(0x15) instruction
	*ir_tdi_data = ISC_ENABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in ISC ERASE(0x03) instruction
	*ir_tdi_data = ISC_ERASE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//LOOP 100 ;
	//RUNTEST IDLE	5 TCK	1.00E-001 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (1);
	//ENDLOOP ;
	data = 0;
	for (i = 0; i < 100 ; i++) {
		ast_jtag_run_test_idle(0, 0, 5);
		usleep(1000);
		ast_jtag_tdo_xfer(1, 1, &data);
	}

//	if(data != 1) {
//		printf("Erase Fail %d [1] \n",data);
//		return -1;
//	}

	//! Read the status bit

	//! Shift in READ STATUS(0xB2) instruction
	*ir_tdi_data = READ_STATUS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//SDR 1	TDI  (0)
	//		TDO  (0);
	ast_jtag_tdo_xfer(1, 1, &data);

	if (data != 0) {
		printf("Read status fail %d [0]\n", data);
		return -1;
	}

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x1E) instruction
	*ir_tdi_data = PROGRAM_DISABLE;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	//RUNTEST	IDLE	5 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 5);
	usleep(3000);

	printf("Done \n");

	return 0;
}

int lcmxo2_70000he_cpld_program(FILE *jed_fd)
{
	int i;
	unsigned int *dr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	//file
	unsigned int *jed_data;

	unsigned int row  = 0;
//	system("echo 10 > /sys/class/gpio/export");
//	system("echo out > /sys/class/gpio/gpio10/direction");
//	system("echo 0 > /sys/class/gpio/gpio10/value");
	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	dr_data = static_cast<unsigned int *> (malloc(((664 / 32) + 1) * sizeof(unsigned int)));
	jed_data = static_cast<unsigned int *> (malloc(((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	*ir_tdi_data = IDCODE_PUB;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	if (dr_data[0] != 0x12B5043) {
		syslog(LOG_WARNING,"ID Fail : %08x [0x012B5043] \n", dr_data[0]);
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = 0x1C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	664 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//		 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//		 FFFFFF);
	memset(dr_data, 0xff, (664 / 32 + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 664, dr_data);

	//! Enable the Flash
	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8 TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8 TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x00;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	*ir_tdi_data = 0x0E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8 TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	dr_data[0] = 0x01;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8 TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8 TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8 TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x08;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
//	if(dr_data[0] != 0x0) printf("%x [0x0]\n", dr_data[0]);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
//	if(dr_data[0] != 0x0) printf("%x [0x0]\n", dr_data[0]);

	//! Erase the Flash
	syslog(LOG_WARNING,"Erase the Flash \n");
	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8 TDI  (0E);
	*ir_tdi_data = 0x0E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8 TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0E;
	ast_jtag_tdi_xfer(0, 8, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8 TDI  (F0);
	*ir_tdi_data = 0xF0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1 TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	syslog(LOG_WARNING,"LOOP 800  \n");
	dr_data[0] = 0;
	for (i = 0; i < 800 ; i++) {
//		printf("loop count %d \n",i);
//		ast_jtag_run_test_idle( 0, 0, 2);
		usleep(2000);
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8 TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
	if (dr_data[0] != 0x0) syslog(LOG_WARNING,"%x [0x0]\n", dr_data[0]);

	syslog(LOG_WARNING,"Erase Done \n");

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_paser_header(jed_fd);

	//! Program CFG

	syslog(LOG_WARNING,"Program CFG \n");

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	*ir_tdi_data = 0x46;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	dr_data[0] = 0x04;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	syslog(LOG_WARNING,"Program 9212 .. \n");

//	system("echo 1 > /sys/class/gpio/gpio10/value");

//	mode = SW_MODE;
	for (row = 0 ; row < cur_dev->row_num; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8 TDI  (70);
		*ir_tdi_data = 0x70;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);


		//! Shift in Data Row = 1
		//SDR 128 TDI  (120600000040000000DCFFFFCDBDFFFF);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);
		usleep(1000);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8 TDI  (F0);
		*ir_tdi_data = 0xF0;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1 TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for (i = 0; i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if (dr_data[0] == 0) break;
		}

		if (dr_data[0] != 0)
			syslog(LOG_WARNING,"row %d, Fail [%d] \n", row, dr_data[0]);
		else
			syslog(LOG_WARNING,".");

	}
//	mode = HW_MODE;

	syslog(LOG_WARNING,"\nDone\n");

	//! Program the UFM
	syslog(LOG_WARNING,"Program the UFM : 2048\n");

	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR 8	TDI  (47);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x47;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	for (row = 0 ; row < 2048; row++) {
		memset(dr_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_PROG_INCR_NV(0x70) instruction
		//SIR 8	TDI  (70);
		*ir_tdi_data = 0x70;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

		//! Shift in Data Row = 1
		//SDR 128 TDI  (00000000000000000000000000000000);
		//RUNTEST IDLE	2 TCK;
		ast_jtag_tdi_xfer(0, cur_dev->dr_bits, dr_data);

		//! Shift in LSC_CHECK_BUSY(0xF0) instruction
		//SIR 8	TDI  (F0);
		*ir_tdi_data = 0xF0;
		ast_jtag_sir_xfer(1, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

		//LOOP 10 ;
		//RUNTEST IDLE	1.00E-003 SEC;
		//SDR 1	TDI  (0)
		//		TDO  (0);
		//ENDLOOP ;
		for (i = 0; i < 10; i++) {
			usleep(3000);
			dr_data[0] = 0;
			ast_jtag_tdo_xfer(0, 1, dr_data);
			if (dr_data[0] == 0) break;
		}

		if (dr_data[0] != 0)
			syslog(LOG_WARNING,"row %d, Fail [%d] \n", row, dr_data[0]);
		else
			syslog(LOG_WARNING,".");

	}
	syslog(LOG_WARNING,"\nDone\n");

	//! Program USERCODE

	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	*ir_tdi_data = 0xC0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (00000000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdi_xfer(0, 32, dr_data);

	//! Shift in ISC PROGRAM USERCODE(0xC2) instruction
	//SIR 8	TDI  (C2);
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	*ir_tdi_data = 0xC2;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(2000);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);
//	if(dr_data[0] != 0x0) printf("Read the status error %x \n", dr_data[0]);

	//! Program Feature Rows

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	*ir_tdi_data = 0x46;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (02);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	dr_data[0] = 0x02;
	ast_jtag_tdi_xfer(0, 8, dr_data);
	usleep(3000);

	//! Shift in LSC_PROG_FEATURE(0xE4) instruction
	//SIR 8	TDI  (E4);
	*ir_tdi_data = 0xE4;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 64	TDI  (0000000000000000);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdi_xfer(0, 64, dr_data);


	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	*ir_tdi_data = 0xF0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0xE7;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer(0, 64, dr_data);
//	if((dr_data[0] != 0x0) || (dr_data[1] != 0x0)) printf(" %x %x [0x0, 0x0] \n", dr_data[0], dr_data[1]);

	//! Shift in in LSC_PROG_FEABITS(0xF8) instruction
	//SIR 8	TDI  (F8);
	*ir_tdi_data = 0xF8;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 16	TDI  (0620);
	//RUNTEST IDLE	2 TCK;
	dr_data[0] = 0x0620;
	ast_jtag_tdi_xfer(0, 16, dr_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	*ir_tdi_data = 0xF0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0xFB;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x0;
	ast_jtag_tdo_xfer(0, 16, dr_data);
//	if(dr_data[0] != 0x0620) printf("%04x [0620]\n", dr_data[0] & 0xfff2);

	//! Program DONE bit

	//! Shift in ISC PROGRAM DONE(0x5E) instruction
	//SIR 8	TDI  (5E);
	//RUNTEST IDLE	2 TCK;
	*ir_tdi_data = 0x5E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	*ir_tdi_data = 0xF0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//LOOP 10 ;
	//RUNTEST IDLE	1.00E-003 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	for (i = 0; i < 10; i++) {
		usleep(3000);
		dr_data[0] = 0;
		ast_jtag_tdo_xfer(0, 1, dr_data);
	}

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (C4);
	*ir_tdi_data = 0xFF;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	*ir_tdi_data = 0x26;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	*ir_tdi_data = 0xFF;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	free(jed_data);
	free(dr_data);

	return 0;

}

int lcmxo2_70000he_cpld_verify(FILE *jed_fd)
{
	int i;
	unsigned int data = 0;
	unsigned int *jed_data;
	unsigned int *dr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int row  = 0;
	int cmp_err = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	dr_data = static_cast<unsigned int *> (malloc(((664 / 32) + 1) * sizeof(unsigned int)));
	jed_data = static_cast<unsigned int *> (malloc(((cur_dev->dr_bits / 32) + 1) * sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 3);
	usleep(3000);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	*ir_tdi_data = IDCODE_PUB;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (00000000)
	//		TDO  (012B5043)
	//		MASK (FFFFFFFF);
	ast_jtag_tdo_xfer(0, 32, dr_data);

	if (dr_data[0] != 0x12B5043) {
		syslog(LOG_WARNING,"ID Fail : %08x [0x012B5043] \n", dr_data[0]);
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = 0x1C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	664 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//			 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF);
	memset(dr_data, 0xff, ((664 / 32) + 1) * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 664, dr_data);

	//! Enable the Flash
	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8	TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	*ir_tdi_data = 0x0E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	printf("Bypass : %02x [0x00] , ",tdo & 0xff);

	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8	TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer(0, 8, &data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//! Verify the Flash
	syslog(LOG_WARNING,"Starting to Verify Device . . . This will take a few seconds\n");

	//! Shift in LSC_INIT_ADDRESS(0x46) instruction
	//SIR 8	TDI  (46);
	*ir_tdi_data = 0x46;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (04);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x04;
	ast_jtag_tdi_xfer(0, 8, &data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR 8	TDI  (73);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x73;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	fseek(jed_fd, 0, SEEK_SET);
	jed_file_paser_header(jed_fd);

	syslog(LOG_WARNING,"Verify CONFIG 9192 \n");
	cmp_err = 0;
	row = 0;

	for (row = 0 ; row < cur_dev->row_num; row++) {
//		printf("%d \n", row);
		memset(dr_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		memset(jed_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, jed_data);
		ast_jtag_tdo_xfer(0, cur_dev->dr_bits, dr_data);

#if 0
		for (i = 0; i < (cur_dev->dr_bits / 32); i++) {
			if (dr_data[i] != jed_data[i]) {
				printf("JED : %x, SDR : %x \n", jed_data[i], dr_data[i]);
				cmp_err = 1;
			}
		}
#else
		for (i = 0; i < (cur_dev->dr_bits / 32); i++) {
			if (dr_data[i] != jed_data[i]) {
				syslog(LOG_WARNING,"JED : %x, SDR : %x \n", jed_data[i], dr_data[i]);
				cmp_err = 1;
			}
		}

#endif

		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
//		ast_jtag_run_test_idle( 0, 0, 2);
		usleep(3000);
//		printf("\n");
		if (cmp_err) {
			goto cmp_error;
			break;
		}
	}

	//! Verify the UFM


	//! Shift in LSC_INIT_ADDR_UFM(0x47) instruction
	//SIR	8	TDI  (47);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x47;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);


	//! Shift in LSC_READ_INCR_NV(0x73) instruction
	//SIR	8	TDI  (73);
	//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x73;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	syslog(LOG_WARNING,"Verify the UFM 2048 \n");
	//! Shift out Data Row
	for (row = 0 ; row < 2048; row++) {
//		printf("%d \n", row);
		memset(dr_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		memset(jed_data, 0, (cur_dev->dr_bits / 32) * sizeof(unsigned int));
		jed_file_paser(jed_fd, cur_dev->dr_bits, jed_data);
		ast_jtag_tdo_xfer(0, cur_dev->dr_bits, dr_data);

		for (i = 0; i < (cur_dev->dr_bits / 32); i++) {
			if (dr_data[i] != jed_data[i]) {
				syslog(LOG_WARNING,"JED : %x, SDR : %x \n", jed_data[i], dr_data[i]);
				cmp_err = 1;
			}
		}
		//RUNTEST	IDLE	2 TCK	1.00E-003 SEC;
//		ast_jtag_run_test_idle( 0, 0, 2);
		usleep(3000);
//		printf("\n");
		if (cmp_err) {
			break;
		}
	}

	//! Verify USERCODE
	syslog(LOG_WARNING,"Verify USERCODE \n");
	//! Shift in READ USERCODE(0xC0) instruction
	//SIR 8	TDI  (C0);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0xC0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (FFFFFFFF);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Verify Feature Rows

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Shift in LSC_READ_FEATURE (0xE7) instruction
	//SIR 8	TDI  (E7);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0xE7;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);

	usleep(3000);

	//SDR 64	TDI  (0000000000000000)
	//		TDO  (0000000000000000);
	dr_data[0] = 0x00000000;
	dr_data[1] = 0x00000000;
	ast_jtag_tdo_xfer(0, 64, dr_data);

	//! Shift in in LSC_READ_FEABITS(0xFB) instruction
	//SIR 8	TDI  (FB);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0xFB;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//SDR 16	TDI  (0000)
	//		TDO  (0620)
	//		MASK (FFF2);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 16, dr_data);
	syslog(LOG_WARNING,"read %x [0x0620] \n", dr_data[0] & 0xffff);

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	dr_data[0] = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, dr_data);

	//! Verify Done Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (C4);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	if((tdo & 0xff) != 0x04)
//		printf("BYPASS error %x \n", tdo & 0xff);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	*ir_tdi_data = 0x26;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	ast_jtag_run_test_idle( 0, 0, 2);
	usleep(1000);

	//! Verify SRAM DONE Bit

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (04)
	//		MASK (84);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
//	if((tdo & 0xff) != 0x04)
//		printf("BYPASS error %x \n", tdo & 0xff);


cmp_error:
	free(jed_data);
	free(dr_data);

	if (cmp_err)
		syslog(LOG_WARNING,"Verify Error !!\n");
	else
		syslog(LOG_WARNING,"Verify Done !!\n");

	return 0;

}

int lcmxo2_70000he_cpld_erase(void)
{
	int i = 0;
	unsigned int *sdr_data;
	unsigned int *ir_tdi_data;
	unsigned int *ir_tdo_data;
	unsigned int data = 0;
	unsigned int sdr_array = 0;

	ir_tdi_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));
	ir_tdo_data = static_cast<unsigned int *> (malloc(sizeof(unsigned int)));

	//RUNTEST	IDLE	15 TCK	1.00E-003 SEC;
	ast_jtag_run_test_idle(0, 0, 3);

	//! Check the IDCODE_PUB
	//SIR	8	TDI  (E0);
	*ir_tdi_data = IDCODE_PUB;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 32	TDI  (00000000)
	//		TDO  (01285043)
	//		MASK (FFFFFFFF);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	if (data != 0x12B5043) {
		syslog(LOG_WARNING,"ID Fail : %08x [0x012B5043] \n", data);
		return -1;
	}

	//! Program Bscan register

	//! Shift in Preload(0x1C) instruction
	//SIR	8	TDI  (1C);
	*ir_tdi_data = PRELOAD;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR	664 TDI  (FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//		 FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	//		 FFFFFF);
	sdr_array = 664 / 32 + 1;
	sdr_data = static_cast<unsigned int *> (malloc(sdr_array * sizeof(unsigned int)));
	memset(sdr_data, 0xff, sdr_array * sizeof(unsigned int));
	ast_jtag_tdi_xfer(0, 664, sdr_data);
	free(sdr_data);

	//! Enable the Flash
	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8	TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (00);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x00;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	*ir_tdi_data = 0x0E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (01);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	data = 0x01;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(1000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF)
	//		TDO  (00)
	//		MASK (C0);
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//! Shift in ISC ENABLE(0xC6) instruction
	//SIR 8	TDI  (C6);
	*ir_tdi_data = 0xC6;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (08);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	data = 0x08;
	ast_jtag_tdi_xfer(0, 8, &data);
	usleep(3000);

	//! Check the Key Protection fuses

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00024040);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00010000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Erase the Flash

	//! Shift in ISC ERASE(0x0E) instruction
	//SIR 8	TDI  (0E);
	*ir_tdi_data = 0x0E;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//SDR 8	TDI  (0E);
	//RUNTEST IDLE	2 TCK;
	data = 0x0E;
	ast_jtag_tdi_xfer(0, 8, &data);

	//! Shift in LSC_CHECK_BUSY(0xF0) instruction
	//SIR 8	TDI  (F0);
	*ir_tdi_data = 0xF0;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	//LOOP 800 ;
	//RUNTEST IDLE	2 TCK	1.00E-002 SEC;
	//SDR 1	TDI  (0)
	//		TDO  (0);
	//ENDLOOP ;
	syslog(LOG_WARNING,"LOOP 800  \n");
	data = 0;
	for (i = 0; i < 800 ; i++) {
//		printf("loop count %d \n",i);
//		ast_jtag_run_test_idle( 0, 0, 2);
		usleep(2000);
		ast_jtag_tdo_xfer(0, 1, &data);
	}

	//! Read the status bit

	//! Shift in LSC_READ_STATUS(0x3C) instruction
	//SIR 8	TDI  (3C);
	//RUNTEST IDLE	2 TCK	1.00E-003 SEC;
	*ir_tdi_data = 0x3C;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//SDR 32	TDI  (00000000)
	//		TDO  (00000000)
	//		MASK (00003000);
	data = 0x00000000;
	ast_jtag_tdo_xfer(0, 32, &data);

	//! Exit the programming mode

	//! Shift in ISC DISABLE(0x26) instruction
	//SIR 8	TDI  (26);
	//RUNTEST IDLE	2 TCK	1.00E+000 SEC;
	*ir_tdi_data = 0x26;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);
	usleep(3000);

	//! Shift in BYPASS(0xFF) instruction
	//SIR 8	TDI  (FF);
	//RUNTEST IDLE	2 TCK	1.00E-001 SEC;
	*ir_tdi_data = BYPASS;
	ast_jtag_sir_xfer(0, LATTICE_INS_LENGTH, ir_tdi_data, ir_tdo_data);

	usleep(1000);

	syslog(LOG_WARNING,"Done \n");

	return 0;

}

