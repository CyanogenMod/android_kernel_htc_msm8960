#ifndef _ARM_USER_H
#define _ARM_USER_H

#include <asm/page.h>
#include <asm/ptrace.h>
/* Core file format: The core file is written in such a way that gdb
   can understand it and provide useful information to the user (under
   linux we use the 'trad-core' bfd).  There are quite a number of
   obstacles to being able to view the contents of the floating point
   registers, and until these are solved you will not be able to view the
   contents of them.  Actually, you can read in the core file and look at
   the contents of the user struct to find out what the floating point
   registers contain.
   The actual file contents are as follows:
   UPAGE: 1 page consisting of a user struct that tells gdb what is present
   in the file.  Directly after this is a copy of the task_struct, which
   is currently not used by gdb, but it may come in useful at some point.
   All of the registers are stored as part of the upage.  The upage should
   always be only one page.
   DATA: The data area is stored.  We use current->end_text to
   current->brk to pick up all of the user variables, plus any memory
   that may have been malloced.  No attempt is made to determine if a page
   is demand-zero or if a page is totally unused, we just cover the entire
   range.  All of the addresses are rounded in such a way that an integral
   number of pages is written.
   STACK: We need the stack information in order to get a meaningful
   backtrace.  We need to write the data from (esp) to
   current->start_stack, so we round each of these off in order to be able
   to write an integer number of pages.
   The minimum core file size is 3 pages, or 12288 bytes.
*/

struct user_fp {
	struct fp_reg {
		unsigned int sign1:1;
		unsigned int unused:15;
		unsigned int sign2:1;
		unsigned int exponent:14;
		unsigned int j:1;
		unsigned int mantissa1:31;
		unsigned int mantissa0:32;
	} fpregs[8];
	unsigned int fpsr:32;
	unsigned int fpcr:32;
	unsigned char ftype[8];
	unsigned int init_flag;
};

struct user{
  struct pt_regs regs;		
  int u_fpvalid;		
                                
  unsigned long int u_tsize;	
  unsigned long int u_dsize;	
  unsigned long int u_ssize;	
  unsigned long start_code;     
  unsigned long start_stack;	
  long int signal;     		
  int reserved;			
  unsigned long u_ar0;		
				
  unsigned long magic;		
  char u_comm[32];		
  int u_debugreg[8];		
  struct user_fp u_fp;		
  struct user_fp_struct * u_fp0;
  				
};
#define NBPG PAGE_SIZE
#define UPAGES 1
#define HOST_TEXT_START_ADDR (u.start_code)
#define HOST_STACK_END_ADDR (u.start_stack + u.u_ssize * NBPG)

struct user_vfp {
	unsigned long long fpregs[32];
	unsigned long fpscr;
};

struct user_vfp_exc {
	unsigned long	fpexc;
	unsigned long	fpinst;
	unsigned long	fpinst2;
};

#endif 
