/*
 * SELinux support for the Audit LSM hooks
 *
 * Most of below header was moved from include/linux/selinux.h which
 * is released under below copyrights:
 *
 * Author: James Morris <jmorris@redhat.com>
 *
 * Copyright (C) 2005 Red Hat, Inc., James Morris <jmorris@redhat.com>
 * Copyright (C) 2006 Trusted Computer Solutions, Inc. <dgoeddel@trustedcs.com>
 * Copyright (C) 2006 IBM Corporation, Timothy R. Chavez <tinytim@us.ibm.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2,
 * as published by the Free Software Foundation.
 */

#ifndef _SELINUX_AUDIT_H
#define _SELINUX_AUDIT_H

int selinux_audit_rule_init(u32 field, u32 op, char *rulestr, void **rule);

void selinux_audit_rule_free(void *rule);

int selinux_audit_rule_match(u32 sid, u32 field, u32 op, void *rule,
			     struct audit_context *actx);

int selinux_audit_rule_known(struct audit_krule *krule);

#endif 

