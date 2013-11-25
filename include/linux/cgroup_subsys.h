


#ifdef CONFIG_CPUSETS
SUBSYS(cpuset)
#endif


#ifdef CONFIG_CGROUP_DEBUG
SUBSYS(debug)
#endif


#ifdef CONFIG_CGROUP_SCHED
SUBSYS(cpu_cgroup)
#endif


#ifdef CONFIG_CGROUP_CPUACCT
SUBSYS(cpuacct)
#endif


#ifdef CONFIG_CGROUP_MEM_RES_CTLR
SUBSYS(mem_cgroup)
#endif


#ifdef CONFIG_CGROUP_DEVICE
SUBSYS(devices)
#endif


#ifdef CONFIG_CGROUP_FREEZER
SUBSYS(freezer)
#endif


#ifdef CONFIG_NET_CLS_CGROUP
SUBSYS(net_cls)
#endif


#ifdef CONFIG_BLK_CGROUP
SUBSYS(blkio)
#endif


#ifdef CONFIG_CGROUP_PERF
SUBSYS(perf)
#endif


#ifdef CONFIG_NETPRIO_CGROUP
SUBSYS(net_prio)
#endif


#ifdef CONFIG_CGROUP_TIMER_SLACK
SUBSYS(timer_slack)
#endif

