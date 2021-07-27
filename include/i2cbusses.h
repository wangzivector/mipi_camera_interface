#ifndef _I2CBUSSES_H
#define _I2CBUSSES_H

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>	/* for NAME_MAX */
#include <strings.h>	/* for strcasecmp() */
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/spi/spidev.h>
#include <time.h>

#include <pthread.h>

enum parse_state {
	PARSE_GET_DESC,
	PARSE_GET_DATA,
};

enum i2c_dir {
	I2C_WRITE_MODE,
	I2C_READ_MODE,
};

enum State_Com {
	STAND_BY,
	WORKING,
	FINISHED
};

struct reg {
    unsigned short int address;
    unsigned short int value;
};

struct R6fromA1 {
    unsigned char reg;
    short data[3];
    unsigned int seq;
};

struct sync_index
{
    long index_video;
    struct timespec ts_video;

    long index_trigger;
    struct timespec ts_trigger;

    unsigned int imgts_count;
    unsigned int imgts_count_wrong;
    unsigned int img_count;
    unsigned char SPI_enable;
    
    enum State_Com state;
} sync_index;


extern struct sync_index sync_obj;

#define PRINT_STDERR	(1 << 0)
#define PRINT_READ_BUF	(1 << 1)
#define PRINT_WRITE_BUF	(1 << 2)
#define PRINT_HEADER	(1 << 3)

// i2c
int open_i2c_dev(int i2cbus, char *filename, size_t size, int quiet);
int set_slave_addr(int file, int address, int force);
int i2cReadWrite(int i2cbus, unsigned char bus_address, struct reg *regs, int n_reg, enum i2c_dir dir);
int i2cRead6FromAdr1(int i2cbus, unsigned char bus_address, struct R6fromA1 read_msg);

//spi
int SPI_Close(void);
int SPI_Init(void);
void SPI_transfer(unsigned int *index);
void *Mlt_SPI_transfer(void *none);



#define MISSING_FUNC_FMT	"Error: Adapter does not have %s capability\n"

#endif
