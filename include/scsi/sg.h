#ifndef _SCSI_GENERIC_H
#define _SCSI_GENERIC_H

#include <linux/compiler.h>

/*
   History:
    Started: Aug 9 by Lawrence Foard (entropy@world.std.com), to allow user
     process control of SCSI devices.
    Development Sponsored by Killy Corp. NY NY
Original driver (sg.h):
*       Copyright (C) 1992 Lawrence Foard
Version 2 and 3 extensions to driver:
*       Copyright (C) 1998 - 2006 Douglas Gilbert

    Version: 3.5.34 (20060920)
    This version is for 2.6 series kernels.

    For a full changelog see http://www.torque.net/sg

Map of SG verions to the Linux kernels in which they appear:
       ----------        ----------------------------------
       original          all kernels < 2.2.6
       2.1.40            2.2.20
       3.0.x             optional version 3 sg driver for 2.2 series
       3.1.17++          2.4.0++
       3.5.30++          2.6.0++

Major new features in SG 3.x driver (cf SG 2.x drivers)
	- SG_IO ioctl() combines function if write() and read()
	- new interface (sg_io_hdr_t) but still supports old interface
	- scatter/gather in user space, direct IO, and mmap supported

 The normal action of this driver is to use the adapter (HBA) driver to DMA
 data into kernel buffers and then use the CPU to copy the data into the 
 user space (vice versa for writes). That is called "indirect" IO due to 
 the double handling of data. There are two methods offered to remove the
 redundant copy: 1) direct IO and 2) using the mmap() system call to map
 the reserve buffer (this driver has one reserve buffer per fd) into the
 user space. Both have their advantages.
 In terms of absolute speed mmap() is faster. If speed is not a concern, 
 indirect IO should be fine. Read the documentation for more information.

 ** N.B. To use direct IO 'echo 1 > /proc/scsi/sg/allow_dio' or
         'echo 1 > /sys/module/sg/parameters/allow_dio' is needed.
         That attribute is 0 by default. **
 
 Historical note: this SCSI pass-through driver has been known as "sg" for 
 a decade. In broader kernel discussions "sg" is used to refer to scatter
 gather techniques. The context should clarify which "sg" is referred to.

 Documentation
 =============
 A web site for the SG device driver can be found at:
	http://www.torque.net/sg  [alternatively check the MAINTAINERS file]
 The documentation for the sg version 3 driver can be found at:
 	http://www.torque.net/sg/p/sg_v3_ho.html
 This is a rendering from DocBook source [change the extension to "sgml"
 or "xml"]. There are renderings in "ps", "pdf", "rtf" and "txt" (soon).
 The SG_IO ioctl is now found in other parts kernel (e.g. the block layer).
 For more information see http://www.torque.net/sg/sg_io.html

 The older, version 2 documents discuss the original sg interface in detail:
	http://www.torque.net/sg/p/scsi-generic.txt
	http://www.torque.net/sg/p/scsi-generic_long.txt
 Also available: <kernel_source>/Documentation/scsi/scsi-generic.txt

 Utility and test programs are available at the sg web site. They are 
 packaged as sg3_utils (for the lk 2.4 and 2.6 series) and sg_utils
 (for the lk 2.2 series).
*/

#ifdef __KERNEL__
extern int sg_big_buff; 
#endif


typedef struct sg_iovec 
{                       
    void __user *iov_base;      
    size_t iov_len;             
} sg_iovec_t;


typedef struct sg_io_hdr
{
    int interface_id;           
    int dxfer_direction;        
    unsigned char cmd_len;      
    unsigned char mx_sb_len;    
    unsigned short iovec_count; 
    unsigned int dxfer_len;     
    void __user *dxferp;	
    unsigned char __user *cmdp; 
    void __user *sbp;		
    unsigned int timeout;       
    unsigned int flags;         
    int pack_id;                
    void __user * usr_ptr;      
    unsigned char status;       
    unsigned char masked_status;
    unsigned char msg_status;   
    unsigned char sb_len_wr;    /* [o] byte count actually written to sbp */
    unsigned short host_status; 
    unsigned short driver_status;
    int resid;                  
    unsigned int duration;      
    unsigned int info;          
} sg_io_hdr_t;  

#define SG_INTERFACE_ID_ORIG 'S'

#define SG_DXFER_NONE (-1)      
#define SG_DXFER_TO_DEV (-2)    
#define SG_DXFER_FROM_DEV (-3)  
#define SG_DXFER_TO_FROM_DEV (-4) 
#define SG_DXFER_UNKNOWN (-5)   

#define SG_FLAG_DIRECT_IO 1     
#define SG_FLAG_UNUSED_LUN_INHIBIT 2   
				
#define SG_FLAG_MMAP_IO 4       
#define SG_FLAG_NO_DXFER 0x10000 
				

#define SG_INFO_OK_MASK 0x1
#define SG_INFO_OK 0x0          
#define SG_INFO_CHECK 0x1       

#define SG_INFO_DIRECT_IO_MASK 0x6
#define SG_INFO_INDIRECT_IO 0x0 
#define SG_INFO_DIRECT_IO 0x2   
#define SG_INFO_MIXED_IO 0x4    


typedef struct sg_scsi_id { 
    int host_no;        
    int channel;
    int scsi_id;        
    int lun;
    int scsi_type;      
    short h_cmd_per_lun;
    short d_queue_depth;
    int unused[2];      
} sg_scsi_id_t; 

typedef struct sg_req_info { 
    char req_state;     /* 0 -> not used, 1 -> written, 2 -> ready to read */
    char orphan;        
    char sg_io_owned;   
    char problem;       
    int pack_id;        
    void __user *usr_ptr;     
    unsigned int duration; /* millisecs elapsed since written (req_state==1)
			      or request duration (req_state==2) */
    int unused;
} sg_req_info_t; 



#define SG_EMULATED_HOST 0x2203 

#define SG_SET_TRANSFORM 0x2204 
		      
#define SG_GET_TRANSFORM 0x2205

#define SG_SET_RESERVED_SIZE 0x2275  
#define SG_GET_RESERVED_SIZE 0x2272  

#define SG_GET_SCSI_ID 0x2276   

#define SG_SET_FORCE_LOW_DMA 0x2279  
#define SG_GET_LOW_DMA 0x227a   

#define SG_SET_FORCE_PACK_ID 0x227b
#define SG_GET_PACK_ID 0x227c 

#define SG_GET_NUM_WAITING 0x227d 

#define SG_GET_SG_TABLESIZE 0x227F  

#define SG_GET_VERSION_NUM 0x2282 

#define SG_SCSI_RESET 0x2284
#define		SG_SCSI_RESET_NOTHING	0
#define		SG_SCSI_RESET_DEVICE	1
#define		SG_SCSI_RESET_BUS	2
#define		SG_SCSI_RESET_HOST	3
#define		SG_SCSI_RESET_TARGET	4

#define SG_IO 0x2285   

#define SG_GET_REQUEST_TABLE 0x2286   

#define SG_SET_KEEP_ORPHAN 0x2287 
#define SG_GET_KEEP_ORPHAN 0x2288

#define SG_GET_ACCESS_COUNT 0x2289  


#define SG_SCATTER_SZ (8 * 4096)
/* Largest size (in bytes) a single scatter-gather list element can have.
   The value used by the driver is 'max(SG_SCATTER_SZ, PAGE_SIZE)'.
   This value should be a power of 2 (and may be rounded up internally).
   If scatter-gather is not supported by adapter then this value is the
   largest data block that can be read/written by a single scsi command. */

#define SG_DEFAULT_RETRIES 0

#define SG_DEF_FORCE_LOW_DMA 0  
#define SG_DEF_FORCE_PACK_ID 0
#define SG_DEF_KEEP_ORPHAN 0
#define SG_DEF_RESERVED_SIZE SG_SCATTER_SZ 

#define SG_MAX_QUEUE 16

#define SG_BIG_BUFF SG_DEF_RESERVED_SIZE    

typedef struct sg_io_hdr Sg_io_hdr;
typedef struct sg_io_vec Sg_io_vec;
typedef struct sg_scsi_id Sg_scsi_id;
typedef struct sg_req_info Sg_req_info;



#define SG_MAX_SENSE 16   

struct sg_header
{
    int pack_len;    
    int reply_len;   
    int pack_id;     
    int result;      
    unsigned int twelve_byte:1;
	
    unsigned int target_status:5;   
    unsigned int host_status:8;     
    unsigned int driver_status:8;   
    unsigned int other_flags:10;    
    unsigned char sense_buffer[SG_MAX_SENSE]; 
};      



#define SG_SET_TIMEOUT 0x2201  
#define SG_GET_TIMEOUT 0x2202  

#define SG_GET_COMMAND_Q 0x2270   
#define SG_SET_COMMAND_Q 0x2271   

#define SG_SET_DEBUG 0x227e    

#define SG_NEXT_CMD_LEN 0x2283  


#ifdef __KERNEL__
#define SG_DEFAULT_TIMEOUT_USER	(60*USER_HZ) 
#else
#define SG_DEFAULT_TIMEOUT	(60*HZ)	     
#endif

#define SG_DEF_COMMAND_Q 0     
#define SG_DEF_UNDERRUN_FLAG 0

#endif
