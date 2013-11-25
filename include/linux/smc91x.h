#ifndef __SMC91X_H__
#define __SMC91X_H__

#define SMC91X_USE_8BIT (1 << 0)
#define SMC91X_USE_16BIT (1 << 1)
#define SMC91X_USE_32BIT (1 << 2)

#define SMC91X_NOWAIT		(1 << 3)

#define SMC91X_IO_SHIFT_0	(0 << 4)
#define SMC91X_IO_SHIFT_1	(1 << 4)
#define SMC91X_IO_SHIFT_2	(2 << 4)
#define SMC91X_IO_SHIFT_3	(3 << 4)
#define SMC91X_IO_SHIFT(x)	(((x) >> 4) & 0x3)

#define SMC91X_USE_DMA		(1 << 6)

#define RPC_LED_100_10	(0x00)	
#define RPC_LED_RES	(0x01)	
#define RPC_LED_10	(0x02)	
#define RPC_LED_FD	(0x03)	
#define RPC_LED_TX_RX	(0x04)	
#define RPC_LED_100	(0x05)	
#define RPC_LED_TX	(0x06)	
#define RPC_LED_RX	(0x07)	

struct smc91x_platdata {
	unsigned long flags;
	unsigned char leda;
	unsigned char ledb;
};

#endif 
