/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright (c) 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * The logical naming of flash comes from the Android project
 * Thse structures and functions that look like fastboot_flash_*
 * They come from bootloader/legacy/include/boot/flash.h
 *
 * The boot_img_hdr structure and associated magic numbers also
 * come from the Android project.  They are from
 * bootloader/legacy/include/boot/bootimg.h
 *
 * Here are their copyrights
 *
 * Copyright (C) 2008 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef FASTBOOT_H
#define FASTBOOT_H

#include <common.h>
#include <command.h>
#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>
#include <android_image.h>

struct fastboot_config {

	/*
	 * Transfer buffer for storing data sent by the client. It should be
	 * able to hold a kernel image and flash partitions. Care should be
	 * take so it does not overrun bootloader memory
	 */
	unsigned char *transfer_buffer;

	/* Size of the buffer mentioned above */
	unsigned int transfer_buffer_size;
};

#define FB_STR_PRODUCT_IDX      1
#define FB_STR_SERIAL_IDX       2
#define FB_STR_CONFIG_IDX       3
#define FB_STR_INTERFACE_IDX    4
#define FB_STR_MANUFACTURER_IDX 5
#define FB_STR_PROC_REV_IDX     6
#define FB_STR_PROC_TYPE_IDX    7
#define FB_STR_USER_DATA_IDX    8


#define FASTBOOT_INTERFACE_CLASS     0xff
#define FASTBOOT_INTERFACE_SUB_CLASS 0x42
#define FASTBOOT_INTERFACE_PROTOCOL  0x03

#define FASTBOOT_VERSION "0.5"

/* The fastboot client uses a value of 2048 for the 
   page size of it boot.img file format. 
   Reset this in your board config file as needed. */
#ifndef CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE
#define CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE 2048
#endif

typedef enum { NAND, EMMC } storage_medium_type;

struct cmd_fastboot_interface
{
	/* This function is called when a buffer has been 
	   recieved from the client app.
	   The buffer is a supplied by the board layer and must be unmodified. 
	   The buffer_size is how much data is passed in. 
	   Returns 0 on success
	   Returns 1 on failure	

	   Set by cmd_fastboot	*/
	int (*rx_handler)(const unsigned char *buffer,
			  unsigned int buffer_size);
	
	/* This function is called when an exception has
	   occurred in the device code and the state
	   off fastboot needs to be reset 

	   Set by cmd_fastboot */
	void (*reset_handler)(void);
  
	/* A getvar string for the product name
	   It can have a maximum of 60 characters 

	   Set by board	*/
	char *product_name;
	
	/* A getvar string for the serial number 
	   It can have a maximum of 60 characters 

	   Set by board */
	char *serial_no;

	/* A getvar string for the processor revision
	   It can have a maximum of 60 characters

	   Set by board */
	char *proc_rev;

	/* A getvar string for the processor type
	   this can be GP, EMU or HS
	   It can have a maximum of 60 characters

	   Set by board */
	char *proc_type;

	/* To determine the storage type NAND or EMMC */
	storage_medium_type storage_medium;

	/* Nand block size 
	   Supports the write option WRITE_NEXT_GOOD_BLOCK 

	   Set by board */
	unsigned int nand_block_size;

	/* Transfer buffer, for handling flash updates
	   Should be multiple of the nand_block_size 
	   Care should be take so it does not overrun bootloader memory	
	   Controlled by the configure variable CFG_FASTBOOT_TRANSFER_BUFFER 

	   Set by board */
	unsigned char *transfer_buffer;

	/* How big is the transfer buffer
	   Controlled by the configure variable
	   CFG_FASTBOOT_TRANSFER_BUFFER_SIZE

	   Set by board	*/ 
	unsigned int transfer_buffer_size;

};

/* Android-style flash naming */
typedef struct fastboot_ptentry fastboot_ptentry;

/* flash partitions are defined in terms of blocks
** (flash erase units)
*/
struct fastboot_ptentry
{
	/* The logical name for this partition, null terminated */
	char name[16];
	/* The start wrt the nand part, must be multiple of nand block size */
	unsigned int start;
	/* The length of the partition, must be multiple of nand block size */
	unsigned int length;
	/* Controls the details of how operations are done on the partition
	   See the FASTBOOT_PTENTRY_FLAGS_*'s defined below */
	unsigned int flags;
};

/* Lower byte shows if the read/write/erase operation in 
   repeated.  The base address is incremented. 
   Either 0 or 1 is ok for a default */

#define FASTBOOT_PTENTRY_FLAGS_REPEAT(n)              (n & 0x0f)
#define FASTBOOT_PTENTRY_FLAGS_REPEAT_MASK            0x0000000F

/* Writes happen a block at a time.
   If the write fails, go to next block 
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_NEXT_GOOD_BLOCK  0x00000010

/* Find a contiguous block big enough for a the whole file 
   NEXT_GOOD_BLOCK and CONTIGOUS_BLOCK can not both be set */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_CONTIGUOUS_BLOCK 0x00000020

/* Sets the ECC to hardware before writing 
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_HW_ECC           0x00000040

/* Sets the ECC to software before writing
   HW and SW ECC should not both be set. */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_SW_ECC           0x00000080

/* Write the file with write.i */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_I                0x00000100

/* Write the file with write.yaffs */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_YAFFS            0x00000200

/* Write the file as a series of variable/value pairs
   using the setenv and saveenv commands */
#define FASTBOOT_PTENTRY_FLAGS_WRITE_ENV              0x00000400

/* Android bootimage file format */
#define FASTBOOT_BOOT_MAGIC "ANDROID!"
#define FASTBOOT_BOOT_MAGIC_SIZE 8
#define FASTBOOT_BOOT_NAME_SIZE 16
#define FASTBOOT_BOOT_ARGS_SIZE 512

struct fastboot_boot_img_hdr {
	unsigned char magic[FASTBOOT_BOOT_MAGIC_SIZE];

	unsigned kernel_size;  /* size in bytes */
	unsigned kernel_addr;  /* physical load addr */

	unsigned ramdisk_size; /* size in bytes */
	unsigned ramdisk_addr; /* physical load addr */

	unsigned second_size;  /* size in bytes */
	unsigned second_addr;  /* physical load addr */

	unsigned tags_addr;    /* physical addr for kernel tags */
	unsigned page_size;    /* flash page size we assume */
	unsigned unused[2];    /* future expansion: should be 0 */

	unsigned char name[FASTBOOT_BOOT_NAME_SIZE]; /* asciiz product name */

	unsigned char cmdline[FASTBOOT_BOOT_ARGS_SIZE];

	unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};



#if (CONFIG_CMD_FASTBOOT)

int fastboot_init(void);
void fastboot_shutdown(void);
int fastboot_poll(void);

int fastboot_board_init(struct fastboot_config *interface,
		struct usb_gadget_strings **str);

/* board-specific fastboot commands */
extern int fastboot_oem(const char *command);

extern void fastboot_flash_reset_ptn(void);
extern void fastboot_flash_add_ptn(fastboot_ptentry *ptn, int count);
extern fastboot_ptentry *fastboot_flash_find_ptn(const char *name);

extern int board_mmc_ftbtptn_init(void);

extern char *get_ptn_size(char *buf, const char *ptn);

__weak int check_fastboot(void);

__weak void fastboot_reboot_bootloader(void);


#endif
#endif