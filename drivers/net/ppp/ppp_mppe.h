#define MPPE_PAD                4      
#define MPPE_MAX_KEY_LEN       16      

#define MPPE_OPT_40            0x01    
#define MPPE_OPT_128           0x02    
#define MPPE_OPT_STATEFUL      0x04    
#define MPPE_OPT_56            0x08    
#define MPPE_OPT_MPPC          0x10    
#define MPPE_OPT_D             0x20    
#define MPPE_OPT_UNSUPPORTED (MPPE_OPT_56|MPPE_OPT_MPPC|MPPE_OPT_D)
#define MPPE_OPT_UNKNOWN       0x40    

#define MPPE_C_BIT             0x01    
#define MPPE_D_BIT             0x10    
#define MPPE_L_BIT             0x20    
#define MPPE_S_BIT             0x40    
#define MPPE_M_BIT             0x80    
#define MPPE_H_BIT             0x01    

#define MPPE_ALL_BITS (MPPE_D_BIT|MPPE_L_BIT|MPPE_S_BIT|MPPE_M_BIT|MPPE_H_BIT)

#define MPPE_OPTS_TO_CI(opts, ci)              \
    do {                                       \
       u_char *ptr = ci;        \
                                               \
                                    \
       if (opts & MPPE_OPT_STATEFUL)           \
           *ptr++ = 0x0;                       \
       else                                    \
           *ptr++ = MPPE_H_BIT;                \
       *ptr++ = 0;                             \
       *ptr++ = 0;                             \
                                               \
                                 \
       *ptr = 0;                               \
       if (opts & MPPE_OPT_128)                \
           *ptr |= MPPE_S_BIT;                 \
       if (opts & MPPE_OPT_40)                 \
           *ptr |= MPPE_L_BIT;                 \
                 \
    } while ( 0)

#define MPPE_CI_TO_OPTS(ci, opts)              \
    do {                                       \
       u_char *ptr = ci;        \
                                               \
       opts = 0;                               \
                                               \
                                    \
       if (!(ptr[0] & MPPE_H_BIT))             \
           opts |= MPPE_OPT_STATEFUL;          \
                                               \
                                 \
       if (ptr[3] & MPPE_S_BIT)                \
           opts |= MPPE_OPT_128;               \
       if (ptr[3] & MPPE_L_BIT)                \
           opts |= MPPE_OPT_40;                \
                                               \
                               \
       if (ptr[3] & MPPE_M_BIT)                \
           opts |= MPPE_OPT_56;                \
       if (ptr[3] & MPPE_D_BIT)                \
           opts |= MPPE_OPT_D;                 \
       if (ptr[3] & MPPE_C_BIT)                \
           opts |= MPPE_OPT_MPPC;              \
                                               \
                               \
       if (ptr[0] & ~MPPE_H_BIT)               \
           opts |= MPPE_OPT_UNKNOWN;           \
       if (ptr[1] || ptr[2])                   \
           opts |= MPPE_OPT_UNKNOWN;           \
       if (ptr[3] & ~MPPE_ALL_BITS)            \
           opts |= MPPE_OPT_UNKNOWN;           \
    } while ( 0)
