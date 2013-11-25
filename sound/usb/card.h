#ifndef __USBAUDIO_CARD_H
#define __USBAUDIO_CARD_H

#define MAX_NR_RATES	1024
#define MAX_PACKS	20
#define MAX_PACKS_HS	(MAX_PACKS * 8)	
#define MAX_URBS	8
#define SYNC_URBS	4	
#define MAX_QUEUE	24	

struct audioformat {
	struct list_head list;
	u64 formats;			
	unsigned int channels;		
	unsigned int fmt_type;		
	unsigned int frame_size;	
	int iface;			
	unsigned char altsetting;	
	unsigned char altset_idx;	
	unsigned char attributes;	
	unsigned char endpoint;		
	unsigned char ep_attr;		
	unsigned char datainterval;	
	unsigned int maxpacksize;	
	unsigned int rates;		
	unsigned int rate_min, rate_max;	
	unsigned int nr_rates;		
	unsigned int *rate_table;	
	unsigned char clock;		
};

struct snd_usb_substream;

struct snd_urb_ctx {
	struct urb *urb;
	unsigned int buffer_size;	
	struct snd_usb_substream *subs;
	int index;	
	int packets;	
};

struct snd_urb_ops {
	int (*prepare)(struct snd_usb_substream *subs, struct snd_pcm_runtime *runtime, struct urb *u);
	int (*retire)(struct snd_usb_substream *subs, struct snd_pcm_runtime *runtime, struct urb *u);
	int (*prepare_sync)(struct snd_usb_substream *subs, struct snd_pcm_runtime *runtime, struct urb *u);
	int (*retire_sync)(struct snd_usb_substream *subs, struct snd_pcm_runtime *runtime, struct urb *u);
};

struct snd_usb_substream {
	struct snd_usb_stream *stream;
	struct usb_device *dev;
	struct snd_pcm_substream *pcm_substream;
	int direction;	
	int interface;	
	int endpoint;	
	struct audioformat *cur_audiofmt;	
	unsigned int cur_rate;		
	unsigned int period_bytes;	
	unsigned int altset_idx;     
	unsigned int datapipe;   
	unsigned int syncpipe;   
	unsigned int datainterval;	
	unsigned int syncinterval;  
	unsigned int freqn;      
	unsigned int freqm;      
	int          freqshift;  
	unsigned int freqmax;    
	unsigned int phase;      
	unsigned int maxpacksize;	
	unsigned int maxframesize;	
	unsigned int curpacksize;	
	unsigned int curframesize;	
	unsigned int syncmaxsize;	
	unsigned int fill_max: 1;	
	unsigned int txfr_quirk:1;	
	unsigned int fmt_type;		

	unsigned int running: 1;	

	unsigned int hwptr_done;	
	unsigned int transfer_done;		
	unsigned long active_mask;	
	unsigned long unlink_mask;	

	unsigned int nurbs;			
	struct snd_urb_ctx dataurb[MAX_URBS];	
	struct snd_urb_ctx syncurb[SYNC_URBS];	
	char *syncbuf;				
	dma_addr_t sync_dma;			

	u64 formats;			
	unsigned int num_formats;		
	struct list_head fmt_list;	
	struct snd_pcm_hw_constraint_list rate_list;	
	spinlock_t lock;

	struct snd_urb_ops ops;		
	int last_frame_number;          
	int last_delay;                 
};

struct snd_usb_stream {
	struct snd_usb_audio *chip;
	struct snd_pcm *pcm;
	int pcm_index;
	unsigned int fmt_type;		
	struct snd_usb_substream substream[2];
	struct list_head list;
};

#endif 
