#ifndef _MATRIX_KEYPAD_H
#define _MATRIX_KEYPAD_H

#include <linux/types.h>
#include <linux/input.h>
#include <linux/of.h>

#define MATRIX_MAX_ROWS		18
#define MATRIX_MAX_COLS		18

#define KEY(row, col, val)	((((row) % (MATRIX_MAX_ROWS)) << 24) |\
				 (((col) % (MATRIX_MAX_COLS)) << 16) |\
				 (val & 0xffff))

#define KEY_ROW(k)		(((k) >> 24) & 0xff)
#define KEY_COL(k)		(((k) >> 16) & 0xff)
#define KEY_VAL(k)		((k) & 0xffff)

#define MATRIX_SCAN_CODE(row, col, row_shift)	(((row) << (row_shift)) + (col))

struct matrix_keymap_data {
	const uint32_t *keymap;
	unsigned int	keymap_size;
};

struct matrix_keypad_platform_data {
	const struct matrix_keymap_data *keymap_data;

	const unsigned int *row_gpios;
	const unsigned int *col_gpios;

	unsigned int	num_row_gpios;
	unsigned int	num_col_gpios;

	unsigned int	col_scan_delay_us;

	
	unsigned int	debounce_ms;

	unsigned int	clustered_irq;
	unsigned int	clustered_irq_flags;

	bool		active_low;
	bool		wakeup;
	bool		no_autorepeat;
};

static inline void
matrix_keypad_build_keymap(const struct matrix_keymap_data *keymap_data,
			   unsigned int row_shift,
			   unsigned short *keymap, unsigned long *keybit)
{
	int i;

	for (i = 0; i < keymap_data->keymap_size; i++) {
		unsigned int key = keymap_data->keymap[i];
		unsigned int row = KEY_ROW(key);
		unsigned int col = KEY_COL(key);
		unsigned short code = KEY_VAL(key);

		keymap[MATRIX_SCAN_CODE(row, col, row_shift)] = code;
		__set_bit(code, keybit);
	}
	__clear_bit(KEY_RESERVED, keybit);
}

#ifdef CONFIG_INPUT_OF_MATRIX_KEYMAP
struct matrix_keymap_data *
matrix_keyboard_of_fill_keymap(struct device_node *np, const char *propname);

void matrix_keyboard_of_free_keymap(const struct matrix_keymap_data *kd);
#else
static inline struct matrix_keymap_data *
matrix_keyboard_of_fill_keymap(struct device_node *np, const char *propname)
{
	return NULL;
}

static inline void
matrix_keyboard_of_free_keymap(const struct matrix_keymap_data *kd)
{
}
#endif

#endif 
