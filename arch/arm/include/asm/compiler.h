#ifndef __ASM_ARM_COMPILER_H
#define __ASM_ARM_COMPILER_H

#define __asmeq(x, y)  ".ifnc " x "," y " ; .err ; .endif\n\t"


#endif 
