/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <mach/gpio.h>
#include <mach/gpiomux.h>
#include <mach/board.h>
#include <mach/camera.h>
#include <mach/vreg.h>
#include <mach/camera.h>
#include <mach/clk.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>

#include <msm_sensor.h>

#define BUFF_SIZE_128 128

#if 1	
#define CAM_VAF_MINUV                 2800000
#define CAM_VAF_MAXUV                 2800000
#define CAM_VDIG_MINUV                    1200000
#define CAM_VDIG_MAXUV                    1200000
#define CAM_VANA_MINUV                    2800000
#define CAM_VANA_MAXUV                    2850000
#define CAM_CSI_VDD_MINUV                  1200000
#define CAM_CSI_VDD_MAXUV                  1200000

#define CAM_VAF_LOAD_UA               300000
#define CAM_VDIG_LOAD_UA                  105000
#define CAM_VANA_LOAD_UA                  85600
#define CAM_CSI_LOAD_UA                    20000

static struct clk *camio_cam_clk;
static struct clk *camio_cam_rawchip_clk;

#endif	
static struct clk *camio_jpeg_clk;
static struct clk *camio_jpeg_pclk;
static struct clk *camio_imem_clk;
static struct regulator *fs_ijpeg;
#if 1	
static struct msm_camera_io_clk camio_clk;
static struct msm_sensor_ctrl_t *camio_sctrl;
struct msm_bus_scale_pdata *cam_bus_scale_table;
#endif	

void msm_io_w(u32 data, void __iomem *addr)
{
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	writel_relaxed((data), (addr));
}

void msm_io_w_mb(u32 data, void __iomem *addr)
{
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	wmb();
	writel_relaxed((data), (addr));
	wmb();
}

u32 msm_io_r(void __iomem *addr)
{
	uint32_t data = readl_relaxed(addr);
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	return data;
}

u32 msm_io_r_mb(void __iomem *addr)
{
	uint32_t data;
	rmb();
	data = readl_relaxed(addr);
	rmb();
	CDBG("%s: %08x %08x\n", __func__, (int) (addr), (data));
	return data;
}

void msm_io_memcpy_toio(void __iomem *dest_addr,
	void __iomem *src_addr, u32 len)
{
	int i;
	u32 *d = (u32 *) dest_addr;
	u32 *s = (u32 *) src_addr;
	
	for (i = 0; i < len; i++) {
		
		if (s)
			writel_relaxed(*s++, d++);
		else {
			pr_err("%s: invalid address %p, break", __func__, s);
			break;
		}
		
	}
}

void msm_io_dump(void __iomem *addr, int size)
{
	char line_str[BUFF_SIZE_128], *p_str;
	int i;
	u32 *p = (u32 *) addr;
	u32 data;
	CDBG("%s: %p %d\n", __func__, addr, size);
	line_str[0] = '\0';
	p_str = line_str;
	for (i = 0; i < size/4; i++) {
		if (i % 4 == 0) {
			snprintf(p_str, 12, "%08x: ", (u32) p);
			p_str += 10;
		}
		data = readl_relaxed(p++);
		snprintf(p_str, 12, "%08x ", data);
		p_str += 9;
		if ((i + 1) % 4 == 0) {
			CDBG("%s\n", line_str);
			line_str[0] = '\0';
			p_str = line_str;
		}
	}
	if (line_str[0] != '\0')
		CDBG("%s\n", line_str);
}

void msm_io_memcpy(void __iomem *dest_addr, void __iomem *src_addr, u32 len)
{
	CDBG("%s: %p %p %d\n", __func__, dest_addr, src_addr, len);
	if (dest_addr && src_addr) {
		msm_io_memcpy_toio(dest_addr, src_addr, len / 4);
		msm_io_dump(dest_addr, len);
	} else
		pr_err("%s: invalid address %p %p", __func__, dest_addr, src_addr);
}

int msm_camio_clk_enable(struct msm_camera_sensor_info* sinfo,enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;

	switch (clktype) {
#if 1	
	case CAMIO_CAM_MCLK_CLK:
		clk = clk_get(&(camio_sctrl->sensor_i2c_client->client->dev), "cam_clk");
	    if (sinfo)
	        sinfo->main_clk = clk;
	    else
	        camio_cam_clk = clk;
		pr_info("%s: clk(%p) obj.name:%s", __func__, clk, camio_sctrl->sensor_i2c_client->client->dev.kobj.name);
		if (!IS_ERR(clk))
			msm_camio_clk_rate_set_2(clk, camio_clk.mclk_clk_rate);
		break;
#endif	
	case CAMIO_CAM_RAWCHIP_MCLK_CLK:
		camio_cam_rawchip_clk =
		clk = clk_get(NULL, "cam0_clk");
		pr_info("[CAM] %s: enable CAMIO_CAM_RAWCHIP_MCLK_CLK", __func__);
		if (!IS_ERR(clk))
			clk_set_rate(clk, 24000000);
		break;

	case CAMIO_JPEG_CLK:
		camio_jpeg_clk =
		clk = clk_get(NULL, "ijpeg_clk");
		clk_set_rate(clk, 228571000);
		break;

	case CAMIO_JPEG_PCLK:
		camio_jpeg_pclk =
		clk = clk_get(NULL, "ijpeg_pclk");
		break;

	case CAMIO_IMEM_CLK:
		camio_imem_clk =
		clk = clk_get(NULL, "imem_clk");
		break;

	default:
		break;
	}

	if (!IS_ERR(clk))
		rc = clk_prepare_enable(clk);
	else
		rc = PTR_ERR(clk);

	if (rc < 0)
		pr_err("%s(%d) failed %d\n", __func__, clktype, rc);

	return rc;
}

int msm_camio_clk_disable(struct msm_camera_sensor_info* sinfo,enum msm_camio_clk_type clktype)
{
	int rc = 0;
	struct clk *clk = NULL;

	switch (clktype) {
#if 1	
	case CAMIO_CAM_MCLK_CLK:
	    clk = sinfo ? sinfo->main_clk : camio_cam_clk;

		break;
#endif	
	case CAMIO_CAM_RAWCHIP_MCLK_CLK:
		clk = camio_cam_rawchip_clk;
		break;

	case CAMIO_JPEG_CLK:
		clk = camio_jpeg_clk;
		break;

	case CAMIO_JPEG_PCLK:
		clk = camio_jpeg_pclk;
		break;

	case CAMIO_IMEM_CLK:
		clk = camio_imem_clk;
		break;

	default:
		break;
	}

	if (!IS_ERR(clk)) {
		clk_disable_unprepare(clk);
		clk_put(clk);
	} else
		rc = PTR_ERR(clk);

	if (rc < 0)
		pr_err("%s(%d) failed %d\n", __func__, clktype, rc);

	return rc;
}

#if 1	
void msm_camio_clk_rate_set(int rate)
{
	struct clk *clk = camio_cam_clk;
	clk_set_rate(clk, rate);
}
#endif	

void msm_camio_clk_rate_set_2(struct clk *clk, int rate)
{
	clk_set_rate(clk, rate);
}

int msm_camio_jpeg_clk_disable(void)
{
	int rc = 0;
	rc = msm_camio_clk_disable(0,CAMIO_JPEG_PCLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_disable(0,CAMIO_JPEG_CLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_disable(0,CAMIO_IMEM_CLK);
	if (rc < 0)
		return rc;

	if (fs_ijpeg) {
		rc = regulator_disable(fs_ijpeg);
		if (rc < 0) {
			pr_err("%s: Regulator disable failed %d\n",
						__func__, rc);
			return rc;
		}
		regulator_put(fs_ijpeg);
	}
	CDBG("%s: exit %d\n", __func__, rc);
	return rc;
}

int msm_camio_jpeg_clk_enable(void)
{
	int rc = 0;
	fs_ijpeg = regulator_get(NULL, "fs_ijpeg");
	if (IS_ERR(fs_ijpeg)) {
		pr_err("%s: Regulator FS_IJPEG get failed %ld\n",
			__func__, PTR_ERR(fs_ijpeg));
		fs_ijpeg = NULL;
	} else if (regulator_enable(fs_ijpeg)) {
		pr_err("%s: Regulator FS_IJPEG enable failed\n", __func__);
		regulator_put(fs_ijpeg);
	}

	rc = msm_camio_clk_enable(0,CAMIO_JPEG_CLK);
	if (rc < 0)
		return rc;
	rc = msm_camio_clk_enable(0,CAMIO_JPEG_PCLK);
	if (rc < 0)
		return rc;

	rc = msm_camio_clk_enable(0,CAMIO_IMEM_CLK);
	if (rc < 0)
		return rc;

	CDBG("%s: exit %d\n", __func__, rc);
	return rc;
}

#if 1	
int msm_camio_config_gpio_table(struct msm_camera_sensor_info* sinfo,int gpio_en)
{
	
	struct msm_camera_gpio_conf *gpio_conf = sinfo->gpio_conf;
	int rc = 0, i = 0;

#if 1 
	if (gpio_conf == NULL || gpio_conf->cam_gpio_tbl == NULL) {
		pr_err("%s: Invalid NULL cam gpio config table\n", __func__);
		return -EFAULT;
	}

	if (gpio_en) {
		msm_gpiomux_install((struct msm_gpiomux_config *)gpio_conf->
			cam_gpiomux_conf_tbl,
			gpio_conf->cam_gpiomux_conf_tbl_size);
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++) {
			rc = gpio_request(gpio_conf->cam_gpio_tbl[i],
				 "CAM_GPIO");
			if (rc < 0) {
				pr_err("%s not able to get gpio\n", __func__);
				for (i--; i >= 0; i--)
					gpio_free(gpio_conf->cam_gpio_tbl[i]);
					break;
			}
		}
	} else {
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++)
			gpio_free(gpio_conf->cam_gpio_tbl[i]);
	}

	if (gpio_conf->cam_gpiomux_conf_tbl	== NULL) {
		pr_info("%s: no cam gpio config table\n", __func__);
		return rc;
	}
#else
	if (gpio_en) {
		msm_gpiomux_install((struct msm_gpiomux_config *)gpio_conf->
			cam_gpiomux_conf_tbl,
			gpio_conf->cam_gpiomux_conf_tbl_size);
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++) {
			rc = gpio_request(gpio_conf->cam_gpio_tbl[i],
				 "CAM_GPIO");
			if (rc < 0) {
				pr_err("%s not able to get gpio\n", __func__);
				for (i--; i >= 0; i--)
					gpio_free(gpio_conf->cam_gpio_tbl[i]);
					break;
			}
		}
	} else {
		for (i = 0; i < gpio_conf->cam_gpio_tbl_size; i++)
			gpio_free(gpio_conf->cam_gpio_tbl[i]);
	}
#endif 
	return rc;
}

void msm_camio_vfe_blk_reset(void)
{
	return;
}

int msm_camio_probe_on(void *s_ctrl)
{
	int rc = 0;
	struct msm_sensor_ctrl_t* sctrl = (struct msm_sensor_ctrl_t *)s_ctrl;
	struct msm_camera_sensor_info *sinfo = sctrl->sensordata;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

	camio_sctrl = sctrl;
	camio_clk = camdev->ioclk;

	pr_info("%s: sinfo sensor name - %s\n", __func__, sinfo->sensor_name);
	pr_info("%s: camio_clk m(%d) v(%d)\n", __func__, camio_clk.mclk_clk_rate, camio_clk.vfe_clk_rate);

	rc = msm_camio_config_gpio_table(sinfo,1);
	if (rc < 0)
		return rc;

	if (camdev && camdev->camera_csi_on)
		rc = camdev->camera_csi_on();
	if (rc < 0)
		pr_info("%s camera_csi_on failed\n", __func__);

	return rc;
}

int msm_camio_probe_off(void *s_ctrl)
{
	int rc = 0;
	struct msm_sensor_ctrl_t* sctrl = (struct msm_sensor_ctrl_t *)s_ctrl;
	struct msm_camera_sensor_info *sinfo = sctrl->sensordata;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

	if (camdev && camdev->camera_csi_off)
		rc = camdev->camera_csi_off();
	if (rc < 0)
		pr_info("%s camera_csi_off failed\n", __func__);

	rc = msm_camio_config_gpio_table(sinfo,0);

	return rc;
}

int msm_camio_probe_on_bootup(void *s_ctrl)
{
	int rc = 0;
	struct msm_sensor_ctrl_t* sctrl = (struct msm_sensor_ctrl_t *)s_ctrl;
	struct msm_camera_sensor_info *sinfo = sctrl->sensordata;
	struct msm_camera_device_platform_data *camdev = sinfo->pdata;

	camio_sctrl = sctrl;
	camio_clk = camdev->ioclk;

	pr_info("%s: sinfo sensor name - %s\n", __func__, sinfo->sensor_name);
	pr_info("%s: camio_clk m(%d) v(%d)\n", __func__, camio_clk.mclk_clk_rate, camio_clk.vfe_clk_rate);

	rc = msm_camio_config_gpio_table(sinfo,1);
	if (rc < 0)
		return rc;

	return rc;
}

int msm_camio_probe_off_bootup(void *s_ctrl)
{
	int rc = 0;
	struct msm_sensor_ctrl_t* sctrl = (struct msm_sensor_ctrl_t *)s_ctrl;
	struct msm_camera_sensor_info *sinfo = sctrl->sensordata;

	rc = msm_camio_config_gpio_table(sinfo,0);

	return rc;
}
#endif	

void msm_camio_bus_scale_cfg(struct msm_bus_scale_pdata *cam_bus_scale_table,
		enum msm_bus_perf_setting perf_setting)
{
	static uint32_t bus_perf_client;
	int rc = 0;
	switch (perf_setting) {
	case S_INIT:
		bus_perf_client =
			msm_bus_scale_register_client(cam_bus_scale_table);
		if (!bus_perf_client) {
			CDBG("%s: Registration Failed!!!\n", __func__);
			bus_perf_client = 0;
			return;
		}
		CDBG("%s: S_INIT rc = %u\n", __func__, bus_perf_client);
		break;
	case S_EXIT:
		if (bus_perf_client) {
			CDBG("%s: S_EXIT\n", __func__);
			msm_bus_scale_unregister_client(bus_perf_client);
		} else
			CDBG("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_PREVIEW:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 1);
			CDBG("%s: S_PREVIEW rc = %d\n", __func__, rc);
		} else
			CDBG("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_VIDEO:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 2);
			CDBG("%s: S_VIDEO rc = %d\n", __func__, rc);
		} else
			CDBG("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_CAPTURE:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 3);
			CDBG("%s: S_CAPTURE rc = %d\n", __func__, rc);
		} else
			CDBG("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_ZSL:
		if (bus_perf_client) {
			rc = msm_bus_scale_client_update_request(
				bus_perf_client, 4);
			CDBG("%s: S_ZSL rc = %d\n", __func__, rc);
		} else
			CDBG("%s: Bus Client NOT Registered!!!\n", __func__);
		break;
	case S_DEFAULT:
		break;
	default:
		pr_warning("%s: INVALID CASE\n", __func__);
	}
}

