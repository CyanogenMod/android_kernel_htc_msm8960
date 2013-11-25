
#define MAC_PARTITION_MAGIC	0x504d

#define APPLE_AUX_TYPE	"Apple_UNIX_SVR2"

struct mac_partition {
	__be16	signature;	
	__be16	res1;
	__be32	map_count;	
	__be32	start_block;	
	__be32	block_count;	
	char	name[32];	
	char	type[32];	
	__be32	data_start;	
	__be32	data_count;	
	__be32	status;		
	__be32	boot_start;
	__be32	boot_size;
	__be32	boot_load;
	__be32	boot_load2;
	__be32	boot_entry;
	__be32	boot_entry2;
	__be32	boot_cksum;
	char	processor[16];	
	
};

#define MAC_STATUS_BOOTABLE	8	

#define MAC_DRIVER_MAGIC	0x4552

struct mac_driver_desc {
	__be16	signature;	
	__be16	block_size;
	__be32	block_count;
    
};

int mac_partition(struct parsed_partitions *state);
