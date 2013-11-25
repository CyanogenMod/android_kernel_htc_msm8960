#ifndef __NET_WIRELESS_REG_H
#define __NET_WIRELESS_REG_H
/*
 * Copyright 2008-2011	Luis R. Rodriguez <mcgrof@qca.qualcomm.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

extern const struct ieee80211_regdomain *cfg80211_regdomain;

bool is_world_regdom(const char *alpha2);
bool reg_is_valid_request(const char *alpha2);
bool reg_supported_dfs_region(u8 dfs_region);

int regulatory_hint_user(const char *alpha2);

int reg_device_uevent(struct device *dev, struct kobj_uevent_env *env);
void reg_device_remove(struct wiphy *wiphy);

int __init regulatory_init(void);
void regulatory_exit(void);

int set_regdom(const struct ieee80211_regdomain *rd);

void regulatory_update(struct wiphy *wiphy, enum nl80211_reg_initiator setby);

int regulatory_hint_found_beacon(struct wiphy *wiphy,
					struct ieee80211_channel *beacon_chan,
					gfp_t gfp);

void regulatory_hint_11d(struct wiphy *wiphy,
			 enum ieee80211_band band,
			 u8 *country_ie,
			 u8 country_ie_len);

void regulatory_hint_disconnect(void);

#endif  
