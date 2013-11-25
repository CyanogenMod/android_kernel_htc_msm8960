#if defined(CONFIG_MSM_RTB)
#undef __raw_writeb
#undef __raw_writew
#undef __raw_writel
#define __raw_writeb(v, a)     __raw_writeb_no_log(v, a)
#define __raw_writew(v, a)     __raw_writew_no_log(v, a)
#define __raw_writel(v, a)     __raw_writel_no_log(v, a)
#undef __raw_readb
#undef __raw_readw
#undef __raw_readl
#define __raw_readb(a)         __raw_readb_no_log(a)
#define __raw_readw(a)         __raw_readw_no_log(a)
#define __raw_readl(a)         __raw_readl_no_log(a)
#endif
