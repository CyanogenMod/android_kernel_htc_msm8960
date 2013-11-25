/*
 * net/sched/ematch.c		Extended Match API
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Thomas Graf <tgraf@suug.ch>
 *
 * ==========================================================================
 *
 * An extended match (ematch) is a small classification tool not worth
 * writing a full classifier for. Ematches can be interconnected to form
 * a logic expression and get attached to classifiers to extend their
 * functionatlity.
 *
 * The userspace part transforms the logic expressions into an array
 * consisting of multiple sequences of interconnected ematches separated
 * by markers. Precedence is implemented by a special ematch kind
 * referencing a sequence beyond the marker of the current sequence
 * causing the current position in the sequence to be pushed onto a stack
 * to allow the current position to be overwritten by the position referenced
 * in the special ematch. Matching continues in the new sequence until a
 * marker is reached causing the position to be restored from the stack.
 *
 * Example:
 *          A AND (B1 OR B2) AND C AND D
 *
 *              ------->-PUSH-------
 *    -->--    /         -->--      \   -->--
 *   /     \  /         /     \      \ /     \
 * +-------+-------+-------+-------+-------+--------+
 * | A AND | B AND | C AND | D END | B1 OR | B2 END |
 * +-------+-------+-------+-------+-------+--------+
 *                    \                      /
 *                     --------<-POP---------
 *
 * where B is a virtual ematch referencing to sequence starting with B1.
 *
 * ==========================================================================
 *
 * How to write an ematch in 60 seconds
 * ------------------------------------
 *
 *   1) Provide a matcher function:
 *      static int my_match(struct sk_buff *skb, struct tcf_ematch *m,
 *                          struct tcf_pkt_info *info)
 *      {
 *      	struct mydata *d = (struct mydata *) m->data;
 *
 *      	if (...matching goes here...)
 *      		return 1;
 *      	else
 *      		return 0;
 *      }
 *
 *   2) Fill out a struct tcf_ematch_ops:
 *      static struct tcf_ematch_ops my_ops = {
 *      	.kind = unique id,
 *      	.datalen = sizeof(struct mydata),
 *      	.match = my_match,
 *      	.owner = THIS_MODULE,
 *      };
 *
 *   3) Register/Unregister your ematch:
 *      static int __init init_my_ematch(void)
 *      {
 *      	return tcf_em_register(&my_ops);
 *      }
 *
 *      static void __exit exit_my_ematch(void)
 *      {
 *      	tcf_em_unregister(&my_ops);
 *      }
 *
 *      module_init(init_my_ematch);
 *      module_exit(exit_my_ematch);
 *
 *   4) By now you should have two more seconds left, barely enough to
 *      open up a beer to watch the compilation going.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/rtnetlink.h>
#include <linux/skbuff.h>
#include <net/pkt_cls.h>

static LIST_HEAD(ematch_ops);
static DEFINE_RWLOCK(ematch_mod_lock);

static struct tcf_ematch_ops *tcf_em_lookup(u16 kind)
{
	struct tcf_ematch_ops *e = NULL;

	read_lock(&ematch_mod_lock);
	list_for_each_entry(e, &ematch_ops, link) {
		if (kind == e->kind) {
			if (!try_module_get(e->owner))
				e = NULL;
			read_unlock(&ematch_mod_lock);
			return e;
		}
	}
	read_unlock(&ematch_mod_lock);

	return NULL;
}

int tcf_em_register(struct tcf_ematch_ops *ops)
{
	int err = -EEXIST;
	struct tcf_ematch_ops *e;

	if (ops->match == NULL)
		return -EINVAL;

	write_lock(&ematch_mod_lock);
	list_for_each_entry(e, &ematch_ops, link)
		if (ops->kind == e->kind)
			goto errout;

	list_add_tail(&ops->link, &ematch_ops);
	err = 0;
errout:
	write_unlock(&ematch_mod_lock);
	return err;
}
EXPORT_SYMBOL(tcf_em_register);

void tcf_em_unregister(struct tcf_ematch_ops *ops)
{
	write_lock(&ematch_mod_lock);
	list_del(&ops->link);
	write_unlock(&ematch_mod_lock);
}
EXPORT_SYMBOL(tcf_em_unregister);

static inline struct tcf_ematch *tcf_em_get_match(struct tcf_ematch_tree *tree,
						  int index)
{
	return &tree->matches[index];
}


static int tcf_em_validate(struct tcf_proto *tp,
			   struct tcf_ematch_tree_hdr *tree_hdr,
			   struct tcf_ematch *em, struct nlattr *nla, int idx)
{
	int err = -EINVAL;
	struct tcf_ematch_hdr *em_hdr = nla_data(nla);
	int data_len = nla_len(nla) - sizeof(*em_hdr);
	void *data = (void *) em_hdr + sizeof(*em_hdr);

	if (!TCF_EM_REL_VALID(em_hdr->flags))
		goto errout;

	if (em_hdr->kind == TCF_EM_CONTAINER) {
		u32 ref;

		if (data_len < sizeof(ref))
			goto errout;
		ref = *(u32 *) data;

		if (ref >= tree_hdr->nmatches)
			goto errout;

		if (ref <= idx)
			goto errout;


		em->data = ref;
	} else {
		em->ops = tcf_em_lookup(em_hdr->kind);

		if (em->ops == NULL) {
			err = -ENOENT;
#ifdef CONFIG_MODULES
			__rtnl_unlock();
			request_module("ematch-kind-%u", em_hdr->kind);
			rtnl_lock();
			em->ops = tcf_em_lookup(em_hdr->kind);
			if (em->ops) {
				module_put(em->ops->owner);
				err = -EAGAIN;
			}
#endif
			goto errout;
		}

		if (em->ops->datalen && data_len < em->ops->datalen)
			goto errout;

		if (em->ops->change) {
			err = em->ops->change(tp, data, data_len, em);
			if (err < 0)
				goto errout;
		} else if (data_len > 0) {
			if (em_hdr->flags & TCF_EM_SIMPLE) {
				if (data_len < sizeof(u32))
					goto errout;
				em->data = *(u32 *) data;
			} else {
				void *v = kmemdup(data, data_len, GFP_KERNEL);
				if (v == NULL) {
					err = -ENOBUFS;
					goto errout;
				}
				em->data = (unsigned long) v;
			}
		}
	}

	em->matchid = em_hdr->matchid;
	em->flags = em_hdr->flags;
	em->datalen = data_len;

	err = 0;
errout:
	return err;
}

static const struct nla_policy em_policy[TCA_EMATCH_TREE_MAX + 1] = {
	[TCA_EMATCH_TREE_HDR]	= { .len = sizeof(struct tcf_ematch_tree_hdr) },
	[TCA_EMATCH_TREE_LIST]	= { .type = NLA_NESTED },
};

int tcf_em_tree_validate(struct tcf_proto *tp, struct nlattr *nla,
			 struct tcf_ematch_tree *tree)
{
	int idx, list_len, matches_len, err;
	struct nlattr *tb[TCA_EMATCH_TREE_MAX + 1];
	struct nlattr *rt_match, *rt_hdr, *rt_list;
	struct tcf_ematch_tree_hdr *tree_hdr;
	struct tcf_ematch *em;

	memset(tree, 0, sizeof(*tree));
	if (!nla)
		return 0;

	err = nla_parse_nested(tb, TCA_EMATCH_TREE_MAX, nla, em_policy);
	if (err < 0)
		goto errout;

	err = -EINVAL;
	rt_hdr = tb[TCA_EMATCH_TREE_HDR];
	rt_list = tb[TCA_EMATCH_TREE_LIST];

	if (rt_hdr == NULL || rt_list == NULL)
		goto errout;

	tree_hdr = nla_data(rt_hdr);
	memcpy(&tree->hdr, tree_hdr, sizeof(*tree_hdr));

	rt_match = nla_data(rt_list);
	list_len = nla_len(rt_list);
	matches_len = tree_hdr->nmatches * sizeof(*em);

	tree->matches = kzalloc(matches_len, GFP_KERNEL);
	if (tree->matches == NULL)
		goto errout;

	for (idx = 0; nla_ok(rt_match, list_len); idx++) {
		err = -EINVAL;

		if (rt_match->nla_type != (idx + 1))
			goto errout_abort;

		if (idx >= tree_hdr->nmatches)
			goto errout_abort;

		if (nla_len(rt_match) < sizeof(struct tcf_ematch_hdr))
			goto errout_abort;

		em = tcf_em_get_match(tree, idx);

		err = tcf_em_validate(tp, tree_hdr, em, rt_match, idx);
		if (err < 0)
			goto errout_abort;

		rt_match = nla_next(rt_match, &list_len);
	}

	if (idx != tree_hdr->nmatches) {
		err = -EINVAL;
		goto errout_abort;
	}

	err = 0;
errout:
	return err;

errout_abort:
	tcf_em_tree_destroy(tp, tree);
	return err;
}
EXPORT_SYMBOL(tcf_em_tree_validate);

void tcf_em_tree_destroy(struct tcf_proto *tp, struct tcf_ematch_tree *tree)
{
	int i;

	if (tree->matches == NULL)
		return;

	for (i = 0; i < tree->hdr.nmatches; i++) {
		struct tcf_ematch *em = tcf_em_get_match(tree, i);

		if (em->ops) {
			if (em->ops->destroy)
				em->ops->destroy(tp, em);
			else if (!tcf_em_is_simple(em))
				kfree((void *) em->data);
			module_put(em->ops->owner);
		}
	}

	tree->hdr.nmatches = 0;
	kfree(tree->matches);
	tree->matches = NULL;
}
EXPORT_SYMBOL(tcf_em_tree_destroy);

int tcf_em_tree_dump(struct sk_buff *skb, struct tcf_ematch_tree *tree, int tlv)
{
	int i;
	u8 *tail;
	struct nlattr *top_start;
	struct nlattr *list_start;

	top_start = nla_nest_start(skb, tlv);
	if (top_start == NULL)
		goto nla_put_failure;

	NLA_PUT(skb, TCA_EMATCH_TREE_HDR, sizeof(tree->hdr), &tree->hdr);

	list_start = nla_nest_start(skb, TCA_EMATCH_TREE_LIST);
	if (list_start == NULL)
		goto nla_put_failure;

	tail = skb_tail_pointer(skb);
	for (i = 0; i < tree->hdr.nmatches; i++) {
		struct nlattr *match_start = (struct nlattr *)tail;
		struct tcf_ematch *em = tcf_em_get_match(tree, i);
		struct tcf_ematch_hdr em_hdr = {
			.kind = em->ops ? em->ops->kind : TCF_EM_CONTAINER,
			.matchid = em->matchid,
			.flags = em->flags
		};

		NLA_PUT(skb, i + 1, sizeof(em_hdr), &em_hdr);

		if (em->ops && em->ops->dump) {
			if (em->ops->dump(skb, em) < 0)
				goto nla_put_failure;
		} else if (tcf_em_is_container(em) || tcf_em_is_simple(em)) {
			u32 u = em->data;
			nla_put_nohdr(skb, sizeof(u), &u);
		} else if (em->datalen > 0)
			nla_put_nohdr(skb, em->datalen, (void *) em->data);

		tail = skb_tail_pointer(skb);
		match_start->nla_len = tail - (u8 *)match_start;
	}

	nla_nest_end(skb, list_start);
	nla_nest_end(skb, top_start);

	return 0;

nla_put_failure:
	return -1;
}
EXPORT_SYMBOL(tcf_em_tree_dump);

static inline int tcf_em_match(struct sk_buff *skb, struct tcf_ematch *em,
			       struct tcf_pkt_info *info)
{
	int r = em->ops->match(skb, em, info);

	return tcf_em_is_inverted(em) ? !r : r;
}

int __tcf_em_tree_match(struct sk_buff *skb, struct tcf_ematch_tree *tree,
			struct tcf_pkt_info *info)
{
	int stackp = 0, match_idx = 0, res = 0;
	struct tcf_ematch *cur_match;
	int stack[CONFIG_NET_EMATCH_STACK];

proceed:
	while (match_idx < tree->hdr.nmatches) {
		cur_match = tcf_em_get_match(tree, match_idx);

		if (tcf_em_is_container(cur_match)) {
			if (unlikely(stackp >= CONFIG_NET_EMATCH_STACK))
				goto stack_overflow;

			stack[stackp++] = match_idx;
			match_idx = cur_match->data;
			goto proceed;
		}

		res = tcf_em_match(skb, cur_match, info);

		if (tcf_em_early_end(cur_match, res))
			break;

		match_idx++;
	}

pop_stack:
	if (stackp > 0) {
		match_idx = stack[--stackp];
		cur_match = tcf_em_get_match(tree, match_idx);

		if (tcf_em_early_end(cur_match, res))
			goto pop_stack;
		else {
			match_idx++;
			goto proceed;
		}
	}

	return res;

stack_overflow:
	if (net_ratelimit())
		pr_warning("tc ematch: local stack overflow,"
			   " increase NET_EMATCH_STACK\n");
	return -1;
}
EXPORT_SYMBOL(__tcf_em_tree_match);
