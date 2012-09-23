#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <net/tcp.h>

#define MAXDATASIZE	5000000
//#define MAXDATASIZE	5000
#define MAXTMP1SIZE 512

#define NIPQUAD(addr) \
    ((unsigned char *)&addr)[0], \
    ((unsigned char *)&addr)[1], \
    ((unsigned char *)&addr)[2], \
    ((unsigned char *)&addr)[3]

unsigned int probe_seq_tx;

/* Using Procfs to record log data */
static struct proc_dir_entry *proc_mtd;
static char ProcBuffer[MAXDATASIZE]={0};
static char Tmp1[MAXTMP1SIZE]={0};
static int Ring=0, WritingLength;

static void* 	log_seq_start(struct seq_file *sfile, loff_t *pos);
static void* 	log_seq_next(struct seq_file *sfile, void *v, loff_t *pos);
static void 	log_seq_stop(struct seq_file *sfile, void *v);
static int 	log_seq_show(struct seq_file *sfile, void *v);

static struct seq_operations log_seq_ops = {
	.start = log_seq_start,
	.next = log_seq_next,
	.stop = log_seq_stop,
	.show = log_seq_show
};

static int log_proc_open(struct inode *inode, struct file *file);

static struct file_operations log_proc_ops = {
	.owner = THIS_MODULE,
	.open = log_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release
};

int init_module(void)
{
	/* Procfs setting */
	WritingLength = 0;
	if( (proc_mtd = create_proc_entry("htc_monitor", 0, 0)) ){
		proc_mtd->proc_fops = &log_proc_ops;
	}

	return 0;
}

void cleanup_module(void)
{
	/* Procfs setting */
	if(proc_mtd)
		remove_proc_entry("test", 0);

	printk("\n--- Transmit Module uninstall ok! ---\n");
}

static void* log_seq_start(struct seq_file *sfile, loff_t *pos)
{
/*	
	if(*pos >= WritingLength)
		return NULL;
		
	return ProcBuffer + *pos;
*/
	if(*pos >= MAXDATASIZE)
	{
     return NULL; 
	}	
	else
	{
		if (*pos >= WritingLength && 0==Ring)
			return NULL;
  }
	return ProcBuffer + *pos;
}

static void* log_seq_next(struct seq_file *sfile, void *v, loff_t *pos)
{
	(*pos)++;
	if(*pos >= MAXDATASIZE)
	{
     return NULL; 
	}	
	else
	{
		if (*pos >= WritingLength && 0==Ring)
			return NULL;
  }		
	return ProcBuffer + *pos;
}

static void log_seq_stop(struct seq_file *sfile, void *v)
{
	return ;
}

static int log_seq_show(struct seq_file *sfile, void *v)
{
	char c = *((char *)v);
	seq_putc(sfile, c);

	return 0;
}

static int log_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &log_seq_ops);
}

inline void record_probe_data(struct sock *sk, int type, size_t size, unsigned long long t_pre)
{
//   struct timeval tv;	
   struct inet_sock *inet = inet_sk(sk);
   __be16 sport, dport;
   __be32 daddr, saddr;	  
	 unsigned long long t_now;
	unsigned long nanosec_rem;
	unsigned long nanosec_rem_pre;	
	 t_now = sched_clock();    
	 nanosec_rem=do_div(t_now, 1000000000U);
	 nanosec_rem_pre=do_div(t_pre, 1000000000U);
//   do_gettimeofday(&tv);

   if (!inet)
      return;	
   saddr=inet->inet_rcv_saddr;
   sport=inet->inet_num;
   daddr=inet->inet_daddr;				 	
   dport=inet->inet_dport;	
    
   //filter
   if (0x00000000==saddr || 0x0100007f==saddr)   
      return;  
   memset(&Tmp1, 0, sizeof(char)*MAXTMP1SIZE);
   
   switch (type)
   {
      case 1: //send
      {
         unsigned long long t_diff=t_now-t_pre;		 
	  unsigned long nanosec_rem_diff;
//	  printk("[victor_kernel] [%05u.%09lu] \n", (unsigned)t_pre,nanosec_rem_pre);
//	  printk("[victor_kernel] [%05u.%09lu] \n", (unsigned)t_now,nanosec_rem);	  
	  if (nanosec_rem>=nanosec_rem_pre)
	     	nanosec_rem_diff=nanosec_rem-nanosec_rem_pre;
	  else
	  {
	       if (t_diff>0)
	       {
	          t_diff=t_diff-1;
	          nanosec_rem_diff=1000000000+nanosec_rem-nanosec_rem_pre;
	       }
	       else
	     	{
	     	   t_diff=t_pre;
		   nanosec_rem_diff=nanosec_rem_pre;	   
	     	}
	  }
//	  printk("[victor_kernel] [%05u.%09lu] \n", (unsigned)t_diff,nanosec_rem_diff);	  
          sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d        SEND S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,%08d Bytes,D.T[%01u.%09lu]\n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport,size,(unsigned)t_diff,nanosec_rem_diff);     
          memset(&t_pre,0,sizeof(unsigned long long)); 
      	   break;
      }	
      case 2: //recv
      {
         unsigned long long t_diff=t_now-t_pre;		 
	  unsigned long nanosec_rem_diff;
//	  printk("[victor_kernel] [%05u.%09lu] \n", (unsigned)t_pre,nanosec_rem_pre);
//	  printk("[victor_kernel] [%05u.%09lu] \n", (unsigned)t_now,nanosec_rem);	  
	  if (nanosec_rem>=nanosec_rem_pre)
	     	nanosec_rem_diff=nanosec_rem-nanosec_rem_pre;
	  else
	  {
	       if (t_diff>0)
	       {
	          t_diff=t_diff-1;
	          nanosec_rem_diff=1000000000+nanosec_rem-nanosec_rem_pre;
	       }
	       else
	     	{
	     	   t_diff=t_pre;
		   nanosec_rem_diff=nanosec_rem_pre;	  
	       }	   
	  }		 
          sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d        RECV S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,%08d Bytes,D.T[%01u.%09lu]\n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport,size,(unsigned)t_diff,nanosec_rem_diff); 
          memset(&t_pre,0,sizeof(unsigned long long));  		  
      	   break;
      	}
       case 3: //accept  
        sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d      ACCEPT S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,              ,                \n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport);      	
      	break;      	
      case 4: //tcp connect 
        sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d TCP CONNECT S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,              ,                \n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport);	      	
      	break;
      case 5: //udp connect
        sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d UDP CONNECT S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,              ,                \n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport);		 	      	
      	break;
      case 6: //close 
        sprintf(Tmp1,"[%05u.%09lu] UID%05d PID%05d       CLOSE S.IP:%03d.%03d.%03d.%03d/%05d, D.IP:%03d.%03d.%03d.%03d/%05d,              ,                \n",(unsigned)t_now,nanosec_rem,current->cred->uid,current->pid,NIPQUAD(saddr),sport,NIPQUAD(daddr),dport);      	
      	break;
      default:
      	break;	      	      	      	      		
   }	  
//	 printk("[victor_kernel_Tmp1]%s\n", Tmp1);	  
	 if(WritingLength + strlen(Tmp1) < MAXDATASIZE)
	 {
      memcpy(&ProcBuffer[WritingLength], Tmp1, strlen(Tmp1));
	    WritingLength += strlen(Tmp1);
	 }
   else
   {
   	  //char *startpos=NULL;
      //printk("[victor_kernel] else \n");	   	  
   	  //startpos=strstr(&ProcBuffer[StartLength], "\n");
   	  //WritingLength=startpos-ProcBuffer;
      //printk("[victor_kernel]ProcBuffer=%p, startpos=%p, WritingLength=%d \n", WritingLength, ProcBuffer, startpos);	   	  
      WritingLength=0;
      Ring=1;
      memcpy(&ProcBuffer[WritingLength], Tmp1, strlen(Tmp1));
	    WritingLength += strlen(Tmp1);      	
   } 	
}

// --modify by victor START
static int __init monitor_init(void)
{   
	return init_module();
}
module_init(monitor_init);
// -- modify by victor END
