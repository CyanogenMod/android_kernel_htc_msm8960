#ifndef _NET_EVENT_H
#define _NET_EVENT_H


struct dst_entry;

struct netevent_redirect {
	struct dst_entry *old;
	struct dst_entry *new;
};

enum netevent_notif_type {
	NETEVENT_NEIGH_UPDATE = 1, 
	NETEVENT_REDIRECT,	   
};

extern int register_netevent_notifier(struct notifier_block *nb);
extern int unregister_netevent_notifier(struct notifier_block *nb);
extern int call_netevent_notifiers(unsigned long val, void *v);

#endif
