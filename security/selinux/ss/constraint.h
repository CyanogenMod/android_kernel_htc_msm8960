#ifndef _SS_CONSTRAINT_H_
#define _SS_CONSTRAINT_H_

#include "ebitmap.h"

#define CEXPR_MAXDEPTH 5

struct constraint_expr {
#define CEXPR_NOT		1 
#define CEXPR_AND		2 
#define CEXPR_OR		3 
#define CEXPR_ATTR		4 
#define CEXPR_NAMES		5 
	u32 expr_type;		

#define CEXPR_USER 1		
#define CEXPR_ROLE 2		
#define CEXPR_TYPE 4		
#define CEXPR_TARGET 8		
#define CEXPR_XTARGET 16	
#define CEXPR_L1L2 32		
#define CEXPR_L1H2 64		
#define CEXPR_H1L2 128		
#define CEXPR_H1H2 256		
#define CEXPR_L1H1 512		
#define CEXPR_L2H2 1024		
	u32 attr;		

#define CEXPR_EQ     1		
#define CEXPR_NEQ    2		
#define CEXPR_DOM    3		
#define CEXPR_DOMBY  4		
#define CEXPR_INCOMP 5		
	u32 op;			

	struct ebitmap names;	

	struct constraint_expr *next;   
};

struct constraint_node {
	u32 permissions;	
	struct constraint_expr *expr;	
	struct constraint_node *next;	
};

#endif	
