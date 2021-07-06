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


int i2c_main()
{
	
	char argv[][8] = { {"w2@0x60"}, {"0x30"}, {"0x30"}, {"r1"}};
	
    int argc = 4;
	char filename[20];
	int i2cbus = 11;
	int address = 0x60;
	int file;
	int arg_idx = 0;
	int nmsgs = 0;
	int nmsgs_sent;
	int i, j;

	int force = 1;
	int yes = 1;

	struct reg regs[] = {
		{0x4F00, 0x01},
		{0x3030, 0x04},
		{0x303F, 0x01},
		{0x302C, 0x00},
		{0x302F, 0x7F},
		{0x3823, 0x30},
		{0x0100, 0x00},
	};
	
	struct i2c_msg msgs[I2C_RDRW_IOCTL_MAX_MSGS]; // create sending messsage
	enum parse_state state = PARSE_GET_DESC; // sending mode
	unsigned buf_idx = 0;

	for (i = 0; i < I2C_RDRW_IOCTL_MAX_MSGS; i++) // clean msgs buffer
		msgs[i].buf = NULL;

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0); // open device
	if (file < 0 || check_funcs(file)) // check device is iic-related device 
		exit(1);

	while (arg_idx < argc) { // read every send mission
		char *arg_ptr = argv[arg_idx]; // take out the arg_idx_th string 
		unsigned long len, raw_data;
		unsigned short int flags;
		unsigned char data, *buf;
		char *end;
		if (nmsgs > I2C_RDRW_IOCTL_MAX_MSGS) { // check the number
			fprintf(stderr, "Error: Too many messages (max: %d)\n",
				I2C_RDRW_IOCTL_MAX_MSGS);
			goto err_out;
		}

		switch (state) { // to know w/r and its bus address
		case PARSE_GET_DESC:
			flags = 0;
			switch (*arg_ptr++) {
			case 'r': flags |= I2C_M_RD; break; // flag == read mode
			case 'w': break;					// flag != read mode
			default:
				fprintf(stderr, "Error: Invalid direction\n");
				goto err_out_with_arg;
			}

			len = strtoul(arg_ptr, &end, 0); // num of operating bytes
			if (len > 0xffff || arg_ptr == end) {
				fprintf(stderr, "Error: Length invalid\n");
				goto err_out_with_arg;
			}
			arg_ptr = end;
			arg_ptr++; // '@'

			/* Ensure address is not busy */
			// this line should be put in ahead===================...
			if (!force && set_slave_addr(file, address, 0)) // this is important for iic, slave force mode
				goto err_out_with_arg;

			msgs[nmsgs].addr = address; // address, always the same
			msgs[nmsgs].flags = flags; // default writing mode
			msgs[nmsgs].len = len;    // num of operating bytes

			if (len) {
				buf = malloc(len); // create buffer for writer buffer
				if (!buf) {
					fprintf(stderr, "Error: No memory for buffer\n");
					goto err_out_with_arg;
				}

				memset(buf, 0, len);
				msgs[nmsgs].buf = buf; // clear the buffer (write) nmsgs is the msg index

				// i2c_m_recv_len means the message is from the slave
				// we must ensure the buffer will be large enough to cope with 
				// a message length of I2C_SMBUS_BLOCK_MAX as this is the maximum underlying 
				// bus driver allow. the first byte in the buffer must be at least one to hold the 
				// message length, but can be greater. 
				/// wait to understand, I think can be erased.
				if (flags & I2C_M_RECV_LEN)
					buf[0] = 1; /* number of extra bytes */ // like the params  'r1'
			}

			if (flags & I2C_M_RD || len == 0) { // read mode,// or read mode and len is undefine
				nmsgs++; // if is read mode, which means the head of string is 'r', then turn to next buffer, why 
			} else { // write mode, 
				buf_idx = 0;
				state = PARSE_GET_DATA;
			}
			// return while loop, from here, you got the targeted data address.
			break;

		case PARSE_GET_DATA:
			raw_data = strtoul(arg_ptr, &end, 0); // get the data register
			if (raw_data > 0xff || arg_ptr == end) {
				fprintf(stderr, "Error: Invalid data byte\n");
				goto err_out_with_arg;
			}
			data = raw_data;
			len = msgs[nmsgs].len;

			while (buf_idx < len) {
				msgs[nmsgs].buf[buf_idx++] = data;

				if (!*end)
					break;

				switch (*end) {
				/* Pseudo randomness (8 bit AXR with a=13 and b=27) */
				case 'p':
					data = (data ^ 27) + 13;
					data = (data << 1) | (data >> 7);
					break;
				case '+': data++; break;
				case '-': data--; break;
				case '=': break;
				default:
					fprintf(stderr, "Error: Invalid data byte suffix\n");
					goto err_out_with_arg;
				}
			}

			if (buf_idx == len) {
				nmsgs++;
				state = PARSE_GET_DESC;
			}

			break;

		default:
			/* Should never happen */
			fprintf(stderr, "Internal Error: Unknown state in state machine!\n");
			goto err_out;
		}
		arg_idx++;
	}

	if (state != PARSE_GET_DESC || nmsgs == 0) {
		fprintf(stderr, "Error: Incomplete message\n");
		goto err_out;
	}

	if (yes) {
		struct i2c_rdwr_ioctl_data rdwr;

		for(i = 0; i< nmsgs; i++ ){
			printf("\n ------- %d msg : ", i);
			printf("addr:%4x   ", msgs[i].addr);
			printf("flags:%4x   ", msgs[i].flags);
			printf("buflen:%d   ", msgs[i].len);
			for(j = 0; j< msgs[i].len; j ++)
				printf("| 0x%2x ", msgs[i].buf[j]);
			printf("\n \n ");
		}


		rdwr.msgs = msgs;
		rdwr.nmsgs = nmsgs;
		nmsgs_sent = ioctl(file, I2C_RDWR, &rdwr);
		if (nmsgs_sent < 0) {
			fprintf(stderr, "Error: Sending messages failed: %s\n", strerror(errno));
			goto err_out;
		} 
		print_msgs(msgs, nmsgs_sent, PRINT_READ_BUF | (1 ? PRINT_HEADER | PRINT_WRITE_BUF : 0));
	}

	close(file);

	for (i = 0; i < nmsgs; i++)
		free(msgs[i].buf);

	exit(0);

 err_out_with_arg:
	fprintf(stderr, "Error: faulty argument is '%s'\n", argv[arg_idx]);
 err_out:
	close(file);

	for (i = 0; i <= nmsgs; i++)
		free(msgs[i].buf);

	exit(1);
}
