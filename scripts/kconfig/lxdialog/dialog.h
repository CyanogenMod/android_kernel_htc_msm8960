/*
 *  dialog.h -- common declarations for all dialog modules
 *
 *  AUTHOR: Savio Lam (lam836@cs.cuhk.hk)
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifndef KBUILD_NO_NLS
# include <libintl.h>
#else
# define gettext(Msgid) ((const char *) (Msgid))
#endif

#ifdef __sun__
#define CURS_MACROS
#endif
#include CURSES_LOC

#if defined(NCURSES_VERSION) && defined(_NEED_WRAP) && !defined(GCC_PRINTFLIKE)
#define OLD_NCURSES 1
#undef  wbkgdset
#define wbkgdset(w,p)		
#else
#define OLD_NCURSES 0
#endif

#define TR(params) _tracef params

#define KEY_ESC 27
#define TAB 9
#define MAX_LEN 2048
#define BUF_SIZE (10*1024)
#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)

#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE '-'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE '|'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE '+'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE '+'
#endif
#ifndef ACS_UARROW
#define ACS_UARROW '^'
#endif
#ifndef ACS_DARROW
#define ACS_DARROW 'v'
#endif

#define ERRDISPLAYTOOSMALL (KEY_MAX + 1)

struct dialog_color {
	chtype atr;	
	int fg;		
	int bg;		
	int hl;		
};

struct dialog_info {
	const char *backtitle;
	struct dialog_color screen;
	struct dialog_color shadow;
	struct dialog_color dialog;
	struct dialog_color title;
	struct dialog_color border;
	struct dialog_color button_active;
	struct dialog_color button_inactive;
	struct dialog_color button_key_active;
	struct dialog_color button_key_inactive;
	struct dialog_color button_label_active;
	struct dialog_color button_label_inactive;
	struct dialog_color inputbox;
	struct dialog_color inputbox_border;
	struct dialog_color searchbox;
	struct dialog_color searchbox_title;
	struct dialog_color searchbox_border;
	struct dialog_color position_indicator;
	struct dialog_color menubox;
	struct dialog_color menubox_border;
	struct dialog_color item;
	struct dialog_color item_selected;
	struct dialog_color tag;
	struct dialog_color tag_selected;
	struct dialog_color tag_key;
	struct dialog_color tag_key_selected;
	struct dialog_color check;
	struct dialog_color check_selected;
	struct dialog_color uarrow;
	struct dialog_color darrow;
};

extern struct dialog_info dlg;
extern char dialog_input_result[];


void item_reset(void);
void item_make(const char *fmt, ...);
void item_add_str(const char *fmt, ...);
void item_set_tag(char tag);
void item_set_data(void *p);
void item_set_selected(int val);
int item_activate_selected(void);
void *item_data(void);
char item_tag(void);

#define MAXITEMSTR 200
struct dialog_item {
	char str[MAXITEMSTR];	
	char tag;
	void *data;	
	int selected;	
};

struct dialog_list {
	struct dialog_item node;
	struct dialog_list *next;
};

extern struct dialog_list *item_cur;
extern struct dialog_list item_nil;
extern struct dialog_list *item_head;

int item_count(void);
void item_set(int n);
int item_n(void);
const char *item_str(void);
int item_is_selected(void);
int item_is_tag(char tag);
#define item_foreach() \
	for (item_cur = item_head ? item_head: item_cur; \
	     item_cur && (item_cur != &item_nil); item_cur = item_cur->next)

int on_key_esc(WINDOW *win);
int on_key_resize(void);

int init_dialog(const char *backtitle);
void set_dialog_backtitle(const char *backtitle);
void end_dialog(int x, int y);
void attr_clear(WINDOW * win, int height, int width, chtype attr);
void dialog_clear(void);
void print_autowrap(WINDOW * win, const char *prompt, int width, int y, int x);
void print_button(WINDOW * win, const char *label, int y, int x, int selected);
void print_title(WINDOW *dialog, const char *title, int width);
void draw_box(WINDOW * win, int y, int x, int height, int width, chtype box,
	      chtype border);
void draw_shadow(WINDOW * win, int y, int x, int height, int width);

int first_alpha(const char *string, const char *exempt);
int dialog_yesno(const char *title, const char *prompt, int height, int width);
int dialog_msgbox(const char *title, const char *prompt, int height,
		  int width, int pause);
int dialog_textbox(const char *title, const char *file, int height, int width);
int dialog_menu(const char *title, const char *prompt,
		const void *selected, int *s_scroll);
int dialog_checklist(const char *title, const char *prompt, int height,
		     int width, int list_height);
extern char dialog_input_result[];
int dialog_inputbox(const char *title, const char *prompt, int height,
		    int width, const char *init);

#define M_EVENT (KEY_MAX+1)
