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

#include <pigpio.h>
#include "i2cbusses.h"

#define SPI_PIN 5
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
static uint8_t bits = 8;
static uint32_t speed = 450000;
static int fd_spi;

FILE *fpWrite_txt;

struct msg_header{
	unsigned char data_type;
	unsigned char data_size;
	unsigned int  data_seq; 
}msg_header;

struct sync_index sync_obj;

inline static void pabort(const char *s)
{
	perror(s);
	abort();
}

int GPIO_init(unsigned char Pin_num,unsigned char mode)
{
	gpioSetMode(Pin_num, mode);
	return 0;
}

int GPIO_read(unsigned char Pin_num)
{
	return gpioRead(Pin_num);
}

int Start_file_txt(void)
{
	int state = 0;
	fpWrite_txt = fopen("VINS_log_data.txt", "w");
	printf("== START DATA == write state code: %d \n", fpWrite_txt);
	if(fpWrite_txt == NULL) return -1;
	
	fprintf(fpWrite_txt, "== START DATA ==\n");
	sync_obj.imgts_count = 0;
	// fclose(fpWrite_txt);
	return 0;
}

int Close_file_txt(void)
{
	usleep(1000*10);
	if(fpWrite_txt == NULL) return -1;
	fprintf(fpWrite_txt, "== END DATA ==\n");
	fclose(fpWrite_txt);

	printf("Closing file txt ... with image %d and imgts %d\n", sync_obj.img_count, sync_obj.imgts_count);
	return 0;
}

int write_image_header(unsigned short seq_num, unsigned int timestamp)
{
	if(fpWrite_txt == NULL) return -1;
	
	fprintf(fpWrite_txt, "%010d   IMG  nameofimagedata                              seqnum %05d\n",
	   timestamp, seq_num);
	return 0;
}

int write_imu_meas(unsigned short seq_num, unsigned int timestamp, short *data_arr)
{
	if(fpWrite_txt == NULL) return -1;
	
	fprintf(fpWrite_txt, "%010d   IMU  %06d %06d %06d   %06d %06d %06d  seqnum %05d\n", timestamp, 
	    data_arr[0], data_arr[1], data_arr[2], data_arr[3], data_arr[4], data_arr[5], seq_num);
	return 0;
}

int decode_msg(char *buff_rx)
{
	int i, type_size, incre_num = 0;
	int idex_imu = 0;
	void *link_data;
	char single_save_flag = 0; 
	unsigned short img_seq_num;
	unsigned int img_timestamp;
	unsigned short imu_seq_num;
	unsigned int imu_timestamp;
	short data_imu[6];

	if ((buff_rx[72+6] == 0x01)&&(buff_rx[73+6] == 0x02)&&(buff_rx[74+6] == 0x03)&&
		(buff_rx[75+6] == 0x04)&&(buff_rx[76+6] == 0x05)&&(buff_rx[77+6] == 0x06)) // for test
	{
		Start_file_txt();
		return 0;
	}

	if((buff_rx[72+6] == 0xfe)&&(buff_rx[73+6] == 0xdc)&&(buff_rx[74+6] == 0xba)&&
		(buff_rx[75+6] == 0x98)&&(buff_rx[76+6] == 0x76)&&(buff_rx[77+6] == 0x54)) // for debug you might take it as CRC
	{
		// image header 
		link_data = &img_seq_num; type_size = 2;       memcpy(link_data, (buff_rx + incre_num), type_size); incre_num += type_size;
		link_data = &img_timestamp; type_size = 4;     memcpy(link_data, (buff_rx + incre_num), type_size); incre_num += type_size;
		single_save_flag = 0;
		printf("++image timestamp :%d  seq:%d \n", img_timestamp, img_seq_num);	
		
		for(idex_imu = 0; idex_imu < 4; idex_imu ++)
		{
			// imu meas
			link_data = &imu_seq_num; type_size = 2;       memcpy(link_data, (buff_rx + incre_num), type_size); incre_num += type_size;
			link_data = &imu_timestamp; type_size = 4;     memcpy(link_data, (buff_rx + incre_num), type_size); incre_num += type_size;
			link_data = &data_imu; type_size = 12;     	   memcpy(link_data, (buff_rx + incre_num), type_size); incre_num += type_size;
			if (imu_timestamp != 0)
			{
				if ((img_timestamp < imu_timestamp) && (single_save_flag == 0))
				{	
					write_image_header(img_seq_num, img_timestamp);
					single_save_flag = 1;
				}
				
				write_imu_meas(imu_seq_num, imu_timestamp, data_imu);
				printf("--IMU timestamp :%d  seq:%d --IMU data: ", imu_timestamp, imu_seq_num);
				for(i = 0; i < 6; i++) printf("%d ", data_imu[i]);	printf("\n");
			}else break;
		}
		
		if(single_save_flag == 0)
			write_image_header(img_seq_num, img_timestamp);
		
		return idex_imu;
	}
	return -1;
}


void *Mlt_SPI_transfer(void *none)
{
	int ret, i, offset_index = 0;
	unsigned short index_short = 0;
	struct msg_header header_msg;

	uint8_t tx[84] = {0x11};
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };

	struct spi_ioc_transfer tr = {
		.tx_buf = (unsigned long)tx,
		.rx_buf = (unsigned long)rx,
		.len = ARRAY_SIZE(tx),
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};

	uint8_t tx_tm[] = {0xBB, 0xBB, (index_short & 0xFF00) >> 8, (index_short & 0x00FF)};
	uint8_t rx_tm[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr_tm = {
		.tx_buf = (unsigned long)tx_tm,
		.rx_buf = (unsigned long)rx_tm,
		.len = ARRAY_SIZE(tx_tm),
		.delay_usecs = 0,
		.speed_hz = speed,
		.bits_per_word = bits,
	};
	sync_obj.SPI_enable = 1;

	while(sync_obj.SPI_enable){

		// index_short = 0xCCDD;
		// tx[2] = (index_short & 0xFF00) >> 8;
		// tx[3] = (index_short & 0x00FF);
		// printf("GPIO_read(SPI_PIN)%d \n", GPIO_read(SPI_PIN));
		
		if(GPIO_read(SPI_PIN) == 1)
			ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &tr);
		else
		{
			usleep(100);
			continue;
		}
		
		if (ret < 1)
			pabort("can't send spi message");
		
		if (decode_msg(rx) > 0)
		{
			sync_obj.imgts_count ++;
			// continue;
		}

		printf("SPI %d transfer buffer finished: \n", sync_obj.imgts_count);
		for (ret = 0; ret < ARRAY_SIZE(tx); ret++)
		{
			printf("%.2X ", rx[ret]);
			if ((ret+1)%6 == 0) printf("\n");
		}

		// printf("DATA type:%d , seq:%d , size:%d \n", header_msg.data_type, header_msg.data_seq, header_msg.data_size);

		// if (header_msg.data_seq == index_short) printf("correct index:%d.\n", header_msg.data_seq);		
		// else printf("wrong index %d with %d!, match then and leave.\n", index_short = header_msg.data_seq, index_short);

		// index_short = header_msg.data_seq;
		// for(i = 0; i < header_msg.data_size; i+=4)
		// {
		// 	tx_tm[2] = (index_short & 0xFF00) >> 8;
		// 	tx_tm[3] = (index_short & 0x00FF);
		// 	// usleep(100);
		// 	ret = ioctl(fd_spi, SPI_IOC_MESSAGE(1), &tr_tm);
		// 	if (ret < 1)
		// 		printf("can not receive session %d receive msg %d .\n", index, i);
		// 	else
		// 		printf("session %d receive msg %d .\n", index, i);
			
		// 	printf("\n\n                                                                  ------ ");
		// 	for (ret = 0; ret < ARRAY_SIZE(tx_tm); ret++)
		// 		printf("%.2X ", rx_tm[ret]);
		// }
		// printf("\n");
		
		// // mark time and index, then compare with video index
		
		// if ((sync_obj.index_trigger + offset_index) == sync_obj.index_video)
		// {
		// 	printf("last video and trigger index %d match up! with time bias: video-trigger= %d ms.\n", sync_obj.index_video,
		// 		(sync_obj.ts_video.tv_sec- sync_obj.ts_trigger.tv_sec)*1000 + (sync_obj.ts_video.tv_nsec- sync_obj.ts_trigger.tv_nsec)/1000000);
		// }
		// else {
		// 	printf("mismatch video and trigger index! with index bias: video%d-trigger%d = %d, but offset_index is %d, bind them!\n",
		// 		sync_obj.index_video, sync_obj.index_trigger, sync_obj.index_video - sync_obj.index_trigger, offset_index);
		// 	// offset_index = sync_obj.index_video - sync_obj.index_trigger;
		// }

		// sync_obj.index_trigger = index_short;
		// clock_gettime(CLOCK_REALTIME, &sync_obj.ts_trigger);
		// printf("\n transfer session done!\n");
	} 
	return 0;
}



int decode_spimsg_header(char *buff_rx, struct msg_header *header)
{
	char char_data;
	unsigned char data_type, data_size = 0;
	unsigned short data_seq_bin = 0;
	int ret;
	// for (ret = 0; ret < 4; ret++) printf("%.2X ", buff_rx[ret]);
	// printf("//\n");
	if (buff_rx[0] != 0xAA) // for test
		return -1;
	// printf("\n%x , %x , %x , %x\n",buff_rx[0], buff_rx[1], buff_rx[2], buff_rx[3]);
	header->data_size = buff_rx[1] & 0x0F;
	header->data_type = (buff_rx[1] & 0xF0) >> 4;
	data_seq_bin = buff_rx[2];
	header->data_seq = (data_seq_bin << 8) | buff_rx[3];
	return header->data_size;
}

void SPI_transfer(unsigned int *index)
{
	int ret, i;
	unsigned short index_short = *index;
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

		if (header_msg.data_seq == *index)
		{
			printf("correct index:%d.\n", header_msg.data_seq);		
		}
		else{
			printf("wrong index !, match then and leave.\n");
			*index = header_msg.data_seq;
		}
		for(i = 0; i < header_msg.data_size; i+=4)
		{
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
	// Start_file_txt();
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

	printf("GPIO Init\n");

	if(gpioInitialise() < 0)
		pabort("wrong  gpioInitialise() !");
	// usleep(10000);
	GPIO_init(SPI_PIN, PI_INPUT);
	// SPI_transfer();
	
}

int SPI_Close(void)
{
	Close_file_txt();
	gpioTerminate();
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
