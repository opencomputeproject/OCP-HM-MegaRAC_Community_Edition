#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <mqueue.h>
#include <semaphore.h>
#include <poll.h>
#include "ipmbd.h"
#include "i2c.h"


#define DEBUG 0

#define MAX_BYTES 300
#define LUN_OFFSET 2

#define MQ_IPMB_REQ "/mq_ipmb_req"
#define MQ_IPMB_RES "/mq_ipmb_res"
#define MQ_MAX_MSG_SIZE MAX_BYTES
#define MQ_MAX_NUM_MSGS 256

#define SEQ_NUM_MAX 64

#define I2C_RETRIES_MAX 15

#define IPMB_PKT_MIN_SIZE 6

#define WRITE_TIMEOUT       1
#define HHM_FAILURE                 ( -1 )
ssize_t i2c_master_write_on_fd(int ,unsigned char ,unsigned char *,size_t  );

ssize_t internal_master_write( int ,unsigned char ,unsigned char *,size_t ,bool  );

int poll_ipmb(uint8_t bus_num);


// Structure for sequence number and buffer
typedef struct _seq_buf_t {
  bool in_use; // seq# is being used
  uint8_t len; // buffer size
  uint8_t *p_buf; // pointer to buffer
  sem_t s_seq; // semaphore for thread sync.
} seq_buf_t;

// Structure for holding currently used sequence number and
// array of all possible sequence number
typedef struct _ipmb_sbuf_t {
  uint8_t curr_seq; // currently used seq#
  seq_buf_t seq[SEQ_NUM_MAX]; //array of all possible seq# struct.
} ipmb_sbuf_t;

// Global storage for holding IPMB sequence number and buffer
ipmb_sbuf_t g_seq;

// mutex to protect global data access
pthread_mutex_t m_seq;

pthread_mutex_t m_i2c;

static int g_bus_id = 0; // store the i2c bus ID for debug print


static int
i2c_open(uint8_t bus_num) {
  int fd;
  char fn[32];
  int rc;

syslog(LOG_WARNING,"inside i2c_opne\n");

  snprintf(fn, sizeof(fn), "/dev/i2c-%d", bus_num);
  fd = open(fn, O_RDWR);
  if (fd == -1) {
    syslog(LOG_WARNING, "Failed to open i2c device %s", fn);
    return -1;
  }

  return fd;
}


int poll_ipmb(uint8_t bus_num){
	struct pollfd fds;
	struct timespec ts;
	unsigned char buffer[100];
	int ret, i,fd=0,count=0;
	char mq_ipmb_res[64] = {0};
	unsigned char Err = 0;
	mqd_t mq = (mqd_t)-1;
	
	fds.fd = open("/sys/bus/i2c/devices/4-1020/slave-mqueue", O_RDONLY  | O_NONBLOCK);
	if(fds.fd < 0)
	{
		syslog(LOG_WARNING,"iniside poll_ipmb  ( ) - %x %s\n",errno, strerror(errno));
		return -1;
	}
	
	fds.events = POLLPRI;

	while(1) {
                ret = poll(&fds, 1, 5000);
		if(count >= 1)
			break;
		if (ret == 0 || !(fds.revents & POLLPRI)){
			count ++;
			continue;
		}
		lseek(fds.fd, 0 , SEEK_SET); 
		ret = read(fds.fd, buffer, sizeof(buffer));
		if(ret != 0)
		{
			#if DEBUG
	                for (i = 0; i < ret; i++){
                        	syslog(LOG_WARNING,"%02x",buffer[i]);
	                	syslog(LOG_WARNING,"\n");}
			#endif
			break;
		}	
                if(ret <= 0)
			continue;
		
		clock_gettime(CLOCK_MONOTONIC, &ts);
		syslog(LOG_WARNING,"[%ld.%.9ld] :", ts.tv_sec, ts.tv_nsec);
		for (i = 0; i < ret; i++)
			syslog(LOG_WARNING,"%02x",buffer[i]);
		syslog(LOG_WARNING,"\n");
		
                }
	fd = i2c_open(bus_num);
	if (fd < 0) {
		syslog(LOG_WARNING, "i2c_open failure\n");
    		return NULL;
  	}

	sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, bus_num);
	
	// Open Queue to send response
	mq = mq_open(mq_ipmb_res, O_WRONLY);
	if (mq == (mqd_t) -1) {
		return NULL;
	}

        #if DEBUG
        for (i = 0; i < ret; i++)
                syslog(LOG_WARNING,"%02x",buffer[i]);
        #endif

	Err =  mq_send(mq,buffer, ret , 1);
	syslog(LOG_WARNING,"Err: %d\n",Err);
	if ((Err == -1))
	{
		syslog(LOG_WARNING,"Message.c : PostMsg ( %s ) - %x %s\n",mq_ipmb_res , errno, strerror(errno));
		return -1;
	}
	mq_close(mq);	
	close(fds.fd);
	return 0;
}


/****************************************************** 
 *   Description: Function to handle IPMB request     *
 *   ipmb_req_handler: Thread to handle new requests  *
 *   bus_num	: Bus number			      *
 ******************************************************/
static void*
ipmb_req_handler(void *bus_num) {
  uint8_t *bnum = (uint8_t*) bus_num;
  mqd_t mq;
  int fd = 0;
  int i = 0;

  //Buffers for IPMB transport
  uint8_t rxbuf[MQ_MAX_MSG_SIZE] = {0};
  ipmb_req_t *p_ipmb_req;

  p_ipmb_req = (ipmb_req_t*) rxbuf;


  uint8_t rlen = 0;

  char mq_ipmb_req[64] = {0};

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, *bnum);

  // Open Queue to receive requests
  mq = mq_open(mq_ipmb_req, O_RDONLY);
  if (mq == (mqd_t) -1) {
    return NULL;
  }

  // Open the i2c bus for sending response
  fd = i2c_open(*bnum);
  if (fd < 0) {
    syslog(LOG_WARNING, "i2c_open failure\n");
    mq_close(mq);
    return NULL;
  }

  // Loop to process incoming requests
  while (1) {
    if ((rlen = mq_receive(mq, (char *)rxbuf, MQ_MAX_MSG_SIZE, NULL)) <= 0) {
      continue;
    }

#if DEBUG
    syslog(LOG_WARNING, "Received Request of %d bytes\n", rlen);
    for (i = 0; i < rlen; i++) {
      syslog(LOG_WARNING, "0x%X", rxbuf[i]);
    }
#endif

	int retval = 0;
	retval = i2c_master_write_on_fd(fd,rxbuf[0] >> 1 ,&rxbuf[1],rlen-1); 	
	if((retval != 0))
		syslog(LOG_WARNING,"errno:%x\t strerror(errno):%s \n",errno, strerror(errno));

	retval = poll_ipmb(*bnum);
	if(retval != 0)
		syslog(LOG_WARNING,"Error Return Value for poll_ipmb :%d\n",retval);

  }
}


ssize_t i2c_master_write_on_fd( int i2cfd,unsigned char  slave,unsigned char *data, size_t count )
{
    /* Pass an actual in-use file descriptor to internal master write */
    return( internal_master_write( i2cfd, slave, data, count, true ) );
}

ssize_t internal_master_write( int i2cfd,unsigned char slave,unsigned char  *data,
                                      size_t count, bool do_wait )
{
    ssize_t ret = -1;
    int i =0;

    /* Check for bogus fd */
    if( i2cfd < 0 ){
	syslog(LOG_WARNING,"failed to open\n");
        return( HHM_FAILURE );}

#if DEBUG 
    syslog(LOG_WARNING, "Inside internal master write Sending Response of %d bytes\n");
    for (i = 0; i < count; i++) {
      syslog(LOG_WARNING, "0x%X:", data[i]);
    }
#endif

    /* Set the remote slave to which we'll be writing */
    if( ioctl( i2cfd, I2C_SLAVE, slave ) < 0 )
    {
        syslog(LOG_WARNING,"Cannot set remote slave device for master write\n" );
        return( HHM_FAILURE );
    }

    /* Write the specified data onto the bus */
    ret = write( i2cfd, data, count );

    if((ret == -1))
	syslog(LOG_WARNING,"inside writeerrno:%x\t strerror(errno):%s \n",errno, strerror(errno));

    syslog(LOG_WARNING,"ret value :%d \t count value:%d\n",ret,count);	
    if( (size_t)ret != count )
    {
	syslog(LOG_WARNING,"inside error count\n");
        errno = EREMOTEIO;
        /*@=unrecog@*/
    }
    
    return( ret );
}



int
main(int argc, char * const argv[]) {
  pthread_t tid_req_handler;
  uint8_t ipmb_bus_num;
  mqd_t mqd_req = (mqd_t)-1, mqd_res = (mqd_t)-1;
  struct mq_attr attr;
  char mq_ipmb_req[64] = {0};
  char mq_ipmb_res[64] = {0};
  int rc = 0;

  if (argc < 2) {
    syslog(LOG_WARNING, "ipmbd: Usage: ipmbd <bus#> ");
    exit(1);
  }

  ipmb_bus_num = (uint8_t)strtoul(argv[1], NULL, 0);
  g_bus_id = ipmb_bus_num;


  syslog(LOG_WARNING, "ipmbd: bus#:%d \n", ipmb_bus_num);

  pthread_mutex_init(&m_i2c, NULL);

  // Create Message Queues for Request Messages and Response Messages
  attr.mq_flags = 0;
  attr.mq_maxmsg = MQ_MAX_NUM_MSGS;
  attr.mq_msgsize = MQ_MAX_MSG_SIZE;
  attr.mq_curmsgs = 0;

  sprintf(mq_ipmb_req, "%s_%d", MQ_IPMB_REQ, ipmb_bus_num);
  sprintf(mq_ipmb_res, "%s_%d", MQ_IPMB_RES, ipmb_bus_num);

  // Remove the MQ if exists
  mq_unlink(mq_ipmb_req);

//  mqd_req = mq_open(mq_ipmb_req, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  mqd_req = mq_open(mq_ipmb_req, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_req == (mqd_t) -1) {
    rc = errno;
    goto cleanup;
  }

  // Remove the MQ if exists
  mq_unlink(mq_ipmb_res);

  mqd_res = mq_open(mq_ipmb_res, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, &attr);
  if (mqd_res == (mqd_t) -1) {
    rc = errno;
    goto cleanup;
  }

  // Create thread to handle IPMB Requests
  if (pthread_create(&tid_req_handler, NULL, ipmb_req_handler, (void*) &ipmb_bus_num) < 0) {
    goto cleanup;
  }


cleanup:
  if (tid_req_handler > 0) {
    pthread_join(tid_req_handler, NULL);
  }


  if (mqd_res > 0) {
    mq_close(mqd_res);
    mq_unlink(mq_ipmb_res);
  }

  if (mqd_req > 0) {
    mq_close(mqd_req);
    mq_unlink(mq_ipmb_req);
  }

  pthread_mutex_destroy(&m_i2c);

  return 0;
}
