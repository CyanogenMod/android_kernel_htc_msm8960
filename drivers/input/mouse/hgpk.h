
#ifndef _HGPK_H
#define _HGPK_H

#define HGPK_GS		0xff       
#define HGPK_PT		0xcf       

enum hgpk_model_t {
	HGPK_MODEL_PREA = 0x0a,	
	HGPK_MODEL_A = 0x14,	
	HGPK_MODEL_B = 0x28,	
	HGPK_MODEL_C = 0x3c,
	HGPK_MODEL_D = 0x50,	
};

enum hgpk_spew_flag {
	NO_SPEW,
	MAYBE_SPEWING,
	SPEW_DETECTED,
	RECALIBRATING,
};

#define SPEW_WATCH_COUNT 42  

enum hgpk_mode {
	HGPK_MODE_MOUSE,
	HGPK_MODE_GLIDESENSOR,
	HGPK_MODE_PENTABLET,
	HGPK_MODE_INVALID
};

struct hgpk_data {
	struct psmouse *psmouse;
	enum hgpk_mode mode;
	bool powered;
	enum hgpk_spew_flag spew_flag;
	int spew_count, x_tally, y_tally;	
	unsigned long recalib_window;
	struct delayed_work recalib_wq;
	int abs_x, abs_y;
	int dupe_count;
	int xbigj, ybigj, xlast, ylast; 
	int xsaw_secondary, ysaw_secondary; 
};

#ifdef CONFIG_MOUSE_PS2_OLPC
void hgpk_module_init(void);
int hgpk_detect(struct psmouse *psmouse, bool set_properties);
int hgpk_init(struct psmouse *psmouse);
#else
static inline void hgpk_module_init(void)
{
}
static inline int hgpk_detect(struct psmouse *psmouse, bool set_properties)
{
	return -ENODEV;
}
static inline int hgpk_init(struct psmouse *psmouse)
{
	return -ENODEV;
}
#endif

#endif
