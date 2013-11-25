/*
 * Customer HW 4 dependant file
 *
 * Copyright (C) 1999-2012, Broadcom Corporation
 * 
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 * 
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 * 
 *      Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a license
 * other than the GPL, without Broadcom's express prior written consent.
 *
 * $Id: dhd_custom_sec.c 334946 2012-05-24 20:38:00Z $
 */
#ifdef CUSTOMER_HW4
#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>

#include <proto/ethernet.h>
#include <dngl_stats.h>
#include <bcmutils.h>
#include <dhd.h>
#include <dhd_dbg.h>

#include <linux/fcntl.h>
#include <linux/fs.h>

struct dhd_info;
extern int _dhd_set_mac_address(struct dhd_info *dhd,
	int ifidx, struct ether_addr *addr);

struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ]; 
	char custom_locale[WLC_CNTRY_BUF_SZ]; 
	int32 custom_locale_rev; 
};

const struct cntry_locales_custom translate_custom_table[] = {
#ifdef BCM4334_CHIP
	{"",   "XZ", 11},  
#endif
	{"AE", "AE", 1},
	{"AR", "AR", 1},
	{"AT", "AT", 1},
	{"AU", "AU", 2},
	{"BE", "BE", 1},
	{"BG", "BG", 1},
	{"BN", "BN", 1},
	{"CA", "CA", 2},
	{"CH", "CH", 1},
	{"CN", "CN", 0},
	{"CY", "CY", 1},
	{"CZ", "CZ", 1},
	{"DE", "DE", 3},
	{"DK", "DK", 1},
	{"EE", "EE", 1},
	{"ES", "ES", 1},
	{"FI", "FI", 1},
	{"FR", "FR", 1},
	{"GB", "GB", 1},
	{"GR", "GR", 1},
	{"HR", "HR", 1},
	{"HU", "HU", 1},
	{"IE", "IE", 1},
	{"IS", "IS", 1},
	{"IT", "IT", 1},
	{"JP", "JP", 5},
	{"KR", "KR", 24},
	{"KW", "KW", 1},
	{"LI", "LI", 1},
	{"LT", "LT", 1},
	{"LU", "LU", 1},
	{"LV", "LV", 1},
	{"MA", "MA", 1},
	{"MT", "MT", 1},
	{"MX", "MX", 1},
	{"NL", "NL", 1},
	{"NO", "NO", 1},
	{"PL", "PL", 1},
	{"PT", "PT", 1},
	{"PY", "PY", 1},
	{"RO", "RO", 1},
	{"SE", "SE", 1},
	{"SI", "SI", 1},
	{"SK", "SK", 1},
	{"TR", "TR", 7},
	{"TW", "TW", 2},
	{"IR", "XZ", 11},	
	{"SD", "XZ", 11},	
	{"SY", "XZ", 11},	
	{"GL", "XZ", 11},	
	{"PS", "XZ", 11},	
	{"TL", "XZ", 11},	
	{"MH", "XZ", 11},	
	{"PK", "XZ", 11},	
#ifdef BCM4334_CHIP
	{"RU", "RU", 5},
	{"SG", "SG", 4},
	{"US", "US", 46}
#endif
#ifdef BCM4330_CHIP
	{"RU", "RU", 1},
	{"US", "US", 5}
#endif
};

void get_customized_country_code(char *country_iso_code, wl_country_t *cspec)
{
	int size, i;

	size = ARRAYSIZE(translate_custom_table);

	if (cspec == 0)
		 return;

	if (size == 0)
		 return;

	for (i = 0; i < size; i++) {
		if (strcmp(country_iso_code, translate_custom_table[i].iso_abbrev) == 0) {
			memcpy(cspec->ccode,
				translate_custom_table[i].custom_locale, WLC_CNTRY_BUF_SZ);
			cspec->rev = translate_custom_table[i].custom_locale_rev;
			return;
		}
	}
	return;
}

#ifdef SLP_PATH
#define CIDINFO "/opt/etc/.cid.info"
#define PSMINFO "/opt/etc/.psm.info"
#define MACINFO "/opt/etc/.mac.info"
#define MACINFO_EFS NULL
#define	REVINFO "/data/.rev"
#else
#define MACINFO "/data/.mac.info"
#define MACINFO_EFS "/efs/wifi/.mac.info"
#define NVMACINFO "/data/.nvmac.info"
#define	REVINFO "/data/.rev"
#define CIDINFO "/data/.cid.info"
#define PSMINFO "/data/.psm.info"
#endif 

#ifdef BCM4330_CHIP
#define CIS_BUF_SIZE		128
#elif defined(BCM4334_CHIP)
#define CIS_BUF_SIZE		256
#else
#define CIS_BUF_SIZE		512
#endif 

#ifdef READ_MACADDR
int dhd_read_macaddr(struct dhd_info *dhd, struct ether_addr *mac)
{
	struct file *fp      = NULL;
	char macbuffer[18]   = {0};
	mm_segment_t oldfs   = {0};
	char randommac[3]    = {0};
	char buf[18]         = {0};
	char *filepath_efs       = MACINFO_EFS;
#ifdef CONFIG_TARGET_LOCALE_VZW
	char *nvfilepath       = "/data/misc/wifi/.nvmac.info";
#else
	char *nvfilepath       = NVMACINFO;
#endif
	int ret = 0;

		fp = filp_open(filepath_efs, O_RDONLY, 0);
		if (IS_ERR(fp)) {
start_readmac:
			
			fp = filp_open(filepath_efs, O_RDWR | O_CREAT, 0666);
			if (IS_ERR(fp)) {
			DHD_ERROR(("[WIFI] %s: File open error\n", filepath_efs));
				return -1;
			}
			oldfs = get_fs();
			set_fs(get_ds());

			
			get_random_bytes(randommac, 3);

			sprintf(macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X\n",
				0x00, 0x12, 0x34, randommac[0], randommac[1], randommac[2]);
			DHD_ERROR(("[WIFI]The Random Generated MAC ID: %s\n", macbuffer));

			if (fp->f_mode & FMODE_WRITE) {
			ret = fp->f_op->write(fp, (const char *)macbuffer,
				sizeof(macbuffer), &fp->f_pos);
				if (ret < 0)
				DHD_ERROR(("[WIFI]MAC address [%s] Failed to write into File: %s\n",
					macbuffer, filepath_efs));
				else
				DHD_ERROR(("[WIFI]MAC address [%s] written into File: %s\n",
					macbuffer, filepath_efs));
			}
			set_fs(oldfs);
		ret = kernel_read(fp, 0, buf, 18);
	} else {
		ret = kernel_read(fp, 0, buf, 18);
		buf[17] = '\0';
		DHD_ERROR(("Read MAC : [%s] [%d] \r\n",
			buf, strncmp(buf, "00:00:00:00:00:00", 17)));
		if (strncmp(buf, "00:00:00:00:00:00", 17) < 1) {
			DHD_ERROR(("goto start_readmac \r\n"));
			filp_close(fp, NULL);
			goto start_readmac;
		}
	}

	if (ret)
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned int *)&(mac->octet[0]), (unsigned int *)&(mac->octet[1]),
			(unsigned int *)&(mac->octet[2]), (unsigned int *)&(mac->octet[3]),
			(unsigned int *)&(mac->octet[4]), (unsigned int *)&(mac->octet[5]));
	else
		DHD_ERROR(("dhd_bus_start: Reading from the '%s' returns 0 bytes\n", filepath_efs));

	if (fp)
		filp_close(fp, NULL);

	
	if (_dhd_set_mac_address(dhd, 0, mac) == 0)
		DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
	else
		DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() failed\n"));

	return 0;
}
#endif 

#ifdef RDWR_MACADDR
static int g_imac_flag;

enum {
	MACADDR_NONE = 0,
	MACADDR_MOD,
	MACADDR_MOD_RANDOM,
	MACADDR_MOD_NONE,
	MACADDR_COB,
	MACADDR_COB_RANDOM
};

int dhd_write_rdwr_macaddr(struct ether_addr *mac)
{
	char *filepath_data = MACINFO;
	char *filepath_efs = MACINFO_EFS;
	struct file *fp_mac = NULL;
	char buf[18]      = {0};
	mm_segment_t oldfs    = {0};
	int ret = -1;

	if ((g_imac_flag != MACADDR_COB) && (g_imac_flag != MACADDR_MOD))
		return 0;

	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X\n",
		mac->octet[0], mac->octet[1], mac->octet[2],
		mac->octet[3], mac->octet[4], mac->octet[5]);

	
	fp_mac = filp_open(filepath_data, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath_data));
		return -1;
	}	else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf,
				sizeof(buf), &fp_mac->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed"
				" to write into File: %s\n", buf, filepath_data));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written"
				" into File: %s\n", buf, filepath_data));
		}
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}
	
	fp_mac = filp_open(filepath_efs, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath_efs));
		return -1;
	}	else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf,
				sizeof(buf), &fp_mac->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed"
				" to write into File: %s\n", buf, filepath_efs));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written"
				" into File: %s\n", buf, filepath_efs));
		}
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}

	return 0;

}

int dhd_check_rdwr_macaddr(struct dhd_info *dhd, dhd_pub_t *dhdp,
	struct ether_addr *mac)
{
	struct file *fp_mac = NULL;
	struct file *fp_nvm = NULL;
	char macbuffer[18]    = {0};
	char randommac[3]   = {0};
	char buf[18]      = {0};
	char *filepath_data      = MACINFO;
	char *filepath_efs      = MACINFO_EFS;
#ifdef CONFIG_TARGET_LOCALE_NA
	char *nvfilepath       = "/data/misc/wifi/.nvmac.info";
#else
	char *nvfilepath = NVMACINFO;
#endif
	char cur_mac[128]   = {0};
	char dummy_mac[ETHER_ADDR_LEN] = {0x00, 0x90, 0x4C, 0xC5, 0x12, 0x38};
	char cur_macbuffer[18]  = {0};
	int ret = -1;

	g_imac_flag = MACADDR_NONE;

	fp_nvm = filp_open(nvfilepath, O_RDONLY, 0);
	if (IS_ERR(fp_nvm)) { 

		
		fp_nvm = filp_open(nvfilepath, O_RDWR | O_CREAT, 0666);
		if (!IS_ERR(fp_nvm))
			filp_close(fp_nvm, NULL);

		
		strcpy(cur_mac, "cur_etheraddr");
		ret = dhd_wl_ioctl_cmd(dhdp, WLC_GET_VAR, cur_mac,
			sizeof(cur_mac), 0, 0);
		if (ret < 0) {
			DHD_ERROR(("Current READ MAC error \r\n"));
			memset(cur_mac, 0, ETHER_ADDR_LEN);
			return -1;
		} else {
			DHD_ERROR(("MAC (OTP) : "
			"[%02X:%02X:%02X:%02X:%02X:%02X] \r\n",
			cur_mac[0], cur_mac[1], cur_mac[2], cur_mac[3],
			cur_mac[4], cur_mac[5]));
		}

		sprintf(cur_macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X\n",
			cur_mac[0], cur_mac[1], cur_mac[2],
			cur_mac[3], cur_mac[4], cur_mac[5]);

		fp_mac = filp_open(filepath_data, O_RDONLY, 0);
		if (IS_ERR(fp_mac)) { 
			
			if (memcmp(cur_mac, dummy_mac, ETHER_ADDR_LEN) == 0)
				g_imac_flag = MACADDR_MOD_RANDOM;
			else if (strncmp(buf, "00:00:00:00:00:00", 17) == 0)
				g_imac_flag = MACADDR_MOD_RANDOM;
			else
				g_imac_flag = MACADDR_MOD;
		} else {
			int is_zeromac;

			ret = kernel_read(fp_mac, 0, buf, 18);
			filp_close(fp_mac, NULL);
			buf[17] = '\0';

			is_zeromac = strncmp(buf, "00:00:00:00:00:00", 17);
			DHD_ERROR(("MAC (FILE): [%s] [%d] \r\n",
				buf, is_zeromac));

			if (is_zeromac == 0) {
				DHD_ERROR(("Zero MAC detected."
					" Trying Random MAC.\n"));
				g_imac_flag = MACADDR_MOD_RANDOM;
			} else {
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned int *)&(mac->octet[0]),
					(unsigned int *)&(mac->octet[1]),
					(unsigned int *)&(mac->octet[2]),
					(unsigned int *)&(mac->octet[3]),
					(unsigned int *)&(mac->octet[4]),
					(unsigned int *)&(mac->octet[5]));
			
				if (memcmp(cur_mac, mac->octet, ETHER_ADDR_LEN) == 0) {
					g_imac_flag = MACADDR_NONE;
				} else { 
					if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
						DHD_INFO(("%s: MACID is"
						" overwritten\n", __FUNCTION__));
						g_imac_flag = MACADDR_MOD;
					} else {
						DHD_ERROR(("%s: "
						"_dhd_set_mac_address()"
						" failed\n", __FUNCTION__));
						g_imac_flag = MACADDR_NONE;
					}
				}
			}
		}
		fp_mac = filp_open(filepath_efs, O_RDONLY, 0);
		if (IS_ERR(fp_mac)) { 
			
			if (memcmp(cur_mac, dummy_mac, ETHER_ADDR_LEN) == 0)
				g_imac_flag = MACADDR_MOD_RANDOM;
			else if (strncmp(buf, "00:00:00:00:00:00", 17) == 0)
				g_imac_flag = MACADDR_MOD_RANDOM;
			else
				g_imac_flag = MACADDR_MOD;
		} else {
			int is_zeromac;

			ret = kernel_read(fp_mac, 0, buf, 18);
			filp_close(fp_mac, NULL);
			buf[17] = '\0';

			is_zeromac = strncmp(buf, "00:00:00:00:00:00", 17);
			DHD_ERROR(("MAC (FILE): [%s] [%d] \r\n",
				buf, is_zeromac));

			if (is_zeromac == 0) {
				DHD_ERROR(("Zero MAC detected."
					" Trying Random MAC.\n"));
				g_imac_flag = MACADDR_MOD_RANDOM;
			} else {
				sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
					(unsigned int *)&(mac->octet[0]),
					(unsigned int *)&(mac->octet[1]),
					(unsigned int *)&(mac->octet[2]),
					(unsigned int *)&(mac->octet[3]),
					(unsigned int *)&(mac->octet[4]),
					(unsigned int *)&(mac->octet[5]));
			
				if (memcmp(cur_mac, mac->octet, ETHER_ADDR_LEN) == 0) {
					g_imac_flag = MACADDR_NONE;
				} else { 
					if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
						DHD_INFO(("%s: MACID is"
						" overwritten\n", __FUNCTION__));
						g_imac_flag = MACADDR_MOD;
					} else {
						DHD_ERROR(("%s: "
						"_dhd_set_mac_address()"
						" failed\n", __FUNCTION__));
						g_imac_flag = MACADDR_NONE;
					}
				}
			}
		}
	} else {
		
		ret = kernel_read(fp_nvm, 0, buf, 18);

		buf[17] = '\0';
		DHD_ERROR(("Read MAC : [%s] [%d] \r\n", buf,
			strncmp(buf, "00:00:00:00:00:00", 17)));
		if ((buf[0] == '\0') ||
			(strncmp(buf, "00:00:00:00:00:00", 17) == 0)) {
			g_imac_flag = MACADDR_COB_RANDOM;
		} else {
			sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
				(unsigned int *)&(mac->octet[0]),
				(unsigned int *)&(mac->octet[1]),
				(unsigned int *)&(mac->octet[2]),
				(unsigned int *)&(mac->octet[3]),
				(unsigned int *)&(mac->octet[4]),
				(unsigned int *)&(mac->octet[5]));
			
			if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
				DHD_INFO(("%s: MACID is overwritten\n",
					__FUNCTION__));
				g_imac_flag = MACADDR_COB;
			} else {
				DHD_ERROR(("%s: _dhd_set_mac_address()"
					" failed\n", __FUNCTION__));
			}
		}
		filp_close(fp_nvm, NULL);
	}

	if ((g_imac_flag == MACADDR_COB_RANDOM) ||
	    (g_imac_flag == MACADDR_MOD_RANDOM)) {
		get_random_bytes(randommac, 3);
		sprintf(macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X\n",
			0x60, 0xd0, 0xa9, randommac[0], randommac[1],
			randommac[2]);
		DHD_ERROR(("[WIFI] The Random Generated MAC ID : %s\n",
			macbuffer));
		sscanf(macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned int *)&(mac->octet[0]),
			(unsigned int *)&(mac->octet[1]),
			(unsigned int *)&(mac->octet[2]),
			(unsigned int *)&(mac->octet[3]),
			(unsigned int *)&(mac->octet[4]),
			(unsigned int *)&(mac->octet[5]));
		if (_dhd_set_mac_address(dhd, 0, mac) == 0) {
			DHD_INFO(("%s: MACID is overwritten\n", __FUNCTION__));
			g_imac_flag = MACADDR_COB;
		} else {
			DHD_ERROR(("%s: _dhd_set_mac_address() failed\n",
				__FUNCTION__));
		}
	}

	return 0;
}
#endif 

#ifdef RDWR_KORICS_MACADDR
int dhd_write_rdwr_korics_macaddr(struct dhd_info *dhd, struct ether_addr *mac)
{
	struct file *fp      = NULL;
	char macbuffer[18]   = {0};
	mm_segment_t oldfs   = {0};
	char randommac[3]    = {0};
	char buf[18]         = {0};
	char *filepath_efs       = MACINFO_EFS;
	int is_zeromac       = 0;
	int ret = 0;
	
	fp = filp_open(filepath_efs, O_RDONLY, 0);

	if (IS_ERR(fp)) {
		
		fp = filp_open(filepath_efs, O_RDWR | O_CREAT, 0666);
		if (IS_ERR(fp)) {
			DHD_ERROR(("[WIFI] %s: File open error\n",
				filepath_efs));
			return -1;
		}

		oldfs = get_fs();
		set_fs(get_ds());

		get_random_bytes(randommac, 3);

		sprintf(macbuffer, "%02X:%02X:%02X:%02X:%02X:%02X\n",
			0x60, 0xd0, 0xa9, randommac[0],
			randommac[1], randommac[2]);
		DHD_ERROR(("[WIFI] The Random Generated MAC ID : %s\n",
			macbuffer));

		if (fp->f_mode & FMODE_WRITE) {
			ret = fp->f_op->write(fp,
				(const char *)macbuffer,
				sizeof(macbuffer), &fp->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s]"
					" Failed to write into File:"
					" %s\n", macbuffer, filepath_efs));
			else
				DHD_ERROR(("[WIFI] Mac address [%s]"
					" written into File: %s\n",
					macbuffer, filepath_efs));
		}
		set_fs(oldfs);
	} else {
	    ret = kernel_read(fp, 0, buf, 18);
		buf[17] = '\0';
		
		if ((buf[0] == '\0') ||
			(strncmp(buf, "00:00:00:00:00:00", 17) == 0)) {
			is_zeromac = 1;
		}
	}

	if (ret)
		sscanf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
			(unsigned int *)&(mac->octet[0]),
			(unsigned int *)&(mac->octet[1]),
			(unsigned int *)&(mac->octet[2]),
			(unsigned int *)&(mac->octet[3]),
			(unsigned int *)&(mac->octet[4]),
			(unsigned int *)&(mac->octet[5]));
	else
		DHD_INFO(("dhd_bus_start: Reading from the"
			" '%s' returns 0 bytes\n", filepath_efs));

	if (fp)
		filp_close(fp, NULL);

	if (!is_zeromac) {
		
		if (_dhd_set_mac_address(dhd, 0, mac) == 0)
			DHD_INFO(("dhd_bus_start: MACID is overwritten\n"));
		else
			DHD_ERROR(("dhd_bus_start: _dhd_set_mac_address() "
				"failed\n"));
	} else {
		DHD_ERROR(("dhd_bus_start:Is ZeroMAC BypassWrite.mac.info!\n"));
	}

	return 0;
}
#endif 

#ifdef USE_CID_CHECK
static int dhd_write_cid_file(const char *filepath_efs, const char *buf, int buf_len)
{
	struct file *fp = NULL;
	mm_segment_t oldfs = {0};
	int ret = 0;

	
	fp = filp_open(filepath_efs, O_RDWR | O_CREAT, 0666);
	if (IS_ERR(fp)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath_efs));
		return -1;
	} else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp->f_mode & FMODE_WRITE) {
			ret = fp->f_op->write(fp, buf, buf_len, &fp->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Failed to write CIS[%s]"
					" into '%s'\n", buf, filepath_efs));
			else
				DHD_ERROR(("[WIFI] CID [%s] written into"
					" '%s'\n", buf, filepath_efs));
		}
		set_fs(oldfs);
	}
	filp_close(fp, NULL);

	return 0;
}

#ifdef DUMP_CIS
static void dhd_dump_cis(const unsigned char *buf, int size)
{
	int i;
	for (i = 0; i < size; i++) {
		DHD_ERROR(("%02X ", buf[i]));
		if ((i % 15) == 15) DHD_ERROR(("\n"));
	}
	DHD_ERROR(("\n"));
}
#endif 

#define MAX_VID_LEN		8
#define MAX_VNAME_LEN		16
#define CIS_TUPLE_START		0x80
#define CIS_TUPLE_VENDOR	0x81

typedef struct {
	uint8 vid_length;
	unsigned char vid[MAX_VID_LEN];
	char vname[MAX_VNAME_LEN];
} vid_info_t;

#if defined(BCM4330_CHIP)
vid_info_t vid_info[] = {
	{6, {0x00, 0x20, 0xc7, 0x00, 0x00, }, {"murata"}},
	{2, {0x99, }, {"semcove"}},
	{0, {0x00, }, {"samsung"}}
};
#elif defined(BCM4334_CHIP)
vid_info_t vid_info[] = {
	{3, {0x33, 0x33, }, {"semco"}},
	{3, {0xfb, 0x50, }, {"semcosh"}},
	{6, {0x00, 0x20, 0xc7, 0x00, 0x00, }, {"murata"}},
	{0, {0x00, }, {"samsung"}}
};
#else
vid_info_t vid_info[] = {
	{3, {0x33, 0x33, }, {"semcosh"}},
	{3, {0x33, 0x66, }, {"semco"}},
	{3, {0x33, 0x88, }, {"semco3rd"}},
	{6, {0x00, 0x20, 0xc7, 0x00, 0x00, }, {"murata"}},
	{0, {0x00, }, {"murata"}}
};
#endif 

int dhd_check_module_cid(dhd_pub_t *dhd)
{
	int ret = -1;
	unsigned char cis_buf[CIS_BUF_SIZE] = {0};
	const char *cidfilepath = CIDINFO;
	cis_rw_t *cish = (cis_rw_t *)&cis_buf[8];
	int idx, max;
	vid_info_t *cur_info;
	unsigned char *vid_start;
	unsigned char vid_length;
#if defined(BCM4334_CHIP)
	int flag_b3;
	const char *revfilepath = REVINFO;
#endif

	
	cish->source = 0;
	cish->byteoff = 0;
	cish->nbytes = sizeof(cis_buf);

	strcpy(cis_buf, "cisdump");
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, cis_buf,
		sizeof(cis_buf), 0, 0);
	if (ret < 0) {
		DHD_ERROR(("%s: CIS reading failed, err=%d\n",
			__FUNCTION__, ret));
		return ret;
	}

	DHD_ERROR(("%s: CIS reading success, ret=%d\n",
		__FUNCTION__, ret));
#ifdef DUMP_CIS
	dhd_dump_cis(cis_buf, 48);
#endif

	max = sizeof(cis_buf) - 4 ;
	for (idx = 0; idx < max; idx++) {
		if (cis_buf[idx] == CIS_TUPLE_START &&
			cis_buf[idx + 2] == CIS_TUPLE_VENDOR) {
			vid_length = cis_buf[idx + 1];
			vid_start = &cis_buf[idx + 3];
			
			break;
		}
	}

	if (idx < max) {
		max = sizeof(vid_info) / sizeof(vid_info_t);
		for (idx = 0; idx < max; idx++) {
			cur_info = &vid_info[idx];
			if ((cur_info->vid_length == vid_length) &&
				(memcmp(cur_info->vid, vid_start,
				cur_info->vid_length - 1) == 0))
				goto write_cid;
		}
	}

	
	DHD_ERROR(("%s: cannot find CIS TUPLE set as default\n", __FUNCTION__));
	max = sizeof(vid_info) / sizeof(vid_info_t);
	for (idx = 0; idx < max; idx++) {
		cur_info = &vid_info[idx];
		if (cur_info->vid_length == 0)
			goto write_cid;
	}
	DHD_ERROR(("%s: cannot find default CID\n", __FUNCTION__));
	return -1;

write_cid:
	DHD_ERROR(("CIS MATCH FOUND : %s\n", cur_info->vname));
	dhd_write_cid_file(cidfilepath, cur_info->vname, strlen(cur_info->vname)+1);
#if defined(BCM4334_CHIP)
	
	memset(cis_buf, 0, sizeof(cis_buf));
	cish = (cis_rw_t *)&cis_buf[8];

	cish->source = 0;
	cish->byteoff = 0;
	cish->nbytes = sizeof(cis_buf);

	strcpy(cis_buf, "otpdump");
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, cis_buf,
		sizeof(cis_buf), 0, 0);
	if (ret < 0) {
		DHD_ERROR(("%s: OTP reading failed, err=%d\n",
			__FUNCTION__, ret));
		return ret;
	}

	
	cis_buf[34] = '\0';
	flag_b3 = bcm_atoi(&cis_buf[33]);
	if (flag_b3 & 0x1) {
		DHD_ERROR(("REV MATCH FOUND : 4334B3, %c\n", cis_buf[33]));
		dhd_write_cid_file(revfilepath, "4334B3", 6);
	}
#endif 

	return ret;
}
#endif 

#ifdef GET_MAC_FROM_OTP
static int dhd_write_mac_file(const char *filepath, const char *buf, int buf_len)
{
	struct file *fp = NULL;
	mm_segment_t oldfs = {0};
	int ret = 0;

	fp = filp_open(filepath, O_RDWR | O_CREAT, 0666);
	
	if (IS_ERR(fp)) {
		DHD_ERROR(("[WIFI] File open error\n"));
		return -1;
	} else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp->f_mode & FMODE_WRITE) {
			ret = fp->f_op->write(fp, buf, buf_len, &fp->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Failed to write CIS. \n"));
			else
				DHD_ERROR(("[WIFI] MAC written. \n"));
		}
		set_fs(oldfs);
	}
	filp_close(fp, NULL);

	return 0;
}

#define CIS_MAC_OFFSET 33

int dhd_check_module_mac(dhd_pub_t *dhd)
{
	int ret = -1;
	unsigned char cis_buf[CIS_BUF_SIZE] = {0};
	unsigned char mac_buf[20] = {0};
	unsigned char otp_mac_buf[20] = {0};
	const char *macfilepath = MACINFO_EFS;

	
	cis_rw_t *cish = (cis_rw_t *)&cis_buf[8];
	struct file *fp_mac = NULL;

	cish->source = 0;
	cish->byteoff = 0;
	cish->nbytes = sizeof(cis_buf);

	strcpy(cis_buf, "cisdump");
	ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, cis_buf,
		sizeof(cis_buf), 0, 0);
	if (ret < 0) {
		DHD_TRACE(("%s: CIS reading failed, err=%d\n", __func__,
			ret));
		return ret;
	} else {
		unsigned char mac_id[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
#ifdef DUMP_CIS
		dump_cis(cis_buf, 48);
#endif
		mac_id[0] = cis_buf[CIS_MAC_OFFSET];
		mac_id[1] = cis_buf[CIS_MAC_OFFSET + 1];
		mac_id[2] = cis_buf[CIS_MAC_OFFSET + 2];
		mac_id[3] = cis_buf[CIS_MAC_OFFSET + 3];
		mac_id[4] = cis_buf[CIS_MAC_OFFSET + 4];
		mac_id[5] = cis_buf[CIS_MAC_OFFSET + 5];

		sprintf(otp_mac_buf, "%02X:%02X:%02X:%02X:%02X:%02X\n",
			mac_id[0], mac_id[1], mac_id[2], mac_id[3], mac_id[4],
			mac_id[5]);
		DHD_ERROR(("[WIFI]mac_id is setted from OTP \n"));
	}

	fp_mac = filp_open(macfilepath, O_RDONLY, 0);
	if (!IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI]Check Mac address in .mac.info \n"));
		kernel_read(fp_mac, fp_mac->f_pos, mac_buf, sizeof(mac_buf));

		if (strncmp(mac_buf, otp_mac_buf, 17) != 0) {
			DHD_ERROR(("[WIFI]file MAC is wrong. Write OTP MAC in .mac.info \n"));
			dhd_write_mac_file(macfilepath, otp_mac_buf, sizeof(otp_mac_buf));
		}
	}

	return ret;
}
#endif 

#ifdef WRITE_MACADDR
int dhd_write_macaddr(struct ether_addr *mac)
{
	char *filepath_data      = MACINFO;
	char *filepath_efs      = MACINFO_EFS;

	struct file *fp_mac = NULL;
	char buf[18]      = {0};
	mm_segment_t oldfs    = {0};
	int ret = -1;
	int retry_count = 0;

startwrite:

	sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X\n",
		mac->octet[0], mac->octet[1], mac->octet[2],
		mac->octet[3], mac->octet[4], mac->octet[5]);

	
	fp_mac = filp_open(filepath_data, O_RDWR | O_CREAT, 0666);

	if (IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath_data));
		return -1;
	} else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf,
				sizeof(buf), &fp_mac->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed to"
				" write into File: %s\n", buf, filepath_data));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written"
				" into File: %s\n", buf, filepath_data));
		}
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}
	
	fp_mac = filp_open(filepath_data, O_RDONLY, 0);
	ret = kernel_read(fp_mac, 0, buf, 18);

	if ((ret == 0) && (retry_count++ < 3)) {
		filp_close(fp_mac, NULL);
		goto startwrite;
	}

	filp_close(fp_mac, NULL);
	

	if (filepath_efs == NULL) {
		DHD_ERROR(("[WIFI]%s : no efs filepath", __func__));
		return 0;
	}

	
	fp_mac = filp_open(filepath_efs, O_RDWR | O_CREAT, 0666);

	if (IS_ERR(fp_mac)) {
		DHD_ERROR(("[WIFI] %s: File open error\n", filepath_efs));
		return -1;
	} else {
		oldfs = get_fs();
		set_fs(get_ds());

		if (fp_mac->f_mode & FMODE_WRITE) {
			ret = fp_mac->f_op->write(fp_mac, (const char *)buf,
				sizeof(buf), &fp_mac->f_pos);
			if (ret < 0)
				DHD_ERROR(("[WIFI] Mac address [%s] Failed to"
				" write into File: %s\n", buf, filepath_efs));
			else
				DHD_INFO(("[WIFI] Mac address [%s] written"
				" into File: %s\n", buf, filepath_efs));
		}
		set_fs(oldfs);
		filp_close(fp_mac, NULL);
	}

	
	fp_mac = filp_open(filepath_efs, O_RDONLY, 0);
	ret = kernel_read(fp_mac, 0, buf, 18);

	if ((ret == 0) && (retry_count++ < 3)) {
		filp_close(fp_mac, NULL);
		goto startwrite;
	}

	filp_close(fp_mac, NULL);

	return 0;
}
#endif 

#ifdef CONFIG_CONTROL_PM
extern bool g_pm_control;
void sec_control_pm(dhd_pub_t *dhd, uint *power_mode)
{
	struct file *fp = NULL;
	char *filepath = PSMINFO;
	mm_segment_t oldfs = {0};
	char power_val = 0;
	char iovbuf[WL_EVENTING_MASK_LEN + 12];

	g_pm_control = FALSE;

	fp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		
		dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode,
			sizeof(uint), TRUE, 0);

		fp = filp_open(filepath, O_RDWR | O_CREAT, 0666);
		if (IS_ERR(fp) || (fp == NULL)) {
			DHD_ERROR(("[%s, %d] /data/.psm.info open failed\n",
				__FUNCTION__, __LINE__));
			return;
		} else {
			oldfs = get_fs();
			set_fs(get_ds());

			if (fp->f_mode & FMODE_WRITE) {
				power_val = '1';
				fp->f_op->write(fp, (const char *)&power_val,
					sizeof(char), &fp->f_pos);
			}
			set_fs(oldfs);
		}
	} else {
		if (fp == NULL) {
			DHD_ERROR(("[%s, %d] /data/.psm.info open failed\n",
				__FUNCTION__, __LINE__));
			return;
		}
		kernel_read(fp, fp->f_pos, &power_val, 1);
		DHD_ERROR(("POWER_VAL = %c \r\n", power_val));

		if (power_val == '0') {
#ifdef ROAM_ENABLE
			uint roamvar = 1;
#endif
			*power_mode = PM_OFF;
			
			dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode,
				sizeof(uint), TRUE, 0);
			
			bcm_mkiovar("mpc", (char *)power_mode, 4,
				iovbuf, sizeof(iovbuf));
			dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf,
				sizeof(iovbuf), TRUE, 0);
			g_pm_control = TRUE;
#ifdef ROAM_ENABLE
			
			bcm_mkiovar("roam_off", (char *)&roamvar, 4,
				iovbuf, sizeof(iovbuf));
			dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf,
				sizeof(iovbuf), TRUE, 0);
#endif
		} else {
			dhd_wl_ioctl_cmd(dhd, WLC_SET_PM, (char *)power_mode,
				sizeof(uint), TRUE, 0);
		}
	}

	if (fp)
		filp_close(fp, NULL);
}
#endif 
#ifdef GLOBALCONFIG_WLAN_COUNTRY_CODE
int dhd_customer_set_country(dhd_pub_t *dhd)
{
	struct file *fp = NULL;
	char *filepath = "/data/.ccode.info";
	char iovbuf[WL_EVENTING_MASK_LEN + 12] = {0};
	char buffer[10] = {0};
	int ret = 0;
	wl_country_t cspec;
	int buf_len = 0;
	char country_code[WLC_CNTRY_BUF_SZ];
	int country_rev;
	int country_offset;
	int country_code_size;
	char country_rev_buf[WLC_CNTRY_BUF_SZ];
	fp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		DHD_ERROR(("%s: %s open failed\n", __FUNCTION__, filepath));
		return -1;
	} else {
		if (kernel_read(fp, 0, buffer, sizeof(buffer))) {
			memset(&cspec, 0, sizeof(cspec));
			memset(country_code, 0, sizeof(country_code));
			memset(country_rev_buf, 0, sizeof(country_rev_buf));
			country_offset = strcspn(buffer, " ");
			country_code_size = country_offset;
			if (country_offset != 0) {
				strncpy(country_code, buffer, country_offset);
				strncpy(country_rev_buf, buffer+country_offset+1,
					strlen(buffer) - country_code_size + 1);
				country_rev = bcm_atoi(country_rev_buf);
				buf_len = bcm_mkiovar("country", (char *)&cspec,
					sizeof(cspec), iovbuf, sizeof(iovbuf));
				ret = dhd_wl_ioctl_cmd(dhd, WLC_GET_VAR, iovbuf, buf_len, FALSE, 0);
				memcpy((void *)&cspec, iovbuf, sizeof(cspec));
				if (!ret) {
					DHD_ERROR(("%s: get country ccode:%s"
						" country_abrev:%s rev:%d  \n",
						__FUNCTION__, cspec.ccode,
						cspec.country_abbrev, cspec.rev));
					if ((strncmp(country_code, cspec.ccode,
						WLC_CNTRY_BUF_SZ) != 0) ||
						(cspec.rev != country_rev)) {
						strncpy(cspec.country_abbrev,
							country_code, country_code_size);
						strncpy(cspec.ccode, country_code,
							country_code_size);
						cspec.rev = country_rev;
						DHD_ERROR(("%s: set country ccode:%s"
							"country_abrev:%s rev:%d\n",
							__FUNCTION__, cspec.ccode,
							cspec.country_abbrev, cspec.rev));
						buf_len = bcm_mkiovar("country", (char *)&cspec,
							sizeof(cspec), iovbuf, sizeof(iovbuf));
						ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR,
							iovbuf, buf_len, TRUE, 0);
					}
				}
			} else {
				DHD_ERROR(("%s: set country %s failed code \n",
					__FUNCTION__, country_code));
				ret = -1;
			}
		} else {
			DHD_ERROR(("%s: Reading from the '%s' returns 0 bytes \n",
				__FUNCTION__, filepath));
			ret = -1;
		}
	}
	if (fp)
		filp_close(fp, NULL);

	return ret;
}
#endif 

#ifdef MIMO_ANT_SETTING
int dhd_sel_ant_from_file(dhd_pub_t *dhd)
{
	struct file *fp = NULL;
	int ret = -1;
	uint32 ant_val = 0;
	char *filepath = "/data/.ant.info";
	char iovbuf[WLC_IOCTL_SMLEN];

	
	fp = filp_open(filepath, O_RDONLY, 0);
	if (IS_ERR(fp)) {
		DHD_ERROR(("[WIFI] %s: File [%s] open error\n", __FUNCTION__, filepath));
		return ret;
	} else {
		ret = kernel_read(fp, 0, (char *)&ant_val, 4);
		if (ret < 0) {
			DHD_ERROR(("[WIFI] %s: File read error, ret=%d\n", __FUNCTION__, ret));
			filp_close(fp, NULL);
			return ret;
		}

		ant_val = bcm_atoi((char *)&ant_val);

		DHD_ERROR(("[WIFI] %s: ANT val = %d\n", __FUNCTION__, ant_val));
		filp_close(fp, NULL);

		
		if (ant_val < 1 || ant_val > 3) {
			DHD_ERROR(("[WIFI] %s: Invalid value %d read from the file %s\n",
				__FUNCTION__, ant_val, filepath));
			return -1;
		}
	}

	
	bcm_mkiovar("txchain", (char *)&ant_val, 4, iovbuf, sizeof(iovbuf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
	if (ret) {
		DHD_ERROR(("[WIFI] %s: Fail to execute dhd_wl_ioctl_cmd(): txchain, ret=%d\n",
			__FUNCTION__, ret));
		return ret;
	}

	bcm_mkiovar("rxchain", (char *)&ant_val, 4, iovbuf, sizeof(iovbuf));
	ret = dhd_wl_ioctl_cmd(dhd, WLC_SET_VAR, iovbuf, sizeof(iovbuf), TRUE, 0);
	if (ret) {
		DHD_ERROR(("[WIFI] %s: Fail to execute dhd_wl_ioctl_cmd(): rxchain, ret=%d\n",
			__FUNCTION__, ret));
		return ret;
	}

	return 0;
}
#endif 
#endif 
