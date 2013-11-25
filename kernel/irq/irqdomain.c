#include <linux/debugfs.h>
#include <linux/hardirq.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/irqdesc.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/fs.h>

#define IRQ_DOMAIN_MAP_LEGACY 0 
#define IRQ_DOMAIN_MAP_NOMAP 1 
#define IRQ_DOMAIN_MAP_LINEAR 2 
#define IRQ_DOMAIN_MAP_TREE 3 

static LIST_HEAD(irq_domain_list);
static DEFINE_MUTEX(irq_domain_mutex);

static DEFINE_MUTEX(revmap_trees_mutex);
static struct irq_domain *irq_default_domain;

static struct irq_domain *irq_domain_alloc(struct device_node *of_node,
					   unsigned int revmap_type,
					   const struct irq_domain_ops *ops,
					   void *host_data)
{
	struct irq_domain *domain;

	domain = kzalloc(sizeof(*domain), GFP_KERNEL);
	if (WARN_ON(!domain))
		return NULL;

	
	domain->revmap_type = revmap_type;
	domain->ops = ops;
	domain->host_data = host_data;
	domain->of_node = of_node_get(of_node);

	return domain;
}

static void irq_domain_add(struct irq_domain *domain)
{
	mutex_lock(&irq_domain_mutex);
	list_add(&domain->link, &irq_domain_list);
	mutex_unlock(&irq_domain_mutex);
	pr_debug("irq: Allocated domain of type %d @0x%p\n",
		 domain->revmap_type, domain);
}

static unsigned int irq_domain_legacy_revmap(struct irq_domain *domain,
					     irq_hw_number_t hwirq)
{
	irq_hw_number_t first_hwirq = domain->revmap_data.legacy.first_hwirq;
	int size = domain->revmap_data.legacy.size;

	if (WARN_ON(hwirq < first_hwirq || hwirq >= first_hwirq + size))
		return 0;
	return hwirq - first_hwirq + domain->revmap_data.legacy.first_irq;
}

struct irq_domain *irq_domain_add_legacy(struct device_node *of_node,
					 unsigned int size,
					 unsigned int first_irq,
					 irq_hw_number_t first_hwirq,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	struct irq_domain *domain;
	unsigned int i;

	domain = irq_domain_alloc(of_node, IRQ_DOMAIN_MAP_LEGACY, ops, host_data);
	if (!domain)
		return NULL;

	domain->revmap_data.legacy.first_irq = first_irq;
	domain->revmap_data.legacy.first_hwirq = first_hwirq;
	domain->revmap_data.legacy.size = size;

	mutex_lock(&irq_domain_mutex);
	
	for (i = 0; i < size; i++) {
		int irq = first_irq + i;
		struct irq_data *irq_data = irq_get_irq_data(irq);

		if (WARN_ON(!irq_data || irq_data->domain)) {
			mutex_unlock(&irq_domain_mutex);
			of_node_put(domain->of_node);
			kfree(domain);
			return NULL;
		}
	}

	
	for (i = 0; i < size; i++) {
		struct irq_data *irq_data = irq_get_irq_data(first_irq + i);
		irq_data->hwirq = first_hwirq + i;
		irq_data->domain = domain;
	}
	mutex_unlock(&irq_domain_mutex);

	for (i = 0; i < size; i++) {
		int irq = first_irq + i;
		int hwirq = first_hwirq + i;

		
		if (!irq)
			continue;

		ops->map(domain, irq, hwirq);

		
		irq_clear_status_flags(irq, IRQ_NOREQUEST);
	}

	irq_domain_add(domain);
	return domain;
}

struct irq_domain *irq_domain_add_linear(struct device_node *of_node,
					 unsigned int size,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	struct irq_domain *domain;
	unsigned int *revmap;

	revmap = kzalloc(sizeof(*revmap) * size, GFP_KERNEL);
	if (WARN_ON(!revmap))
		return NULL;

	domain = irq_domain_alloc(of_node, IRQ_DOMAIN_MAP_LINEAR, ops, host_data);
	if (!domain) {
		kfree(revmap);
		return NULL;
	}
	domain->revmap_data.linear.size = size;
	domain->revmap_data.linear.revmap = revmap;
	irq_domain_add(domain);
	return domain;
}

struct irq_domain *irq_domain_add_nomap(struct device_node *of_node,
					 unsigned int max_irq,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	struct irq_domain *domain = irq_domain_alloc(of_node,
					IRQ_DOMAIN_MAP_NOMAP, ops, host_data);
	if (domain) {
		domain->revmap_data.nomap.max_irq = max_irq ? max_irq : ~0;
		irq_domain_add(domain);
	}
	return domain;
}

struct irq_domain *irq_domain_add_tree(struct device_node *of_node,
					 const struct irq_domain_ops *ops,
					 void *host_data)
{
	struct irq_domain *domain = irq_domain_alloc(of_node,
					IRQ_DOMAIN_MAP_TREE, ops, host_data);
	if (domain) {
		INIT_RADIX_TREE(&domain->revmap_data.tree, GFP_KERNEL);
		irq_domain_add(domain);
	}
	return domain;
}

struct irq_domain *irq_find_host(struct device_node *node)
{
	struct irq_domain *h, *found = NULL;
	int rc;

	mutex_lock(&irq_domain_mutex);
	list_for_each_entry(h, &irq_domain_list, link) {
		if (h->ops->match)
			rc = h->ops->match(h, node);
		else
			rc = (h->of_node != NULL) && (h->of_node == node);

		if (rc) {
			found = h;
			break;
		}
	}
	mutex_unlock(&irq_domain_mutex);
	return found;
}
EXPORT_SYMBOL_GPL(irq_find_host);

void irq_set_default_host(struct irq_domain *domain)
{
	pr_debug("irq: Default domain set to @0x%p\n", domain);

	irq_default_domain = domain;
}

static int irq_setup_virq(struct irq_domain *domain, unsigned int virq,
			    irq_hw_number_t hwirq)
{
	struct irq_data *irq_data = irq_get_irq_data(virq);

	irq_data->hwirq = hwirq;
	irq_data->domain = domain;
	if (domain->ops->map(domain, virq, hwirq)) {
		pr_debug("irq: -> mapping failed, freeing\n");
		irq_data->domain = NULL;
		irq_data->hwirq = 0;
		return -1;
	}

	irq_clear_status_flags(virq, IRQ_NOREQUEST);

	return 0;
}

unsigned int irq_create_direct_mapping(struct irq_domain *domain)
{
	unsigned int virq;

	if (domain == NULL)
		domain = irq_default_domain;

	BUG_ON(domain == NULL);
	WARN_ON(domain->revmap_type != IRQ_DOMAIN_MAP_NOMAP);

	virq = irq_alloc_desc_from(1, 0);
	if (!virq) {
		pr_debug("irq: create_direct virq allocation failed\n");
		return 0;
	}
	if (virq >= domain->revmap_data.nomap.max_irq) {
		pr_err("ERROR: no free irqs available below %i maximum\n",
			domain->revmap_data.nomap.max_irq);
		irq_free_desc(virq);
		return 0;
	}
	pr_debug("irq: create_direct obtained virq %d\n", virq);

	if (irq_setup_virq(domain, virq, virq)) {
		irq_free_desc(virq);
		return 0;
	}

	return virq;
}

unsigned int irq_create_mapping(struct irq_domain *domain,
				irq_hw_number_t hwirq)
{
	unsigned int hint;
	int virq;

	pr_debug("irq: irq_create_mapping(0x%p, 0x%lx)\n", domain, hwirq);

	
	if (domain == NULL)
		domain = irq_default_domain;
	if (domain == NULL) {
		printk(KERN_WARNING "irq_create_mapping called for"
		       " NULL domain, hwirq=%lx\n", hwirq);
		WARN_ON(1);
		return 0;
	}
	pr_debug("irq: -> using domain @%p\n", domain);

	
	virq = irq_find_mapping(domain, hwirq);
	if (virq) {
		pr_debug("irq: -> existing mapping on virq %d\n", virq);
		return virq;
	}

	
	if (domain->revmap_type == IRQ_DOMAIN_MAP_LEGACY)
		return irq_domain_legacy_revmap(domain, hwirq);

	
	hint = hwirq % nr_irqs;
	if (hint == 0)
		hint++;
	virq = irq_alloc_desc_from(hint, 0);
	if (virq <= 0)
		virq = irq_alloc_desc_from(1, 0);
	if (virq <= 0) {
		pr_debug("irq: -> virq allocation failed\n");
		return 0;
	}

	if (irq_setup_virq(domain, virq, hwirq)) {
		if (domain->revmap_type != IRQ_DOMAIN_MAP_LEGACY)
			irq_free_desc(virq);
		return 0;
	}

	pr_debug("irq: irq %lu on domain %s mapped to virtual irq %u\n",
		hwirq, domain->of_node ? domain->of_node->full_name : "null", virq);

	return virq;
}
EXPORT_SYMBOL_GPL(irq_create_mapping);

unsigned int irq_create_of_mapping(struct device_node *controller,
				   const u32 *intspec, unsigned int intsize)
{
	struct irq_domain *domain;
	irq_hw_number_t hwirq;
	unsigned int type = IRQ_TYPE_NONE;
	unsigned int virq;

	domain = controller ? irq_find_host(controller) : irq_default_domain;
	if (!domain) {
#ifdef CONFIG_MIPS
		if (intsize > 0)
			return intspec[0];
#endif
		printk(KERN_WARNING "irq: no irq domain found for %s !\n",
		       controller->full_name);
		return 0;
	}

	
	if (domain->ops->xlate == NULL)
		hwirq = intspec[0];
	else {
		if (domain->ops->xlate(domain, controller, intspec, intsize,
				     &hwirq, &type))
			return 0;
	}

	
	virq = irq_create_mapping(domain, hwirq);
	if (!virq)
		return virq;

	
	if (type != IRQ_TYPE_NONE &&
	    type != (irqd_get_trigger_type(irq_get_irq_data(virq))))
		irq_set_irq_type(virq, type);
	return virq;
}
EXPORT_SYMBOL_GPL(irq_create_of_mapping);

void irq_dispose_mapping(unsigned int virq)
{
	struct irq_data *irq_data = irq_get_irq_data(virq);
	struct irq_domain *domain;
	irq_hw_number_t hwirq;

	if (!virq || !irq_data)
		return;

	domain = irq_data->domain;
	if (WARN_ON(domain == NULL))
		return;

	
	if (domain->revmap_type == IRQ_DOMAIN_MAP_LEGACY)
		return;

	irq_set_status_flags(virq, IRQ_NOREQUEST);

	
	irq_set_chip_and_handler(virq, NULL, NULL);

	
	synchronize_irq(virq);

	
	if (domain->ops->unmap)
		domain->ops->unmap(domain, virq);
	smp_mb();

	
	hwirq = irq_data->hwirq;
	switch(domain->revmap_type) {
	case IRQ_DOMAIN_MAP_LINEAR:
		if (hwirq < domain->revmap_data.linear.size)
			domain->revmap_data.linear.revmap[hwirq] = 0;
		break;
	case IRQ_DOMAIN_MAP_TREE:
		mutex_lock(&revmap_trees_mutex);
		radix_tree_delete(&domain->revmap_data.tree, hwirq);
		mutex_unlock(&revmap_trees_mutex);
		break;
	}

	irq_free_desc(virq);
}
EXPORT_SYMBOL_GPL(irq_dispose_mapping);

unsigned int irq_find_mapping(struct irq_domain *domain,
			      irq_hw_number_t hwirq)
{
	unsigned int i;
	unsigned int hint = hwirq % nr_irqs;

	
	if (domain == NULL)
		domain = irq_default_domain;
	if (domain == NULL)
		return 0;

	
	if (domain->revmap_type == IRQ_DOMAIN_MAP_LEGACY)
		return irq_domain_legacy_revmap(domain, hwirq);

	
	if (hint == 0)
		hint = 1;
	i = hint;
	do {
		struct irq_data *data = irq_get_irq_data(i);
		if (data && (data->domain == domain) && (data->hwirq == hwirq))
			return i;
		i++;
		if (i >= nr_irqs)
			i = 1;
	} while(i != hint);
	return 0;
}
EXPORT_SYMBOL_GPL(irq_find_mapping);

unsigned int irq_radix_revmap_lookup(struct irq_domain *domain,
				     irq_hw_number_t hwirq)
{
	struct irq_data *irq_data;

	if (WARN_ON_ONCE(domain->revmap_type != IRQ_DOMAIN_MAP_TREE))
		return irq_find_mapping(domain, hwirq);

	rcu_read_lock();
	irq_data = radix_tree_lookup(&domain->revmap_data.tree, hwirq);
	rcu_read_unlock();

	return irq_data ? irq_data->irq : irq_find_mapping(domain, hwirq);
}

void irq_radix_revmap_insert(struct irq_domain *domain, unsigned int virq,
			     irq_hw_number_t hwirq)
{
	struct irq_data *irq_data = irq_get_irq_data(virq);

	if (WARN_ON(domain->revmap_type != IRQ_DOMAIN_MAP_TREE))
		return;

	if (virq) {
		mutex_lock(&revmap_trees_mutex);
		radix_tree_insert(&domain->revmap_data.tree, hwirq, irq_data);
		mutex_unlock(&revmap_trees_mutex);
	}
}

unsigned int irq_linear_revmap(struct irq_domain *domain,
			       irq_hw_number_t hwirq)
{
	unsigned int *revmap;

	if (WARN_ON_ONCE(domain->revmap_type != IRQ_DOMAIN_MAP_LINEAR))
		return irq_find_mapping(domain, hwirq);

	
	if (unlikely(hwirq >= domain->revmap_data.linear.size))
		return irq_find_mapping(domain, hwirq);

	
	revmap = domain->revmap_data.linear.revmap;
	if (unlikely(revmap == NULL))
		return irq_find_mapping(domain, hwirq);

	
	if (unlikely(!revmap[hwirq]))
		revmap[hwirq] = irq_find_mapping(domain, hwirq);

	return revmap[hwirq];
}

#ifdef CONFIG_IRQ_DOMAIN_DEBUG
static int virq_debug_show(struct seq_file *m, void *private)
{
	unsigned long flags;
	struct irq_desc *desc;
	const char *p;
	static const char none[] = "none";
	void *data;
	int i;

	seq_printf(m, "%-5s  %-7s  %-15s  %-*s  %s\n", "irq", "hwirq",
		      "chip name", (int)(2 * sizeof(void *) + 2), "chip data",
		      "domain name");

	for (i = 1; i < nr_irqs; i++) {
		desc = irq_to_desc(i);
		if (!desc)
			continue;

		raw_spin_lock_irqsave(&desc->lock, flags);

		if (desc->action && desc->action->handler) {
			struct irq_chip *chip;

			seq_printf(m, "%5d  ", i);
			seq_printf(m, "0x%05lx  ", desc->irq_data.hwirq);

			chip = irq_desc_get_chip(desc);
			if (chip && chip->name)
				p = chip->name;
			else
				p = none;
			seq_printf(m, "%-15s  ", p);

			data = irq_desc_get_chip_data(desc);
			seq_printf(m, data ? "0x%p  " : "  %p  ", data);

			if (desc->irq_data.domain && desc->irq_data.domain->of_node)
				p = desc->irq_data.domain->of_node->full_name;
			else
				p = none;
			seq_printf(m, "%s\n", p);
		}

		raw_spin_unlock_irqrestore(&desc->lock, flags);
	}

	return 0;
}

static int virq_debug_open(struct inode *inode, struct file *file)
{
	return single_open(file, virq_debug_show, inode->i_private);
}

static const struct file_operations virq_debug_fops = {
	.open = virq_debug_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init irq_debugfs_init(void)
{
	if (debugfs_create_file("irq_domain_mapping", S_IRUGO, NULL,
				 NULL, &virq_debug_fops) == NULL)
		return -ENOMEM;

	return 0;
}
__initcall(irq_debugfs_init);
#endif 

int irq_domain_simple_map(struct irq_domain *d, unsigned int irq,
			  irq_hw_number_t hwirq)
{
	return 0;
}

int irq_domain_xlate_onecell(struct irq_domain *d, struct device_node *ctrlr,
			     const u32 *intspec, unsigned int intsize,
			     unsigned long *out_hwirq, unsigned int *out_type)
{
	if (WARN_ON(intsize < 1))
		return -EINVAL;
	*out_hwirq = intspec[0];
	*out_type = IRQ_TYPE_NONE;
	return 0;
}
EXPORT_SYMBOL_GPL(irq_domain_xlate_onecell);

int irq_domain_xlate_twocell(struct irq_domain *d, struct device_node *ctrlr,
			const u32 *intspec, unsigned int intsize,
			irq_hw_number_t *out_hwirq, unsigned int *out_type)
{
	if (WARN_ON(intsize < 2))
		return -EINVAL;
	*out_hwirq = intspec[0];
	*out_type = intspec[1] & IRQ_TYPE_SENSE_MASK;
	return 0;
}
EXPORT_SYMBOL_GPL(irq_domain_xlate_twocell);

int irq_domain_xlate_onetwocell(struct irq_domain *d,
				struct device_node *ctrlr,
				const u32 *intspec, unsigned int intsize,
				unsigned long *out_hwirq, unsigned int *out_type)
{
	if (WARN_ON(intsize < 1))
		return -EINVAL;
	*out_hwirq = intspec[0];
	*out_type = (intsize > 1) ? intspec[1] : IRQ_TYPE_NONE;
	return 0;
}
EXPORT_SYMBOL_GPL(irq_domain_xlate_onetwocell);

const struct irq_domain_ops irq_domain_simple_ops = {
	.map = irq_domain_simple_map,
	.xlate = irq_domain_xlate_onetwocell,
};
EXPORT_SYMBOL_GPL(irq_domain_simple_ops);

#ifdef CONFIG_OF_IRQ
void irq_domain_generate_simple(const struct of_device_id *match,
				u64 phys_base, unsigned int irq_start)
{
	struct device_node *node;
	pr_debug("looking for phys_base=%llx, irq_start=%i\n",
		(unsigned long long) phys_base, (int) irq_start);
	node = of_find_matching_node_by_address(NULL, match, phys_base);
	if (node)
		irq_domain_add_legacy(node, 32, irq_start, 0,
				      &irq_domain_simple_ops, NULL);
}
EXPORT_SYMBOL_GPL(irq_domain_generate_simple);
#endif
