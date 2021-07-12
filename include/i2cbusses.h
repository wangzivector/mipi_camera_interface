#ifndef _I2CBUSSES_H
#define _I2CBUSSES_H

#include <unistd.h>
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>


enum parse_state {
	PARSE_GET_DESC,
	PARSE_GET_DATA,
};

enum i2c_dir {
	I2C_WRITE_MODE,
	I2C_READ_MODE,
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



#define PRINT_STDERR	(1 << 0)
#define PRINT_READ_BUF	(1 << 1)
#define PRINT_WRITE_BUF	(1 << 2)
#define PRINT_HEADER	(1 << 3)


int open_i2c_dev(int i2cbus, char *filename, size_t size, int quiet);
int set_slave_addr(int file, int address, int force);
int i2cReadWrite(int i2cbus, unsigned char bus_address, struct reg regs[], int n_reg, enum i2c_dir dir);
int i2cRead6FromAdr1(int i2cbus, unsigned char bus_address, struct R6fromA1 read_msg);


#define MISSING_FUNC_FMT	"Error: Adapter does not have %s capability\n"

#endif
