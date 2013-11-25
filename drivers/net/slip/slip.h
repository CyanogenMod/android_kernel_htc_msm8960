#ifndef _LINUX_SLIP_H
#define _LINUX_SLIP_H


#if defined(CONFIG_INET) && defined(CONFIG_SLIP_COMPRESSED)
# define SL_INCLUDE_CSLIP
#endif

#ifdef SL_INCLUDE_CSLIP
# define SL_MODE_DEFAULT SL_MODE_ADAPTIVE
#else
# define SL_MODE_DEFAULT SL_MODE_SLIP
#endif

#define SL_NRUNIT	256		
#define SL_MTU		296		

#define END             0300		
#define ESC             0333		
#define ESC_END         0334		
#define ESC_ESC         0335		


struct slip {
  int			magic;

  
  struct tty_struct	*tty;		
  struct net_device	*dev;		
  spinlock_t		lock;

#ifdef SL_INCLUDE_CSLIP
  struct slcompress	*slcomp;	
  unsigned char		*cbuff;		
#endif

  
  unsigned char		*rbuff;		
  int                   rcount;         
  unsigned char		*xbuff;		
  unsigned char         *xhead;         
  int                   xleft;          
  int			mtu;		
  int                   buffsize;       

#ifdef CONFIG_SLIP_MODE_SLIP6
  int			xdata, xbits;	
#endif

  unsigned long		flags;		
#define SLF_INUSE	0		
#define SLF_ESCAPE	1               
#define SLF_ERROR	2               
#define SLF_KEEPTEST	3		
#define SLF_OUTWAIT	4		

  unsigned char		mode;		
  unsigned char		leased;
  pid_t			pid;
#define SL_MODE_SLIP	0
#define SL_MODE_CSLIP	1
#define SL_MODE_SLIP6	2		
#define SL_MODE_CSLIP6	(SL_MODE_SLIP6|SL_MODE_CSLIP)
#define SL_MODE_AX25	4
#define SL_MODE_ADAPTIVE 8
#ifdef CONFIG_SLIP_SMART
  unsigned char		outfill;	
  unsigned char		keepalive;	
  struct timer_list	outfill_timer;
  struct timer_list	keepalive_timer;
#endif
};

#define SLIP_MAGIC 0x5302

#endif	
