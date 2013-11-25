#ifndef __NET_PKT_CLS_H
#define __NET_PKT_CLS_H

#include <linux/pkt_cls.h>
#include <net/sch_generic.h>
#include <net/act_api.h>


struct tcf_walker {
	int	stop;
	int	skip;
	int	count;
	int	(*fn)(struct tcf_proto *, unsigned long node, struct tcf_walker *);
};

extern int register_tcf_proto_ops(struct tcf_proto_ops *ops);
extern int unregister_tcf_proto_ops(struct tcf_proto_ops *ops);

static inline unsigned long
__cls_set_class(unsigned long *clp, unsigned long cl)
{
	unsigned long old_cl;
 
	old_cl = *clp;
	*clp = cl;
	return old_cl;
}

static inline unsigned long
cls_set_class(struct tcf_proto *tp, unsigned long *clp, 
	unsigned long cl)
{
	unsigned long old_cl;
	
	tcf_tree_lock(tp);
	old_cl = __cls_set_class(clp, cl);
	tcf_tree_unlock(tp);
 
	return old_cl;
}

static inline void
tcf_bind_filter(struct tcf_proto *tp, struct tcf_result *r, unsigned long base)
{
	unsigned long cl;

	cl = tp->q->ops->cl_ops->bind_tcf(tp->q, base, r->classid);
	cl = cls_set_class(tp, &r->class, cl);
	if (cl)
		tp->q->ops->cl_ops->unbind_tcf(tp->q, cl);
}

static inline void
tcf_unbind_filter(struct tcf_proto *tp, struct tcf_result *r)
{
	unsigned long cl;

	if ((cl = __cls_set_class(&r->class, 0)) != 0)
		tp->q->ops->cl_ops->unbind_tcf(tp->q, cl);
}

struct tcf_exts {
#ifdef CONFIG_NET_CLS_ACT
	struct tc_action *action;
#endif
};

struct tcf_ext_map {
	int action;
	int police;
};

static inline int
tcf_exts_is_predicative(struct tcf_exts *exts)
{
#ifdef CONFIG_NET_CLS_ACT
	return !!exts->action;
#else
	return 0;
#endif
}

static inline int
tcf_exts_is_available(struct tcf_exts *exts)
{
	
	return tcf_exts_is_predicative(exts);
}

static inline int
tcf_exts_exec(struct sk_buff *skb, struct tcf_exts *exts,
	       struct tcf_result *res)
{
#ifdef CONFIG_NET_CLS_ACT
	if (exts->action)
		return tcf_action_exec(skb, exts->action, res);
#endif
	return 0;
}

extern int tcf_exts_validate(struct tcf_proto *tp, struct nlattr **tb,
	                     struct nlattr *rate_tlv, struct tcf_exts *exts,
	                     const struct tcf_ext_map *map);
extern void tcf_exts_destroy(struct tcf_proto *tp, struct tcf_exts *exts);
extern void tcf_exts_change(struct tcf_proto *tp, struct tcf_exts *dst,
	                     struct tcf_exts *src);
extern int tcf_exts_dump(struct sk_buff *skb, struct tcf_exts *exts,
	                 const struct tcf_ext_map *map);
extern int tcf_exts_dump_stats(struct sk_buff *skb, struct tcf_exts *exts,
	                       const struct tcf_ext_map *map);

struct tcf_pkt_info {
	unsigned char *		ptr;
	int			nexthdr;
};

#ifdef CONFIG_NET_EMATCH

struct tcf_ematch_ops;

struct tcf_ematch {
	struct tcf_ematch_ops * ops;
	unsigned long		data;
	unsigned int		datalen;
	u16			matchid;
	u16			flags;
};

static inline int tcf_em_is_container(struct tcf_ematch *em)
{
	return !em->ops;
}

static inline int tcf_em_is_simple(struct tcf_ematch *em)
{
	return em->flags & TCF_EM_SIMPLE;
}

static inline int tcf_em_is_inverted(struct tcf_ematch *em)
{
	return em->flags & TCF_EM_INVERT;
}

static inline int tcf_em_last_match(struct tcf_ematch *em)
{
	return (em->flags & TCF_EM_REL_MASK) == TCF_EM_REL_END;
}

static inline int tcf_em_early_end(struct tcf_ematch *em, int result)
{
	if (tcf_em_last_match(em))
		return 1;

	if (result == 0 && em->flags & TCF_EM_REL_AND)
		return 1;

	if (result != 0 && em->flags & TCF_EM_REL_OR)
		return 1;

	return 0;
}
	
struct tcf_ematch_tree {
	struct tcf_ematch_tree_hdr hdr;
	struct tcf_ematch *	matches;
	
};

struct tcf_ematch_ops {
	int			kind;
	int			datalen;
	int			(*change)(struct tcf_proto *, void *,
					  int, struct tcf_ematch *);
	int			(*match)(struct sk_buff *, struct tcf_ematch *,
					 struct tcf_pkt_info *);
	void			(*destroy)(struct tcf_proto *,
					   struct tcf_ematch *);
	int			(*dump)(struct sk_buff *, struct tcf_ematch *);
	struct module		*owner;
	struct list_head	link;
};

extern int tcf_em_register(struct tcf_ematch_ops *);
extern void tcf_em_unregister(struct tcf_ematch_ops *);
extern int tcf_em_tree_validate(struct tcf_proto *, struct nlattr *,
				struct tcf_ematch_tree *);
extern void tcf_em_tree_destroy(struct tcf_proto *, struct tcf_ematch_tree *);
extern int tcf_em_tree_dump(struct sk_buff *, struct tcf_ematch_tree *, int);
extern int __tcf_em_tree_match(struct sk_buff *, struct tcf_ematch_tree *,
			       struct tcf_pkt_info *);

static inline void tcf_em_tree_change(struct tcf_proto *tp,
				      struct tcf_ematch_tree *dst,
				      struct tcf_ematch_tree *src)
{
	tcf_tree_lock(tp);
	memcpy(dst, src, sizeof(*dst));
	tcf_tree_unlock(tp);
}

static inline int tcf_em_tree_match(struct sk_buff *skb,
				    struct tcf_ematch_tree *tree,
				    struct tcf_pkt_info *info)
{
	if (tree->hdr.nmatches)
		return __tcf_em_tree_match(skb, tree, info);
	else
		return 1;
}

#define MODULE_ALIAS_TCF_EMATCH(kind)	MODULE_ALIAS("ematch-kind-" __stringify(kind))

#else 

struct tcf_ematch_tree {
};

#define tcf_em_tree_validate(tp, tb, t) ((void)(t), 0)
#define tcf_em_tree_destroy(tp, t) do { (void)(t); } while(0)
#define tcf_em_tree_dump(skb, t, tlv) (0)
#define tcf_em_tree_change(tp, dst, src) do { } while(0)
#define tcf_em_tree_match(skb, t, info) ((void)(info), 1)

#endif 

static inline unsigned char * tcf_get_base_ptr(struct sk_buff *skb, int layer)
{
	switch (layer) {
		case TCF_LAYER_LINK:
			return skb->data;
		case TCF_LAYER_NETWORK:
			return skb_network_header(skb);
		case TCF_LAYER_TRANSPORT:
			return skb_transport_header(skb);
	}

	return NULL;
}

static inline int tcf_valid_offset(const struct sk_buff *skb,
				   const unsigned char *ptr, const int len)
{
	return likely((ptr + len) <= skb_tail_pointer(skb) &&
		      ptr >= skb->head &&
		      (ptr <= (ptr + len)));
}

#ifdef CONFIG_NET_CLS_IND
#include <net/net_namespace.h>

static inline int
tcf_change_indev(struct tcf_proto *tp, char *indev, struct nlattr *indev_tlv)
{
	if (nla_strlcpy(indev, indev_tlv, IFNAMSIZ) >= IFNAMSIZ)
		return -EINVAL;
	return 0;
}

static inline int
tcf_match_indev(struct sk_buff *skb, char *indev)
{
	struct net_device *dev;

	if (indev[0]) {
		if  (!skb->skb_iif)
			return 0;
		dev = __dev_get_by_index(dev_net(skb->dev), skb->skb_iif);
		if (!dev || strcmp(indev, dev->name))
			return 0;
	}

	return 1;
}
#endif 

#endif
