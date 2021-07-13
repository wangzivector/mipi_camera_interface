/*
    i2cbusses: Print the installed i2c busses for both 2.4 and 2.6 kernels.
               Part of user-space programs to access for I2C
               devices.
    Copyright (c) 1999-2003  Frodo Looijaard <frodol@dds.nl> and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2008-2012  Jean Delvare <jdelvare@suse.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

/* For strdup and snprintf */
#define _BSD_SOURCE 1 /* for glibc <= 2.19 */
#define _DEFAULT_SOURCE 1 /* for glibc >= 2.19 */

#include "i2cbusses.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static uint8_t bits = 8;
static uint32_t speed = 500000;
static int fd_spi;

struct msg_header{
	unsigned char data_type;
	unsigned char data_size;
	unsigned int  data_seq; 
}msg_header;

static void pabort(const char *s)
{
	perror(s);
	abort();
}

int decode_spimsg_header(char *buff_rx, struct msg_header *header)
{
	char char_data;
	unsigned char data_type, data_size = 0;
	unsigned short data_seq_bin = 0;
	if (buff_rx[0] != 0xAA)
		return -1;
	// printf("\n%x , %x , %x , %x\n",buff_rx[0], buff_rx[1], buff_rx[2], buff_rx[3]);
	header->data_size = buff_rx[1] & 0x0F;
	header->data_type = (buff_rx[1] & 0xF0) >> 4;
	data_seq_bin = buff_rx[2];
	header->data_seq = (data_seq_bin << 8) | buff_rx[3];
	return header->data_size;
}

void SPI_transfer(int index)
{
	int ret, i;
	unsigned short index_short = index;
	uint8_t tx[] = {0xAA, 0xAA, (index_short & 0xFF00) >> 8, (index_short & 0x00FF)};
	struct msg_header header_msg;


	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	uint8_t rx_tm[ARRAY_SIZE(tx)] = {0, };
	uint8_t tx_tm[] = {0xBB, 0xBB, (index_short & 0xFF00) >> 8, (index_short & 0x00FF)};
	struct spi_ioc_transfer tr_tm = {
		.tx_buf = (unsigned long)tx_tm,
		.rx_buf = (unsigned long)rx_tm,
		.len = ARRAY_SIZE(tx_tm),
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		pabort("can't send spi message");
	printf("transfer buffer finished: \n");
	for (ret = 0; ret < ARRAY_SIZE(tx); ret++) {
		printf("%.2X ", rx[ret]);
	}
	
	if (-1 == decode_spimsg_header(rx, &header_msg))
		printf("fail decode spi message");
	else{
		printf("DATA type:%d , seqe:%d , size:%d \n", header_msg.data_type, header_msg.data_seq, header_msg.data_size);
		for(i = 0; i < header_msg.data_size; i+=4)
		{
			// tr.tx_buf = (unsigned long)(0xAA);
			// rx[0] = (unsigned long)(0x00);
			// tr.rx_buf = (unsigned long)rx;
			ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &tr_tm);
			if (ret < 1)
				printf("can not receive session %d receive msg %d .\n", index, i);
			else
				printf("session %d receive msg %d .\n", index, i);

			for (ret = 0; ret < ARRAY_SIZE(tx_tm); ret++) {
				printf("%X", rx_tm[ret]);
			}
		}
	}
	printf("\n transfer session done!\n");
}


int SPI_Init(void)
{
	static const char *device = "/dev/spidev0.0";
	static uint8_t mode;
	int ret;
	
	fd_spi = open(device, O_RDWR);
	if (fd_spi < 0)
		pabort("can't open device");

	/*
	 * spi mode
	 */
	ret = ioctl(fd_spi, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd_spi, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	 * bits per word
	 */
	ret = ioctl(fd_spi, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd_spi, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	 * max speed hz
	 */
	ret = ioctl(fd_spi, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd_spi, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);

	// SPI_transfer();
	
}

int SPI_Close(void)
{
	close(fd_spi);
}




////////
////////  IIC
////////


int open_i2c_dev(int i2cbus, char *filename, size_t size, int quiet)
{
	int file, len;

	len = snprintf(filename, size, "/dev/i2c/%d", i2cbus);
	if (len >= (int)size) {
		fprintf(stderr, "%s: path truncated\n", filename);
		return -EOVERFLOW;
	}
	file = open(filename, O_RDWR);

	if (file < 0 && (errno == ENOENT || errno == ENOTDIR)) {
		len = snprintf(filename, size, "/dev/i2c-%d", i2cbus);
		if (len >= (int)size) {
			fprintf(stderr, "%s: path truncated\n", filename);
			return -EOVERFLOW;
		}
		file = open(filename, O_RDWR);
	}

	if (file < 0 && !quiet) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"`/dev/i2c-%d' or `/dev/i2c/%d': %s\n",
				i2cbus, i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
	}

	return file;
}

int set_slave_addr(int file, int address, int force)
{
	/* With force, let the user read from/write to the registers
	   even when a driver is also running */
	if (ioctl(file, force ? I2C_SLAVE_FORCE : I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		return -errno;
	}

	return 0;
}

static int check_funcs(int file)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	if (!(funcs & I2C_FUNC_I2C)) {
		fprintf(stderr, MISSING_FUNC_FMT, "I2C transfers");
		return -1;
	}

	return 0;
}

static void print_msgs(struct i2c_msg *msgs, __u32 nmsgs, unsigned flags)
{
	FILE *output = flags & PRINT_STDERR ? stderr : stdout;
	unsigned i;
	__u16 j;

	for (i = 0; i < nmsgs; i++) {
		int read = msgs[i].flags & I2C_M_RD;
		int recv_len = msgs[i].flags & I2C_M_RECV_LEN;
		int print_buf = (read && (flags & PRINT_READ_BUF)) ||
				(!read && (flags & PRINT_WRITE_BUF));
		__u16 len = msgs[i].len;

		if (recv_len && print_buf && len != msgs[i].buf[0] + 1) {
			fprintf(stderr, "Correcting wrong msg length after recv_len! Please fix the I2C driver and/or report.\n");
			len = msgs[i].buf[0] + 1;
		}

		if (flags & PRINT_HEADER) {
			fprintf(output, "msg %u: addr 0x%02x, %s, len ",
				i, msgs[i].addr, read ? "read" : "write");
			if (!recv_len || flags & PRINT_READ_BUF)
				fprintf(output, "%u", len);
			else
				fprintf(output, "TBD");
		}

		if (len && print_buf) {
			if (flags & PRINT_HEADER)
				fprintf(output, ", buf ");
			for (j = 0; j < len - 1; j++)
				fprintf(output, "0x%02x ", msgs[i].buf[j]);
			/* Print final byte with newline */
			fprintf(output, "0x%02x\n", msgs[i].buf[j]);
		} else if (flags & PRINT_HEADER) {
			fprintf(output, "\n");
		}
	}
}

int i2cRead6FromAdr1(int i2cbus, unsigned char bus_address, struct R6fromA1 read_msg)
{
	int i, j, nmsgs_sent, nmsgs;
	char filename[20];
	int file;
	struct i2c_msg msgs[2];
	unsigned char data;
	struct i2c_rdwr_ioctl_data rdwr;
	unsigned char *buf;
	unsigned short int  flags = 0;

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0); // open device, filename is wait2fill value
	if (file < 0 || check_funcs(file)) // check device is iic-related device 
		exit(1);

// reg address buffer
	msgs[0].addr = bus_address; // address, always the same
	msgs[0].flags = flags; // default writing mode
	msgs[0].len = 1;    // num of operating bytes
	buf = malloc(msgs[0].len); // create buffer for writer buffer
	if (!buf) {
		fprintf(stderr, "Error: No memory for buffer\n");
		goto err_out_with_arg;
	}
	memset(buf, 0, msgs[0].len);
	msgs[0].buf = buf; // clear the buffer (write) nall_msgs is the msg index
	data = read_msg.reg;
	msgs[0].buf[0] = data;


// read buffer
	msgs[1].addr = bus_address; // address, always the same
	msgs[1].flags = flags|I2C_M_RD; // default writing mode
	msgs[1].len = 6;    // num of operating bytes
	if (msgs[1].len) {
		buf = malloc(msgs[1].len); // create buffer for writer buffer
		if (!buf) {
			fprintf(stderr, "Error: No memory for buffer\n");
			goto err_out_with_arg;
		}
	}
	memset(buf, 0, msgs[1].len);
	msgs[1].buf = buf; // clear the buffer (write) nall_msgs is the msg index
	msgs[1].buf[0] = 0x00;

	nmsgs = 2;
	rdwr.msgs = msgs;
	rdwr.nmsgs = nmsgs;
	nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);

	if (nmsgs_sent < 0) {
		fprintf(stderr, "Error: Sending messages failed: %s\n", strerror(errno));
		goto err_out;
	} 

	print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (1 ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
	
	for (i = 0; i < 3; i++)
		read_msg.data[i] = (short)(msgs[1].buf[i*2 + 1]) << 8 + (msgs[1].buf[i*2]);

	close(file);

	for (i = 0; i < 2; i++)
		free(msgs[i].buf);

	return 0;

 err_out_with_arg:
	fprintf(stderr, "Error: faulty ");
 err_out:
	close(file);
	for (i = 0; i <= nmsgs; i++)
		free(msgs[i].buf);
	exit(1);
}

int i2cReadWrite(int i2cbus, unsigned char bus_address, struct reg *regs, int n_reg, enum i2c_dir dir)
{
	int i, j;
	char filename[20];
	int file;
	int nmsgs_sent, size_allmsgs, nmsgs = 0;
	unsigned short int data, flags = 0;
	unsigned char *buf;

	struct i2c_msg msgs[2], all_msgs[I2C_RDRW_IOCTL_MAX_MSGS]; // create sending messsage
	// enum parse_state state = PARSE_GET_DESC; // sending mode

	for (i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++) // clean msgs buffer
		all_msgs[i].buf = NULL;

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0); // open device
	if (file < 0 || check_funcs(file)) // check device is iic-related device 
		exit(1);
	
	// n_reg = sizeof(regs)/sizeof(struct reg);
	printf("   num of regs to read: %d\n", n_reg);

	// read buffer setting
	for(i = 0; i < n_reg; i ++)
	{
		switch (dir){
			case I2C_READ_MODE :
				size_allmsgs = 2*n_reg;
				all_msgs[i*2].addr = bus_address; // address, always the same
				all_msgs[i*2].flags = flags; // default writing mode
				all_msgs[i*2].len = 2;    // num of operating bytes
				if (all_msgs[i*2].len) {
					buf = malloc(all_msgs[i*2].len); // create buffer for writer buffer
					if (!buf) {
						fprintf(stderr, "Error: No memory for buffer\n");
						goto err_out_with_arg;
					}
				}
				memset(buf, 0, all_msgs[i*2].len);
				all_msgs[i*2].buf = buf; // clear the buffer (write) nall_msgs is the msg index
				data = regs[i].address;
				all_msgs[i*2].buf[0] = data >> 8;
				all_msgs[i*2].buf[1] = data & 0x00ff;

				all_msgs[i*2 + 1].addr = bus_address; // address, always the same
				all_msgs[i*2 + 1].flags = flags|I2C_M_RD;; // default writing mode
				all_msgs[i*2 + 1].len = 1;    // num of operating bytes
				if (all_msgs[i*2 + 1].len) {
					buf = malloc(all_msgs[i*2 + 1].len); // create buffer for writer buffer
					if (!buf) {
						fprintf(stderr, "Error: No memory for buffer\n");
						goto err_out_with_arg;
					}
				}
				memset(buf, 0, all_msgs[i*2 + 1].len);
				all_msgs[i*2 + 1].buf = buf; // clear the buffer (write) nall_msgs is the msg index
				all_msgs[i*2 + 1].buf[0] = 0x00;
				break;

			case I2C_WRITE_MODE :
				size_allmsgs = n_reg;
				all_msgs[i].addr = bus_address; // address, always the same
				all_msgs[i].flags = flags; // default writing mode
				all_msgs[i].len = 3;    // num of operating bytes
				if (all_msgs[i].len) {
					buf = malloc(all_msgs[i].len); // create buffer for writer buffer
					if (!buf) {
						fprintf(stderr, "Error: No memory for buffer\n");
						goto err_out_with_arg;
					}
				}
				memset(buf, 0, all_msgs[i].len);
				all_msgs[i].buf = buf; // clear the buffer (write) nall_msgs is the msg index
				data = regs[i].address;
				all_msgs[i].buf[0] = data >> 8;
				all_msgs[i].buf[1] = data & 0x00ff;
				data = regs[i].value;
				all_msgs[i].buf[2] = data;
			break;

			default :
				fprintf(stderr, "Error: wrong mode %s\n");
				goto err_out;
		}
	}

	struct i2c_rdwr_ioctl_data rdwr;

	/* Ensure address is not busy */
	// this line should be put in ahead===================...
	// if (!force && set_slave_addr(file, bus_address, 0)) // this is important for iic, slave force mode
	// 	goto err_out_with_arg;

	// for(i = 0; i< size_allmsgs; i++ ){
	// 	printf("\n ------- %d msg : ", i);
	// 	printf("addr: 0x%02x   ", all_msgs[i].addr);
	// 	printf("flags: %s   ", all_msgs[i].flags ? "read": "write");
	// 	printf("buflen: %d   ", all_msgs[i].len);
	// 	for(j = 0; j< all_msgs[i].len; j ++)
	// 		printf("| 0x%2x ", all_msgs[i].buf[j]);
	// }
	// printf("\n\n");

	switch (dir) {
		case I2C_READ_MODE :
			for(j = 0; j < n_reg; j++)
			{
				msgs[0] = all_msgs[j*2];
				msgs[1] = all_msgs[j*2 + 1];
				nmsgs = 2;
				rdwr.msgs = msgs;
				rdwr.nmsgs = nmsgs;
				nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);

				if (nmsgs_sent < 0) {
					fprintf(stderr, "Error: Sending messages failed: %s\n", strerror(errno));
					goto err_out;
				} 
				print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (1 ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
			}
			break;

		case I2C_WRITE_MODE :
		for(j = 0; j < n_reg; j++)
			{
				msgs[0] = all_msgs[j];
				nmsgs = 1;
				rdwr.msgs = msgs;
				rdwr.nmsgs = nmsgs;
				nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);

				if (nmsgs_sent < 0) {
					fprintf(stderr, "Error: Sending messages failed: %s\n", strerror(errno));
					goto err_out;
				} 
				print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (1 ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
			}
		break;

		default :
			fprintf(stderr, "Error: wrong mode %s\n");
			goto err_out;
	}

	close(file);

	for (i = 0; i < size_allmsgs; i++)
		free(all_msgs[i].buf);

	return 0;


 err_out_with_arg:
	fprintf(stderr, "Error: faulty ");
 err_out:
	close(file);

	for (i = 0; i <= nmsgs; i++)
		free(msgs[i].buf);

	exit(1);
}
