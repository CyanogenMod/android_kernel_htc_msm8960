/*
* Customer code to add GPIO control during WLAN start/stop
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
* $Id: dhd_custom_gpio.c 353280 2012-08-26 04:33:17Z $
*/

#include <typedefs.h>
#include <linuxver.h>
#include <osl.h>
#include <bcmutils.h>

#include <dngl_stats.h>
#include <dhd.h>

#include <wlioctl.h>
#include <wl_iw.h>

#define WL_ERROR(x) printf x
#define WL_TRACE(x)

#ifdef CUSTOMER_HW
extern  void bcm_wlan_power_off(int);
extern  void bcm_wlan_power_on(int);
#endif 
#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
#ifdef CONFIG_WIFI_CONTROL_FUNC
int wifi_set_power(int on, unsigned long msec);
int wifi_get_irq_number(unsigned long *irq_flags_ptr);
int wifi_get_mac_addr(unsigned char *buf);
void *wifi_get_country_code(char *ccode);
#else
int wifi_set_power(int on, unsigned long msec) { return -1; }
int wifi_get_irq_number(unsigned long *irq_flags_ptr) { return -1; }
int wifi_get_mac_addr(unsigned char *buf) { return -1; }
void *wifi_get_country_code(char *ccode) { return NULL; }
#endif 
#endif 

#if defined(OOB_INTR_ONLY) || defined(BCMSPI_ANDROID)

#if defined(BCMLXSDMMC)
extern int sdioh_mmc_irq(int irq);
#endif 

#ifdef CUSTOMER_HW3
#include <mach/gpio.h>
#endif

static int dhd_oob_gpio_num = -1;

module_param(dhd_oob_gpio_num, int, 0644);
MODULE_PARM_DESC(dhd_oob_gpio_num, "DHD oob gpio number");

int dhd_customer_oob_irq_map(unsigned long *irq_flags_ptr)
{
	int  host_oob_irq = 0;

#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
	host_oob_irq = wifi_get_irq_number(irq_flags_ptr);

#else
#if defined(CUSTOM_OOB_GPIO_NUM)
	if (dhd_oob_gpio_num < 0) {
		dhd_oob_gpio_num = CUSTOM_OOB_GPIO_NUM;
	}
#endif 

	if (dhd_oob_gpio_num < 0) {
		WL_ERROR(("%s: ERROR customer specific Host GPIO is NOT defined \n",
		__FUNCTION__));
		return (dhd_oob_gpio_num);
	}

	WL_ERROR(("%s: customer specific Host GPIO number is (%d)\n",
	         __FUNCTION__, dhd_oob_gpio_num));

#if defined CUSTOMER_HW
	host_oob_irq = MSM_GPIO_TO_INT(dhd_oob_gpio_num);
#elif defined CUSTOMER_HW3
	gpio_request(dhd_oob_gpio_num, "oob irq");
	host_oob_irq = gpio_to_irq(dhd_oob_gpio_num);
	gpio_direction_input(dhd_oob_gpio_num);
#endif 
#endif 

	return (host_oob_irq);
}
#endif 

void
dhd_customer_gpio_wlan_ctrl(int onoff)
{
	switch (onoff) {
		case WLAN_RESET_OFF:
			WL_TRACE(("%s: call customer specific GPIO to insert WLAN RESET\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_off(2);
#endif 
#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
			wifi_set_power(0, 0);
#endif
			WL_ERROR(("=========== WLAN placed in RESET ========\n"));
		break;

		case WLAN_RESET_ON:
			WL_TRACE(("%s: callc customer specific GPIO to remove WLAN RESET\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(2);
#endif 
#if defined(CUSTOMER_HW2) || defined(CUSTOMER_HW4)
			wifi_set_power(1, 0);
#endif
			WL_ERROR(("=========== WLAN going back to live  ========\n"));
		break;

		case WLAN_POWER_OFF:
			WL_TRACE(("%s: call customer specific GPIO to turn off WL_REG_ON\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_off(1);
#endif 
		break;

		case WLAN_POWER_ON:
			WL_TRACE(("%s: call customer specific GPIO to turn on WL_REG_ON\n",
				__FUNCTION__));
#ifdef CUSTOMER_HW
			bcm_wlan_power_on(1);
			
			OSL_DELAY(200);
#endif 
		break;
	}
}

#ifdef GET_CUSTOM_MAC_ENABLE
int
dhd_custom_get_mac_address(unsigned char *buf)
{
	int ret = 0;

	WL_TRACE(("%s Enter\n", __FUNCTION__));
	if (!buf)
		return -EINVAL;

	
#if defined(CUSTOMER_HW2) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 35))
	ret = wifi_get_mac_addr(buf);
#endif

#ifdef EXAMPLE_GET_MAC
	
	{
		struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};
		bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
	}
#endif 

	return ret;
}
#endif 

const struct cntry_locales_custom translate_custom_table[] = {
#ifdef EXAMPLE_TABLE
	{"",   "XY", 4},  
	{"US", "US", 69}, 
	{"CA", "US", 69}, 
	{"EU", "EU", 5},  
	{"AT", "EU", 5},
	{"BE", "EU", 5},
	{"BG", "EU", 5},
	{"CY", "EU", 5},
	{"CZ", "EU", 5},
	{"DK", "EU", 5},
	{"EE", "EU", 5},
	{"FI", "EU", 5},
	{"FR", "EU", 5},
	{"DE", "EU", 5},
	{"GR", "EU", 5},
	{"HU", "EU", 5},
	{"IE", "EU", 5},
	{"IT", "EU", 5},
	{"LV", "EU", 5},
	{"LI", "EU", 5},
	{"LT", "EU", 5},
	{"LU", "EU", 5},
	{"MT", "EU", 5},
	{"NL", "EU", 5},
	{"PL", "EU", 5},
	{"PT", "EU", 5},
	{"RO", "EU", 5},
	{"SK", "EU", 5},
	{"SI", "EU", 5},
	{"ES", "EU", 5},
	{"SE", "EU", 5},
	{"GB", "EU", 5},
	{"KR", "XY", 3},
	{"AU", "XY", 3},
	{"CN", "XY", 3}, 
	{"TW", "XY", 3},
	{"AR", "XY", 3},
	{"MX", "XY", 3},
	{"IL", "IL", 0},
	{"CH", "CH", 0},
	{"TR", "TR", 0},
	{"NO", "NO", 0},
#endif 
#if defined(CUSTOMER_HW2)
#if defined(BCM4334_CHIP) || defined(BCM4335_CHIP)
	{"",   "XZ", 11},  
#endif
#if defined(BCM4335_CHIP)
	{"AE", "AE", 8},
	{"AR", "AR", 1},
	{"AT", "AT", 3},
	{"AU", "AU", 2},
	{"BE", "BE", 3},
	{"BG", "BG", 3},
	{"BH", "BH", 4},
	{"BN", "BN", 1},
	{"BR", "BR", 2},
	{"CA", "CA", 31},
	{"CH", "CH", 3},
	{"CN", "CN", 7},
	{"CY", "CY", 1},
	{"CZ", "CZ", 3},
	{"DE", "DE", 6},
	{"DK", "DK", 3},
	{"DZ", "DZ", 1},
	{"EE", "EE", 3},
	{"ES", "ES", 3},
	{"FI", "FI", 3},
	{"FR", "FR", 3},
	{"GB", "GB", 5},
	{"GR", "GR", 3},
	{"HK", "HK", 1},
	{"HR", "HR", 3},
	{"HU", "HU", 3},
	{"ID", "ID", 1},
	{"IE", "IE", 3},
	{"IL", "IL", 5},
	{"IN", "IN", 2},
	{"IS", "IS", 3},
	{"IT", "IT", 3},
	{"JO", "JO", 4},
	{"JP", "JP", 36},
	{"KR", "KR", 45},
	{"KZ", "KZ", 1},
	{"KW", "KW", 5},
	{"LB", "LB", 3},
	{"LI", "LI", 1},
	{"LT", "LT", 3},
	{"LU", "LU", 3},
	{"LV", "LV", 3},
	{"MA", "MA", 2},
    {"MM", "MM", 5},
	{"MT", "MT", 1},
	{"MX", "MX", 24},
	{"MY", "MY", 2},
    {"NG", "NG", 2},
	{"NL", "NL", 3},
	{"NO", "NO", 3},
	{"NZ", "NZ", 2},
	{"OM", "OM", 4},
	{"PH", "PH", 3},
	{"PL", "PL", 3},
	{"PR", "PR", 20},
	{"PT", "PT", 3},
	{"PY", "PY", 1},
	{"RO", "RO", 3},
	{"RS", "RS", 2},
    {"RU", "RU", 36},
	{"SE", "SE", 3},
	{"SI", "SI", 3},
	{"SK", "SK", 3},
	{"TH", "TH", 3},
	{"TR", "TR", 7},
	{"TW", "TW", 1},
    {"UA", "UA", 12},
    {"US", "US", 111},
	{"VE", "VE", 2},
	{"VN", "VN", 2},
	{"ZA", "ZA", 3},
	{"ZM", "ZM", 2},
#else 
	{"AE", "AE", 1},
	{"AR", "AR", 1},
	{"AT", "AT", 1},
	{"AU", "AU", 2},
	{"BE", "BE", 1},
	{"BG", "BG", 1},
	{"BN", "BN", 1},
	{"CA", "CA", 2},
	{"CH", "CH", 1},
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
	{"ID", "ID", 1},
	{"JP", "JP", 8},
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
	{"TW", "TW", 1},
#endif  
#ifdef CUSTOMER_HW2
	{"IR", "XZ", 11},	
	{"SD", "XZ", 11},	
	{"SY", "XZ", 11},	
	{"GL", "XZ", 11},	
	{"PS", "XZ", 11},	
	{"TL", "XZ", 11},	
	{"MH", "XZ", 11},	
#endif
#ifdef BCM4334_CHIP
	{"US", "US", 0}
#endif
#ifdef BCM4334_CHIP
	{"RU", "RU", 5},
	{"SG", "SG", 4},
	{"US", "US", 46}
#endif
#ifdef BCM4330_CHIP
	{"RU", "RU", 1},
	{"US", "US", 5}
#endif
#endif 
};


void get_customized_country_code(char *country_iso_code, wl_country_t *cspec)
{
#if 0 && defined(CUSTOMER_HW2) && (LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 39))

	struct cntry_locales_custom *cloc_ptr;

	if (!cspec)
		return;

	cloc_ptr = wifi_get_country_code(country_iso_code);
	if (cloc_ptr) {
		strlcpy(cspec->ccode, cloc_ptr->custom_locale, WLC_CNTRY_BUF_SZ);
		cspec->rev = cloc_ptr->custom_locale_rev;
	}
	return;
#else
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
#ifdef EXAMPLE_TABLE
	
	memcpy(cspec->ccode, translate_custom_table[0].custom_locale, WLC_CNTRY_BUF_SZ);
	cspec->rev = translate_custom_table[0].custom_locale_rev;
#endif 
	return;
#endif 
}
