#ifndef _LINUX_UTS_H
#define _LINUX_UTS_H

#ifndef UTS_SYSNAME
#define UTS_SYSNAME "Linux"
#endif

#ifndef UTS_NODENAME
#define UTS_NODENAME CONFIG_DEFAULT_HOSTNAME 
#endif

#ifndef UTS_DOMAINNAME
#define UTS_DOMAINNAME "(none)"	
#endif

#endif
