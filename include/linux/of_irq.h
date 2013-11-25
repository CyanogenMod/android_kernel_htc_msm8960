#ifndef __OF_IRQ_H
#define __OF_IRQ_H

#if defined(CONFIG_OF)
struct of_irq;
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/ioport.h>
#include <linux/of.h>

extern unsigned int irq_of_parse_and_map(struct device_node *node, int index);

#if defined(CONFIG_OF_IRQ)
#define OF_MAX_IRQ_SPEC		4 
struct of_irq {
	struct device_node *controller; 
	u32 size; 
	u32 specifier[OF_MAX_IRQ_SPEC]; 
};

typedef int (*of_irq_init_cb_t)(struct device_node *, struct device_node *);

#define OF_IMAP_OLDWORLD_MAC	0x00000001
#define OF_IMAP_NO_PHANDLE	0x00000002

#if defined(CONFIG_PPC32) && defined(CONFIG_PPC_PMAC)
extern unsigned int of_irq_workarounds;
extern struct device_node *of_irq_dflt_pic;
extern int of_irq_map_oldworld(struct device_node *device, int index,
			       struct of_irq *out_irq);
#else 
#define of_irq_workarounds (0)
#define of_irq_dflt_pic (NULL)
static inline int of_irq_map_oldworld(struct device_node *device, int index,
				      struct of_irq *out_irq)
{
	return -EINVAL;
}
#endif 


extern int of_irq_map_raw(struct device_node *parent, const u32 *intspec,
			  u32 ointsize, const u32 *addr,
			  struct of_irq *out_irq);
extern int of_irq_map_one(struct device_node *device, int index,
			  struct of_irq *out_irq);
extern unsigned int irq_create_of_mapping(struct device_node *controller,
					  const u32 *intspec,
					  unsigned int intsize);
extern int of_irq_to_resource(struct device_node *dev, int index,
			      struct resource *r);
extern int of_irq_count(struct device_node *dev);
extern int of_irq_to_resource_table(struct device_node *dev,
		struct resource *res, int nr_irqs);
extern struct device_node *of_irq_find_parent(struct device_node *child);

extern void of_irq_init(const struct of_device_id *matches);

#endif 
#endif 
#endif 
