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

#include <sys/mman.h>
#include "lattice.hpp"
#include "ast-jtag.hpp"

extern xfer_mode mode;
int jtag_fd;

/*************************************************************************************/
/*				AST JTAG LIB					*/
int ast_jtag_open(char *dev)
{
	jtag_fd = open(dev, O_RDWR);
	if (jtag_fd == -1) {
		perror("Can't open /dev/aspeed-jtag, please install driver!! \n");
		return -1;
	}
	return 0;
}

void ast_jtag_close(void)
{
	close(jtag_fd);
}

unsigned int ast_get_jtag_freq(void)
{
	int retval;
	unsigned int freq = 0;
	retval = ioctl(jtag_fd, ASPEED_JTAG_GIOCFREQ, &freq);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return 0;
	}

	return freq;
}

int ast_set_jtag_freq(unsigned int freq)
{
	int retval;
	retval = ioctl(jtag_fd, ASPEED_JTAG_SIOCFREQ, freq);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return -1;
	}

	return 0;
}

int ast_jtag_run_test_idle(unsigned char reset, unsigned char end, unsigned char tck)

{
	int retval;
	struct runtest_idle run_idle;

	run_idle.mode = mode;
	run_idle.end = end;
	run_idle.reset = reset;
	run_idle.tck = tck;

	retval = ioctl(jtag_fd, ASPEED_JTAG_IOCRUNTEST, &run_idle);
	if (retval == -1) {
		perror("ioctl JTAG run reset fail!\n");
		return -1;
	}

//	if(end)
//		usleep(3000);

	return 0;
}

unsigned int ast_jtag_sir_xfer(unsigned char endir, unsigned int len, unsigned int *tdi, unsigned int *tdo)
{
	int 	retval;
	struct sir_xfer	sir;

	if (len > 32)
		return -1;

	sir.mode = mode;
	sir.length = len;
	sir.endir = endir;
	sir.tdo = tdo;
	sir.tdi = tdi;
	retval = ioctl(jtag_fd, ASPEED_JTAG_IOCSIR, &sir);
	if (retval == -1) {
		perror("ioctl JTAG sir fail!\n");
		return -1;
	}
//	if(endir)
//		usleep(3000);
	// return *sir.tdo;
	return 0;
}

int ast_jtag_tdi_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
	//write
	int retval;
	struct sdr_xfer sdr;

	sdr.mode = mode;

	sdr.direct = 1;
	sdr.enddr = enddr;
	sdr.length = len;
	sdr.tdio = tdio;

	retval = ioctl(jtag_fd, ASPEED_JTAG_IOCSDR, &sdr);
	if (retval == -1) {
		perror("ioctl JTAG data xfer fail!\n");
		return -1;
	}

//	if(enddr)
//		usleep(3000);
	return 0;
}

int ast_jtag_tdo_xfer(unsigned char enddr, unsigned int len, unsigned int *tdio)
{
	//read
	int retval;

	struct sdr_xfer sdr;

	sdr.mode = mode;

	sdr.direct = 0;
	sdr.enddr = enddr;
	sdr.length = len;
	sdr.tdio = tdio;

	retval = ioctl(jtag_fd, ASPEED_JTAG_IOCSDR, &sdr);
	if (retval == -1) {
		perror("ioctl JTAG data xfer fail!\n");
		return -1;
	}

//	if(enddr)
//		usleep(3000);
	return 0;
}

