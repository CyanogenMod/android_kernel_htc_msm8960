
struct partition_info
{
  u8 flg;			
  char id[3];			
  __be32 st;			
  __be32 siz;			
};

struct rootsector
{
  char unused[0x156];		
  struct partition_info icdpart[8];	
  char unused2[0xc];
  u32 hd_siz;			
  struct partition_info part[4];
  u32 bsl_st;			
  u32 bsl_cnt;			
  u16 checksum;			
} __attribute__((__packed__));

int atari_partition(struct parsed_partitions *state);
