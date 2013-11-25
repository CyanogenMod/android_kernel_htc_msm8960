/*
 *  Copyright (C) 1997 Martin Mares
 *
 *  Automatic IP Layer Configuration
 */


extern int ic_proto_enabled;	
extern int ic_set_manually;	

extern __be32 ic_myaddr;		
extern __be32 ic_gateway;		

extern __be32 ic_servaddr;		

extern __be32 root_server_addr;	
extern u8 root_server_path[];	


#define IC_PROTO	0xFF	
#define IC_BOOTP	0x01	
#define IC_RARP		0x02	
#define IC_USE_DHCP    0x100	
