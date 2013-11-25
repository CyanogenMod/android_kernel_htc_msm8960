
#ifndef _LINUX_IRQDOMAIN_H
#define _LINUX_IRQDOMAIN_H

#include <linux/types.h>
#include <linux/radix-tree.h>

struct device_node;
struct irq_domain;
struct of_device_id;

#define NUM_ISA_INTERRUPTS	16

struct irq_domain_ops {
	int (*match)(struct irq_domain *d, struct device_node *node);
	int (*map)(struct irq_domain *d, unsigned int virq, irq_hw_number_t hw);
	void (*unmap)(struct irq_domain *d, unsigned int virq);
	int (*xlate)(struct irq_domain *d, struct device_node *node,
		     const u32 *intspec, unsigned int intsize,
		     unsigned long *out_hwirq, unsigned int *out_type);
};

struct irq_domain {
	struct list_head link;

	
	unsigned int revmap_type;
	union {
		struct {
			unsigned int size;
			unsigned int first_irq;
			irq_hw_number_t first_hwirq;
		} legacy;
		struct {
			unsigned int size;
			unsigned int *revmap;
		} linear;
		struct {
			unsigned int max_irq;
		} nomap;
		struct radix_tree_root tree;
	} revmap_data;
	const struct irq_domain_ops *ops;
	void *host_data;
	irq_hw_number_t inval_irq;

	
	struct device_node *of_node;
};

#ifdef CONFIG_IRQ_DOMAIN
struct irq_domain *irq_domain_add_legacy(struct device_node *of_node,
					 unsigned int size,
					 unsigned int first_irq,
					 irq_hw_number_t first_hwirq,
					 const struct irq_domain_ops *ops,
					 void *host_data);
struct irq_domain *irq_domain_add_linear(struct device_node *of_node,
					 unsigned int size,
					 const struct irq_domain_ops *ops,
					 void *host_data);
struct irq_domain *irq_domain_add_nomap(struct device_node *of_node,
					 unsigned int max_irq,
					 const struct irq_domain_ops *ops,
					 void *host_data);
struct irq_domain *irq_domain_add_tree(struct device_node *of_node,
					 const struct irq_domain_ops *ops,
					 void *host_data);

extern struct irq_domain *irq_find_host(struct device_node *node);
extern void irq_set_default_host(struct irq_domain *host);

static inline struct irq_domain *irq_domain_add_legacy_isa(
				struct device_node *of_node,
				const struct irq_domain_ops *ops,
				void *host_data)
{
	return irq_domain_add_legacy(of_node, NUM_ISA_INTERRUPTS, 0, 0, ops,
				     host_data);
}
extern struct irq_domain *irq_find_host(struct device_node *node);
extern void irq_set_default_host(struct irq_domain *host);


extern unsigned int irq_create_mapping(struct irq_domain *host,
				       irq_hw_number_t hwirq);
extern void irq_dispose_mapping(unsigned int virq);
extern unsigned int irq_find_mapping(struct irq_domain *host,
				     irq_hw_number_t hwirq);
extern unsigned int irq_create_direct_mapping(struct irq_domain *host);
extern void irq_radix_revmap_insert(struct irq_domain *host, unsigned int virq,
				    irq_hw_number_t hwirq);
extern unsigned int irq_radix_revmap_lookup(struct irq_domain *host,
					    irq_hw_number_t hwirq);
extern unsigned int irq_linear_revmap(struct irq_domain *host,
				      irq_hw_number_t hwirq);

extern const struct irq_domain_ops irq_domain_simple_ops;

int irq_domain_xlate_onecell(struct irq_domain *d, struct device_node *ctrlr,
			const u32 *intspec, unsigned int intsize,
			irq_hw_number_t *out_hwirq, unsigned int *out_type);
int irq_domain_xlate_twocell(struct irq_domain *d, struct device_node *ctrlr,
			const u32 *intspec, unsigned int intsize,
			irq_hw_number_t *out_hwirq, unsigned int *out_type);
int irq_domain_xlate_onetwocell(struct irq_domain *d, struct device_node *ctrlr,
			const u32 *intspec, unsigned int intsize,
			irq_hw_number_t *out_hwirq, unsigned int *out_type);

#if defined(CONFIG_OF_IRQ)
extern void irq_domain_generate_simple(const struct of_device_id *match,
					u64 phys_base, unsigned int irq_start);
#else 
static inline void irq_domain_generate_simple(const struct of_device_id *match,
					u64 phys_base, unsigned int irq_start) { }
#endif 

#else 
static inline void irq_dispose_mapping(unsigned int virq) { }
#endif 

#endif 
