#ifndef __QSC_DSDA
#define __QSC_DSDA

#ifdef CONFIG_QSC_MODEM
inline int is_qsc_dsda(void) { return true; }
#else
inline int is_qsc_dsda(void) { return false; }
#endif

#endif
