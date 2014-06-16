/* arch/arm/mach-msm/htc_port_list.c
 * Copyright (C) 2009 HTC Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/wakelock.h>
#include <linux/list.h>
#include <mach/msm_iomap.h>
#include <net/tcp.h>

static struct mutex port_lock;
static struct wake_lock port_suspend_lock;

#define MAX_FILTER_PORT 127
static int port_cnt = 0;

static int usb_enable = 0;
struct p_list {
	struct list_head list;
	int no;
};
static struct p_list curr_port_list;
static int packet_filter_flag = 1;
struct class *p_class;
static struct miscdevice portlist_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "htc-portlist",
};

static int htc_port_list_init_success = 0;

#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
struct streaming_port_list_elem {
    struct list_head list;
    __be16 port;
};
struct streaming_sock_list_elem {
    struct list_head list;
    struct socket * sock;
    __be16 port;
};

static struct list_head streaming_port_list;
static struct list_head streaming_sock_list;

static DEFINE_MUTEX(streaming_port_list_lock);
static DEFINE_MUTEX(streaming_sock_list_lock);

static bool streaming_port_add(__be16 port)
{
    bool ret = false;
    struct list_head *listptr = NULL;
    struct streaming_port_list_elem *list_elem = NULL;
    struct streaming_port_list_elem *new_list_elem = NULL;

    if ( htc_port_list_init_success == 0 )
        return ret;

    mutex_lock(&streaming_port_list_lock);

    
    list_for_each(listptr, &streaming_port_list) {
        list_elem = list_entry(listptr, struct streaming_port_list_elem, list);
        if (list_elem && list_elem->port == port) {
            pr_err("%s(%d): Port %d is already in the list!", __func__, __LINE__, be16_to_cpu(port));
            goto exit_out;
        }
    }

    
    new_list_elem = kmalloc(sizeof(struct streaming_port_list_elem), GFP_KERNEL);
    if (!new_list_elem) {
        pr_err("%s(%d): new_list_elem alloc failed\n", __func__, __LINE__);
        goto exit_out;
    }
    if (new_list_elem) {
        new_list_elem->port = port;
        list_add_tail(&new_list_elem->list, &streaming_port_list);
        pr_info("%s(%d): Add port:%d\n", __func__, __LINE__, be16_to_cpu(port));
        ret = true;
    }

exit_out:
    mutex_unlock(&streaming_port_list_lock);

    return ret;
}

static bool streaming_port_remove(__be16 port)
{
    bool ret = false;
    struct list_head *listptr = NULL;
    struct streaming_port_list_elem *list_elem = NULL;

    if ( htc_port_list_init_success == 0 )
        return ret;

    mutex_lock(&streaming_port_list_lock);

    list_for_each(listptr, &streaming_port_list) {
        list_elem = list_entry(listptr, struct streaming_port_list_elem, list);
        if (list_elem && list_elem->port == port) {
            pr_info("%s(%d): Remove port:%d\n", __func__, __LINE__, be16_to_cpu(port));
            list_del(&list_elem->list);
            kfree(list_elem);
            ret = true;
            break;
        }
    }

    mutex_unlock(&streaming_port_list_lock);

    if (ret != true)
        pr_err("%s(%d): Remove failed! Port:%d is not in list!\n", __func__, __LINE__, be16_to_cpu(port));
    return ret;
}

bool is_in_streaming_port_list(__be16 port)
{
    bool ret = false;
    struct list_head *listptr = NULL;
    struct streaming_port_list_elem *list_elem = NULL;

    if ( htc_port_list_init_success == 0 )
        return ret;

    mutex_lock(&streaming_port_list_lock);

    list_for_each(listptr, &streaming_port_list) {
        list_elem = list_entry(listptr, struct streaming_port_list_elem, list);
        if (list_elem && list_elem->port == port) {
            ret = true;
            break;
        }
    }

    mutex_unlock(&streaming_port_list_lock);

    return ret;
}


static ssize_t streaming_port_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
    char *s = buf;
    struct list_head *listptr;
    struct streaming_port_list_elem *list_elem;

    if ( htc_port_list_init_success == 0 )
        return 0;

    mutex_lock(&streaming_port_list_lock);

    list_for_each(listptr, &streaming_port_list) {
        list_elem = list_entry(listptr, struct streaming_port_list_elem, list);
        if (list_elem && list_elem != NULL) {
            s += sprintf(s, "%d ", be16_to_cpu(list_elem->port));
        }
    }

    mutex_unlock(&streaming_port_list_lock);

    return s - buf;
}

static ssize_t streaming_port_store(struct device *dev, struct device_attribute *attr,  const char *buf, size_t count)
{
    int num;
    int ret;
    __be16 port;

    if ( htc_port_list_init_success == 0 )
        return count;

    if (!dev)
        return -ENODEV;

    ret = sscanf(buf, "%d", &num);
    pr_info("%s(%d): ret:%d num:%d\n", __func__, __LINE__, ret, num);

    if (ret > 0) {
        if (num < 0) {
            port = cpu_to_be16(0-num);
            streaming_port_remove(port);
        }
        else {
            port = cpu_to_be16(num);
            streaming_port_add(port);
        }
    }

    return count;
}

static bool streaming_sock_add(struct socket * sock, __be16 port)
{
    bool ret = false;
    struct list_head *listptr = NULL;
    struct streaming_sock_list_elem *list_elem = NULL;
    struct streaming_sock_list_elem *new_list_elem = NULL;

    if ( htc_port_list_init_success == 0 )
        return ret;

    mutex_lock(&streaming_sock_list_lock);

    
    list_for_each(listptr, &streaming_sock_list) {
        list_elem = list_entry(listptr, struct streaming_sock_list_elem, list);
        if (list_elem && list_elem->sock == sock) {
            pr_err("%s(%d): sock %x is already in the list!", __func__, __LINE__, (unsigned int)sock);
            goto exit_out;
        }
    }

    
    new_list_elem = kmalloc(sizeof(struct streaming_sock_list_elem), GFP_KERNEL);
    if (!new_list_elem) {
        pr_err("%s(%d): new_list_elem alloc failed\n", __func__, __LINE__);
        goto exit_out;
    }
    if (new_list_elem) {
        new_list_elem->sock = sock;
        new_list_elem->port = port;
        list_add_tail(&new_list_elem->list, &streaming_sock_list);
        pr_info("%s(%d):Add sock:%x port:%d\n", __func__, __LINE__, (unsigned int)sock, be16_to_cpu(port));
        ret = true;
    }

exit_out:
    mutex_unlock(&streaming_sock_list_lock);

    return ret;
}

static bool streaming_sock_remove(struct socket * sock)
{
    bool ret = false;
    struct list_head *listptr = NULL;
    struct streaming_sock_list_elem *list_elem = NULL;

    if ( htc_port_list_init_success == 0 )
        return ret;

    mutex_lock(&streaming_sock_list_lock);

    list_for_each(listptr, &streaming_sock_list) {
        list_elem = list_entry(listptr, struct streaming_sock_list_elem, list);
        if (list_elem && list_elem->sock == sock) {
            pr_info("%s(%d):Remove sock:%x port:%d\n", __func__, __LINE__, (unsigned int)list_elem->sock, be16_to_cpu(list_elem->port));
            list_del(&list_elem->list);
            kfree(list_elem);
            ret = true;
            break;
        }
    }

    mutex_unlock(&streaming_sock_list_lock);

    return ret;
}

static ssize_t streaming_sock_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
    char *s = buf;
    struct list_head *listptr;
    struct streaming_sock_list_elem *list_elem;

    if ( htc_port_list_init_success == 0 )
        return 0;

    mutex_lock(&streaming_sock_list_lock);

    list_for_each(listptr, &streaming_sock_list) {
        list_elem = list_entry(listptr, struct streaming_sock_list_elem, list);
        if (list_elem && list_elem != NULL) {
            s += sprintf(s, "%x ", (unsigned int)list_elem->sock);
        }
    }

    mutex_unlock(&streaming_sock_list_lock);

    return s - buf;
}

bool is_streaming_sock_connectted = false;

static DEFINE_MUTEX(sock_connect_lock);
void sock_connect_hook(struct socket *sock, struct sockaddr *address, int addrlen)
{
    struct sockaddr_in *usin = (struct sockaddr_in *)address;
    __be16 conn_port = 0;
    bool is_org_stream_sock_list_empty;

    if (usin) {
        conn_port = usin->sin_port;
    }

    mutex_lock(&sock_connect_lock);

    if (is_in_streaming_port_list(conn_port)) {
        is_org_stream_sock_list_empty = list_empty(&streaming_sock_list);
        if (streaming_sock_add(sock, conn_port)) {
            if (is_org_stream_sock_list_empty) {
                is_streaming_sock_connectted = true;
                pr_info("%s(%d): is_streaming_sock_connectted:%x\n", __func__, __LINE__, is_streaming_sock_connectted);
            }
        }
    }

    mutex_unlock(&sock_connect_lock);
}
EXPORT_SYMBOL(sock_connect_hook);

void sock_disconnect_hook(struct socket *sock)
{
    mutex_lock(&sock_connect_lock);

    if(!list_empty(&streaming_sock_list)) {
        streaming_sock_remove(sock);

        if(list_empty(&streaming_sock_list)) {
            is_streaming_sock_connectted = false;
            pr_info("%s(%d): is_streaming_sock_connectted:%x\n", __func__, __LINE__, is_streaming_sock_connectted);
        }
    }

    mutex_unlock(&sock_connect_lock);
}
EXPORT_SYMBOL(sock_disconnect_hook);
#endif  

static ssize_t htc_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	char *s = buf;

	if ( htc_port_list_init_success == 0 )
		return 0;

	mutex_lock(&port_lock);
	s += sprintf(s, "%d\n", packet_filter_flag);
	mutex_unlock(&port_lock);
	return s - buf;
}

static ssize_t htcport_show(struct device *dev,  struct device_attribute *attr,  char *buf)
{
	char *s = buf;
	struct list_head *listptr;
	struct p_list *entry;

	if ( htc_port_list_init_success == 0 )
		return 0;

	mutex_lock(&port_lock);
	list_for_each(listptr, &curr_port_list.list) {
		entry = list_entry(listptr, struct p_list, list);
		if (entry != NULL) {
			printk(KERN_INFO "[Port list] %s [%d]\n", __func__, entry->no);
		    s += sprintf(s, "%d ", (entry->no));
		}
	}
	mutex_unlock(&port_lock);

	return s - buf;
}

static ssize_t htc_store(struct device *dev, struct device_attribute *attr,  const char *buf, size_t count)
{
	int ret;

	if ( htc_port_list_init_success == 0 )
		return 0;

	mutex_lock(&port_lock);
	if (!strncmp(buf, "0", strlen("0"))) {
		packet_filter_flag = 0;
		printk(KERN_INFO "[Port list] Disable Packet filter\n");
		ret = count;
	} else if (!strncmp(buf, "1", strlen("1"))) {
		packet_filter_flag = 1;
		printk(KERN_INFO "[Port list] Enable Packet filter\n");
		ret = count;
	} else {
		printk(KERN_ERR "[Port list] flag: invalid argument\n");
		ret = -EINVAL;
	}
	mutex_unlock(&port_lock);

	return ret;
}

static DEVICE_ATTR(flag, 0664, htc_show, htc_store);
static DEVICE_ATTR(port, 0664, htcport_show, NULL);
#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
static DEVICE_ATTR(stream_port, 0664, streaming_port_show, streaming_port_store);
static DEVICE_ATTR(stream_sock, 0664, streaming_sock_show, NULL);
#endif  

static int port_list_enable(int enable)
{
	if ( htc_port_list_init_success == 0 )
		return 0;

	if (enable) {
		packet_filter_flag = 1;
		printk(KERN_INFO "[Port list] port_list is enabled.\n");
	} else {
		packet_filter_flag = 0;
		printk(KERN_INFO "[Port list] port_list is disabled.\n");
	}
	return 0;
}

static void update_port_list(void)
{
	size_t count = 0;
	struct list_head *listptr;
	struct p_list *entry;

	if ( htc_port_list_init_success == 0 )
		return;

	list_for_each(listptr, &curr_port_list.list) {
		entry = list_entry(listptr, struct p_list, list);
		count++;
		printk(KERN_INFO "[Port list] update_port_list [%d] = %d\n", count, entry->no);
	}

	if (usb_enable) {
		port_list_enable(0);
	} else {
		if (count <= MAX_FILTER_PORT)
			port_list_enable(1);
		else
			port_list_enable(0);
	}
	port_cnt = count;
}

static struct p_list *add_list(int no)
{
	struct p_list *ptr = NULL;
	struct list_head *listptr;
	struct p_list *entry;
	int get_list = 0;

	if ( htc_port_list_init_success == 0 )
		return 0;

	list_for_each(listptr, &curr_port_list.list) {
		entry = list_entry(listptr, struct p_list, list);
		if (entry->no == no) {
			printk(KERN_INFO "[Port list] Port %d is already in the list!", entry->no);
			get_list = 1;
			break;
		}
	}
	if (!get_list) {
		ptr = kmalloc(sizeof(struct p_list), GFP_KERNEL);
		if (ptr) {
			ptr->no = no;
			list_add_tail(&ptr->list, &curr_port_list.list);
			printk(KERN_INFO "[Port list] Add port [%d]\n", no);
		}
	}
	return (ptr);
}

static void remove_list(int no)
{
	struct list_head *listptr;
	struct p_list *entry;
	int get_list = 0;

	if ( htc_port_list_init_success == 0 )
		return;

	list_for_each(listptr, &curr_port_list.list) {
		entry = list_entry(listptr, struct p_list, list);
		if (entry->no == no) {
			printk(KERN_INFO "[Port list] Remove port [%d]\n", entry->no);
			list_del(&entry->list);
			kfree(entry);
			get_list = 1;
			break;
		}
	}
	if (!get_list)
		printk(KERN_INFO "[Port list] Remove failed! Port number is not in list!\n");
}

int add_or_remove_port(struct sock *sk, int add_or_remove)
{
	struct inet_sock *inet = inet_sk(sk);
	__be32 src = inet->inet_rcv_saddr;
	__u16 srcp = ntohs(inet->inet_sport);

	if ( htc_port_list_init_success == 0 )
		return 0;

	wake_lock(&port_suspend_lock);

	if (sk->sk_state != TCP_LISTEN) {
		wake_unlock(&port_suspend_lock);
		return 0;
	}

	
	if (src != 0x0100007F && srcp != 0) {
		mutex_lock(&port_lock);
		if (add_or_remove)
			add_list(srcp);
		else
			remove_list(srcp);
		update_port_list();

		kobject_uevent(&portlist_misc.this_device->kobj, KOBJ_CHANGE);

		mutex_unlock(&port_lock);
		
	}
	wake_unlock(&port_suspend_lock);
	return 0;
}
EXPORT_SYMBOL(add_or_remove_port);

int update_port_list_charging_state(int enable)
{
	if ( htc_port_list_init_success == 0 )
		return 0;

	wake_lock(&port_suspend_lock);
	if (!packet_filter_flag) {
		wake_unlock(&port_suspend_lock);
		return 0;
	}

	usb_enable = enable;
	mutex_lock(&port_lock);
	if (usb_enable) {
		port_list_enable(0);
	} else {
		if (port_cnt <= MAX_FILTER_PORT)
			port_list_enable(1);
		else
			port_list_enable(0);
	}

	kobject_uevent(&portlist_misc.this_device->kobj, KOBJ_CHANGE);

	mutex_unlock(&port_lock);
	wake_unlock(&port_suspend_lock);
	return 0;
}
EXPORT_SYMBOL(update_port_list_charging_state);

static int __init port_list_init(void)
{
	int ret;
	wake_lock_init(&port_suspend_lock, WAKE_LOCK_SUSPEND, "port_list");
	mutex_init(&port_lock);

	htc_port_list_init_success = 0;

	printk(KERN_INFO "[Port list] init()\n");

	memset(&curr_port_list, 0, sizeof(curr_port_list));
	INIT_LIST_HEAD(&curr_port_list.list);

	#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
	INIT_LIST_HEAD(&streaming_port_list);
	INIT_LIST_HEAD(&streaming_sock_list);
	#endif  

	ret = misc_register(&portlist_misc);
	if (ret < 0) {
		printk(KERN_ERR "[Port list] failed to register misc device!\n");
		goto err_misc_register;
	}

	p_class = class_create(THIS_MODULE, "htc_portlist");
	if (IS_ERR(p_class)) {
		ret = PTR_ERR(p_class);
		p_class = NULL;
		printk(KERN_ERR "[Port list] class_create failed!\n");
		goto err_class_create;
	}

	portlist_misc.this_device = device_create(p_class, NULL, 0 , NULL, "packet_filter");
	if (IS_ERR(portlist_misc.this_device)) {
		ret = PTR_ERR(portlist_misc.this_device);
		portlist_misc.this_device = NULL;
		printk(KERN_ERR "[Port list] device_create failed!\n");
		goto err_device_create;
	}

	ret = device_create_file(portlist_misc.this_device, &dev_attr_flag);
	if (ret < 0) {
		printk(KERN_ERR "[Port list] devices_create_file failed!\n");
		goto err_device_create_file;
	}

	
	ret = device_create_file(portlist_misc.this_device, &dev_attr_port);
	if (ret < 0) {
		printk(KERN_ERR "[Port list] devices_create_file failed!\n");
		goto err_device_create_file;
	}

#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
	
	ret = device_create_file(portlist_misc.this_device, &dev_attr_stream_port);
	if (ret < 0) {
	    printk(KERN_ERR "[Port list] devices_create_file  dev_attr_stream_port failed!\n");
	    goto err_device_create_file;
	}

	ret = device_create_file(portlist_misc.this_device, &dev_attr_stream_sock);
	if (ret < 0) {
	    printk(KERN_ERR "[Port list] devices_create_file  dev_attr_stream_sock failed!\n");
	    goto err_device_create_file;
	}
#endif  

	htc_port_list_init_success = 1;

	#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
	streaming_port_add(cpu_to_be16(1755));  
	streaming_port_add(cpu_to_be16(554));   
	#endif  

	return 0;

err_device_create_file:
	device_destroy(p_class, 0);
err_device_create:
	class_destroy(p_class);
err_class_create:
	misc_deregister(&portlist_misc);
err_misc_register:
	return ret;
}

static void __exit port_list_exit(void) {
	int ret;
	struct list_head *listptr;
	struct p_list *entry;

	if ( htc_port_list_init_success == 0 )
		return;

	device_remove_file(portlist_misc.this_device, &dev_attr_flag);
	device_remove_file(portlist_misc.this_device, &dev_attr_port);

#ifdef CONFIG_MONITOR_STREAMING_PORT_SOCKET
	device_remove_file(portlist_misc.this_device, &dev_attr_stream_port);
	device_remove_file(portlist_misc.this_device, &dev_attr_stream_sock);
#endif  

	device_destroy(p_class, 0);
	class_destroy(p_class);

	ret = misc_deregister(&portlist_misc);
	if (ret < 0)
		printk(KERN_ERR "[Port list] failed to unregister misc device!\n");

	list_for_each(listptr, &curr_port_list.list) {
		entry = list_entry(listptr, struct p_list, list);
		kfree(entry);
	}
}

late_initcall(port_list_init);
module_exit(port_list_exit);

MODULE_AUTHOR("Mio Su <Mio_Su@htc.com>");
MODULE_DESCRIPTION("HTC port list driver");
MODULE_LICENSE("GPL");
