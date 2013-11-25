#ifndef ASMARM_MACH_MMC_H
#define ASMARM_MACH_MMC_H

#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/sdio_func.h>
#include <mach/gpio.h>
#include <mach/msm_bus.h>

#define SDC_DAT1_DISABLE 0
#define SDC_DAT1_ENABLE  1
#define SDC_DAT1_ENWAKE  2
#define SDC_DAT1_DISWAKE 3

struct embedded_sdio_data {
        struct sdio_cis cis;
        struct sdio_cccr cccr;
        struct sdio_embedded_func *funcs;
        int num_funcs;
};

struct msm_mmc_reg_data {
	
	struct regulator *reg;
	
	const char *name;
	
	unsigned int low_vol_level;
	unsigned int high_vol_level;
	
	unsigned int lpm_uA;
	unsigned int hpm_uA;
	bool set_voltage_sup;
	
	bool is_enabled;
	
	bool always_on;
	
	bool lpm_sup;
	bool reset_at_init;
};

struct msm_mmc_slot_reg_data {
	struct msm_mmc_reg_data *vdd_data; 
	struct msm_mmc_reg_data *vdd_io_data; 
};

struct msm_mmc_gpio {
	u32 no;
	const char *name;
	bool is_always_on;
	bool is_enabled;
};

struct msm_mmc_gpio_data {
	struct msm_mmc_gpio *gpio;
	u8 size;
};

struct msm_mmc_pad_pull {
	enum msm_tlmm_pull_tgt no;
	u32 val;
};

struct msm_mmc_pad_pull_data {
	struct msm_mmc_pad_pull *on;
	struct msm_mmc_pad_pull *off;
	u8 size;
};

struct msm_mmc_pad_drv {
	enum msm_tlmm_hdrive_tgt no;
	u32 val;
};

struct msm_mmc_pad_drv_data {
	struct msm_mmc_pad_drv *on;
	struct msm_mmc_pad_drv *on_SDR104;
	struct msm_mmc_pad_drv *off;
	u8 size;
};

struct msm_mmc_pad_data {
	struct msm_mmc_pad_pull_data *pull;
	struct msm_mmc_pad_drv_data *drv;
};

struct msm_mmc_pin_data {
	u8 is_gpio;
	u8 cfg_sts;
	struct msm_mmc_gpio_data *gpio_data;
	struct msm_mmc_pad_data *pad_data;
};

struct msm_mmc_bus_voting_data {
	struct msm_bus_scale_pdata *use_cases;
	unsigned int *bw_vecs;
	unsigned int bw_vecs_size;
};

struct mmc_platform_data {
	unsigned int ocr_mask;			
	int built_in;				
	int card_present;			
	u32 (*translate_vdd)(struct device *, unsigned int);
	int (*config_sdgpio)(bool);
	unsigned int (*status)(struct device *);
	struct embedded_sdio_data *embedded_sdio;
	int (*register_status_notify)(void (*callback)(int card_present, void *dev_id), void *dev_id);
	unsigned int xpc_cap;
	
	unsigned int uhs_caps;
	
	unsigned int uhs_caps2;
	unsigned int *slot_type;
	void (*sdio_lpm_gpio_setup)(struct device *, unsigned int);
        unsigned int status_irq;
	unsigned int status_gpio;
	
	bool is_status_gpio_active_low;
        unsigned int sdiowakeup_irq;
        unsigned long irq_flags;
        unsigned long mmc_bus_width;
        int (*wpswitch) (struct device *);
	unsigned int msmsdcc_fmin;
	unsigned int msmsdcc_fmid;
	unsigned int msmsdcc_fmax;
	bool nonremovable;
	bool hc_erase_group_def;
	bool pack_cmd_support;
	bool sanitize_support;
	bool cache_support;
	bool bkops_support;
	unsigned int mpm_sdiowakeup_int;
	unsigned int wpswitch_gpio;
	unsigned char wpswitch_polarity; 
	bool is_wpswitch_active_low;
	struct msm_mmc_slot_reg_data *vreg_data;
	int is_sdio_al_client;
	unsigned int *sup_clk_table;
	unsigned char sup_clk_cnt;
	struct msm_mmc_pin_data *pin_data;
	struct msm_mmc_pin_data *pin_data_SDR104;
	bool disable_bam;
	bool disable_runtime_pm;
	bool disable_cmd23;
	u32 cpu_dma_latency;
	struct msm_mmc_bus_voting_data *msm_bus_voting_data;
};

#endif
