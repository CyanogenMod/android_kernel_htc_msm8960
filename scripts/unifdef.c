/*
 * Copyright (c) 2002 - 2011 Tony Finch <dot@dotat.at>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * unifdef - remove ifdef'ed lines
 *
 * This code was derived from software contributed to Berkeley by Dave Yost.
 * It was rewritten to support ANSI C by Tony Finch. The original version
 * of unifdef carried the 4-clause BSD copyright licence. None of its code
 * remains in this version (though some of the names remain) so it now
 * carries a more liberal licence.
 *
 *  Wishlist:
 *      provide an option which will append the name of the
 *        appropriate symbol after #else's and #endif's
 *      provide an option which will check symbols after
 *        #else's and #endif's to see that they match their
 *        corresponding #ifdef or #ifndef
 *
 *   These require better buffer handling, which would also make
 *   it possible to handle all "dodgy" directives correctly.
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char copyright[] =
    "@(#) $Version: unifdef-2.5 $\n"
    "@(#) $Author: Tony Finch (dot@dotat.at) $\n"
    "@(#) $URL: http://dotat.at/prog/unifdef $\n"
;

typedef enum {
	LT_TRUEI,		
	LT_FALSEI,		
	LT_IF,			
	LT_TRUE,		
	LT_FALSE,		
	LT_ELIF,		
	LT_ELTRUE,		
	LT_ELFALSE,		
	LT_ELSE,		
	LT_ENDIF,		
	LT_DODGY,		
	LT_DODGY_LAST = LT_DODGY + LT_ENDIF,
	LT_PLAIN,		
	LT_EOF,			
	LT_ERROR,		
	LT_COUNT
} Linetype;

static char const * const linetype_name[] = {
	"TRUEI", "FALSEI", "IF", "TRUE", "FALSE",
	"ELIF", "ELTRUE", "ELFALSE", "ELSE", "ENDIF",
	"DODGY TRUEI", "DODGY FALSEI",
	"DODGY IF", "DODGY TRUE", "DODGY FALSE",
	"DODGY ELIF", "DODGY ELTRUE", "DODGY ELFALSE",
	"DODGY ELSE", "DODGY ENDIF",
	"PLAIN", "EOF", "ERROR"
};

typedef enum {
	IS_OUTSIDE,
	IS_FALSE_PREFIX,	
	IS_TRUE_PREFIX,		
	IS_PASS_MIDDLE,		
	IS_FALSE_MIDDLE,	
	IS_TRUE_MIDDLE,		
	IS_PASS_ELSE,		
	IS_FALSE_ELSE,		
	IS_TRUE_ELSE,		
	IS_FALSE_TRAILER,	
	IS_COUNT
} Ifstate;

static char const * const ifstate_name[] = {
	"OUTSIDE", "FALSE_PREFIX", "TRUE_PREFIX",
	"PASS_MIDDLE", "FALSE_MIDDLE", "TRUE_MIDDLE",
	"PASS_ELSE", "FALSE_ELSE", "TRUE_ELSE",
	"FALSE_TRAILER"
};

typedef enum {
	NO_COMMENT = false,	
	C_COMMENT,		
	CXX_COMMENT,		
	STARTING_COMMENT,	
	FINISHING_COMMENT,	
	CHAR_LITERAL,		
	STRING_LITERAL		
} Comment_state;

static char const * const comment_name[] = {
	"NO", "C", "CXX", "STARTING", "FINISHING", "CHAR", "STRING"
};

typedef enum {
	LS_START,		
	LS_HASH,		
	LS_DIRTY		
} Line_state;

static char const * const linestate_name[] = {
	"START", "HASH", "DIRTY"
};

#define	MAXDEPTH        64			
#define	MAXLINE         4096			
#define	MAXSYMS         4096			

#define	EDITSLOP        10

#define TEMPLATE        "unifdef.XXXXXX"


static bool             compblank;		
static bool             lnblank;		
static bool             complement;		
static bool             debugging;		
static bool             iocccok;		
static bool             strictlogic;		
static bool             killconsts;		
static bool             lnnum;			
static bool             symlist;		
static bool             symdepth;		
static bool             text;			

static const char      *symname[MAXSYMS];	
static const char      *value[MAXSYMS];		
static bool             ignore[MAXSYMS];	
static int              nsyms;			

static FILE            *input;			
static const char      *filename;		
static int              linenum;		
static FILE            *output;			
static const char      *ofilename;		
static bool             overwriting;		
static char             tempname[FILENAME_MAX];	

static char             tline[MAXLINE+EDITSLOP];
static char            *keyword;		

static const char      *newline;		
static const char       newline_unix[] = "\n";
static const char       newline_crlf[] = "\r\n";

static Comment_state    incomment;		
static Line_state       linestate;		
static Ifstate          ifstate[MAXDEPTH];	
static bool             ignoring[MAXDEPTH];	
static int              stifline[MAXDEPTH];	
static int              depth;			
static int              delcount;		
static unsigned         blankcount;		
static unsigned         blankmax;		
static bool             constexpr;		
static bool             zerosyms = true;	
static bool             firstsym;		

static int              exitstat;		

static void             addsym(bool, bool, char *);
static void             closeout(void);
static void             debug(const char *, ...);
static void             done(void);
static void             error(const char *);
static int              findsym(const char *);
static void             flushline(bool);
static Linetype         parseline(void);
static Linetype         ifeval(const char **);
static void             ignoreoff(void);
static void             ignoreon(void);
static void             keywordedit(const char *);
static void             nest(void);
static void             process(void);
static const char      *skipargs(const char *);
static const char      *skipcomment(const char *);
static const char      *skipsym(const char *);
static void             state(Ifstate);
static int              strlcmp(const char *, const char *, size_t);
static void             unnest(void);
static void             usage(void);
static void             version(void);

#define endsym(c) (!isalnum((unsigned char)c) && c != '_')

int
main(int argc, char *argv[])
{
	int opt;

	while ((opt = getopt(argc, argv, "i:D:U:I:o:bBcdeKklnsStV")) != -1)
		switch (opt) {
		case 'i': 
			opt = *optarg++;
			if (opt == 'D')
				addsym(true, true, optarg);
			else if (opt == 'U')
				addsym(true, false, optarg);
			else
				usage();
			break;
		case 'D': 
			addsym(false, true, optarg);
			break;
		case 'U': 
			addsym(false, false, optarg);
			break;
		case 'I': 
			break;
		case 'b': 
		case 'l': 
			lnblank = true;
			break;
		case 'B': 
			compblank = true;
			break;
		case 'c': 
			complement = true;
			break;
		case 'd':
			debugging = true;
			break;
		case 'e': 
			iocccok = true;
			break;
		case 'K': 
			strictlogic = true;
			break;
		case 'k': 
			killconsts = true;
			break;
		case 'n': 
			lnnum = true;
			break;
		case 'o': 
			ofilename = optarg;
			break;
		case 's': 
			symlist = true;
			break;
		case 'S': 
			symlist = symdepth = true;
			break;
		case 't': 
			text = true;
			break;
		case 'V': 
			version();
		default:
			usage();
		}
	argc -= optind;
	argv += optind;
	if (compblank && lnblank)
		errx(2, "-B and -b are mutually exclusive");
	if (argc > 1) {
		errx(2, "can only do one file");
	} else if (argc == 1 && strcmp(*argv, "-") != 0) {
		filename = *argv;
		input = fopen(filename, "rb");
		if (input == NULL)
			err(2, "can't open %s", filename);
	} else {
		filename = "[stdin]";
		input = stdin;
	}
	if (ofilename == NULL) {
		ofilename = "[stdout]";
		output = stdout;
	} else {
		struct stat ist, ost;
		if (stat(ofilename, &ost) == 0 &&
		    fstat(fileno(input), &ist) == 0)
			overwriting = (ist.st_dev == ost.st_dev
				    && ist.st_ino == ost.st_ino);
		if (overwriting) {
			const char *dirsep;
			int ofd;

			dirsep = strrchr(ofilename, '/');
			if (dirsep != NULL)
				snprintf(tempname, sizeof(tempname),
				    "%.*s/" TEMPLATE,
				    (int)(dirsep - ofilename), ofilename);
			else
				snprintf(tempname, sizeof(tempname),
				    TEMPLATE);
			ofd = mkstemp(tempname);
			if (ofd != -1)
				output = fdopen(ofd, "wb+");
			if (output == NULL)
				err(2, "can't create temporary file");
			fchmod(ofd, ist.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO));
		} else {
			output = fopen(ofilename, "wb");
			if (output == NULL)
				err(2, "can't open %s", ofilename);
		}
	}
	process();
	abort(); 
}

static void
version(void)
{
	const char *c = copyright;
	for (;;) {
		while (*++c != '$')
			if (*c == '\0')
				exit(0);
		while (*++c != '$')
			putc(*c, stderr);
		putc('\n', stderr);
	}
}

static void
usage(void)
{
	fprintf(stderr, "usage: unifdef [-bBcdeKknsStV] [-Ipath]"
	    " [-Dsym[=val]] [-Usym] [-iDsym[=val]] [-iUsym] ... [file]\n");
	exit(2);
}

typedef void state_fn(void);

static void Eelif (void) { error("Inappropriate #elif"); }
static void Eelse (void) { error("Inappropriate #else"); }
static void Eendif(void) { error("Inappropriate #endif"); }
static void Eeof  (void) { error("Premature EOF"); }
static void Eioccc(void) { error("Obfuscated preprocessor control line"); }
static void print (void) { flushline(true); }
static void drop  (void) { flushline(false); }
static void Strue (void) { drop();  ignoreoff(); state(IS_TRUE_PREFIX); }
static void Sfalse(void) { drop();  ignoreoff(); state(IS_FALSE_PREFIX); }
static void Selse (void) { drop();               state(IS_TRUE_ELSE); }
static void Pelif (void) { print(); ignoreoff(); state(IS_PASS_MIDDLE); }
static void Pelse (void) { print();              state(IS_PASS_ELSE); }
static void Pendif(void) { print(); unnest(); }
static void Dfalse(void) { drop();  ignoreoff(); state(IS_FALSE_TRAILER); }
static void Delif (void) { drop();  ignoreoff(); state(IS_FALSE_MIDDLE); }
static void Delse (void) { drop();               state(IS_FALSE_ELSE); }
static void Dendif(void) { drop();  unnest(); }
static void Fdrop (void) { nest();  Dfalse(); }
static void Fpass (void) { nest();  Pelif(); }
static void Ftrue (void) { nest();  Strue(); }
static void Ffalse(void) { nest();  Sfalse(); }
static void Oiffy (void) { if (!iocccok) Eioccc(); Fpass(); ignoreon(); }
static void Oif   (void) { if (!iocccok) Eioccc(); Fpass(); }
static void Oelif (void) { if (!iocccok) Eioccc(); Pelif(); }
static void Idrop (void) { Fdrop();  ignoreon(); }
static void Itrue (void) { Ftrue();  ignoreon(); }
static void Ifalse(void) { Ffalse(); ignoreon(); }
static void Mpass (void) { strncpy(keyword, "if  ", 4); Pelif(); }
static void Mtrue (void) { keywordedit("else");  state(IS_TRUE_MIDDLE); }
static void Melif (void) { keywordedit("endif"); state(IS_FALSE_TRAILER); }
static void Melse (void) { keywordedit("endif"); state(IS_FALSE_ELSE); }

static state_fn * const trans_table[IS_COUNT][LT_COUNT] = {
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Eelif, Eelif, Eelif, Eelse, Eendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Eelif, Eelif, Eelif, Eelse, Eendif,
  print, done,  abort },
{ Idrop, Idrop, Fdrop, Fdrop, Fdrop, Mpass, Strue, Sfalse,Selse, Dendif,
  Idrop, Idrop, Fdrop, Fdrop, Fdrop, Mpass, Eioccc,Eioccc,Eioccc,Eioccc,
  drop,  Eeof,  abort },
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Dfalse,Dfalse,Dfalse,Delse, Dendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Eioccc,Eioccc,Eioccc,Eioccc,Eioccc,
  print, Eeof,  abort },
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Pelif, Mtrue, Delif, Pelse, Pendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Pelif, Oelif, Oelif, Pelse, Pendif,
  print, Eeof,  abort },
{ Idrop, Idrop, Fdrop, Fdrop, Fdrop, Pelif, Mtrue, Delif, Pelse, Pendif,
  Idrop, Idrop, Fdrop, Fdrop, Fdrop, Eioccc,Eioccc,Eioccc,Eioccc,Eioccc,
  drop,  Eeof,  abort },
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Melif, Melif, Melif, Melse, Pendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Eioccc,Eioccc,Eioccc,Eioccc,Pendif,
  print, Eeof,  abort },
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Eelif, Eelif, Eelif, Eelse, Pendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Eelif, Eelif, Eelif, Eelse, Pendif,
  print, Eeof,  abort },
{ Idrop, Idrop, Fdrop, Fdrop, Fdrop, Eelif, Eelif, Eelif, Eelse, Dendif,
  Idrop, Idrop, Fdrop, Fdrop, Fdrop, Eelif, Eelif, Eelif, Eelse, Eioccc,
  drop,  Eeof,  abort },
{ Itrue, Ifalse,Fpass, Ftrue, Ffalse,Eelif, Eelif, Eelif, Eelse, Dendif,
  Oiffy, Oiffy, Fpass, Oif,   Oif,   Eelif, Eelif, Eelif, Eelse, Eioccc,
  print, Eeof,  abort },
{ Idrop, Idrop, Fdrop, Fdrop, Fdrop, Dfalse,Dfalse,Dfalse,Delse, Dendif,
  Idrop, Idrop, Fdrop, Fdrop, Fdrop, Dfalse,Dfalse,Dfalse,Delse, Eioccc,
  drop,  Eeof,  abort }
};

static void
ignoreoff(void)
{
	if (depth == 0)
		abort(); 
	ignoring[depth] = ignoring[depth-1];
}
static void
ignoreon(void)
{
	ignoring[depth] = true;
}
static void
keywordedit(const char *replacement)
{
	snprintf(keyword, tline + sizeof(tline) - keyword,
	    "%s%s", replacement, newline);
	print();
}
static void
nest(void)
{
	if (depth > MAXDEPTH-1)
		abort(); 
	if (depth == MAXDEPTH-1)
		error("Too many levels of nesting");
	depth += 1;
	stifline[depth] = linenum;
}
static void
unnest(void)
{
	if (depth == 0)
		abort(); 
	depth -= 1;
}
static void
state(Ifstate is)
{
	ifstate[depth] = is;
}

static void
flushline(bool keep)
{
	if (symlist)
		return;
	if (keep ^ complement) {
		bool blankline = tline[strspn(tline, " \t\r\n")] == '\0';
		if (blankline && compblank && blankcount != blankmax) {
			delcount += 1;
			blankcount += 1;
		} else {
			if (lnnum && delcount > 0)
				printf("#line %d%s", linenum, newline);
			fputs(tline, output);
			delcount = 0;
			blankmax = blankcount = blankline ? blankcount + 1 : 0;
		}
	} else {
		if (lnblank)
			fputs(newline, output);
		exitstat = 1;
		delcount += 1;
		blankcount = 0;
	}
	if (debugging)
		fflush(output);
}

static void
process(void)
{
	blankmax = blankcount = 1000;
	for (;;) {
		Linetype lineval = parseline();
		trans_table[ifstate[depth]][lineval]();
		debug("process line %d %s -> %s depth %d",
		    linenum, linetype_name[lineval],
		    ifstate_name[ifstate[depth]], depth);
	}
}

static void
closeout(void)
{
	if (symdepth && !zerosyms)
		printf("\n");
	if (fclose(output) == EOF) {
		warn("couldn't write to %s", ofilename);
		if (overwriting) {
			unlink(tempname);
			errx(2, "%s unchanged", filename);
		} else {
			exit(2);
		}
	}
}

static void
done(void)
{
	if (incomment)
		error("EOF in comment");
	closeout();
	if (overwriting && rename(tempname, ofilename) == -1) {
		warn("couldn't rename temporary file");
		unlink(tempname);
		errx(2, "%s unchanged", ofilename);
	}
	exit(exitstat);
}

static Linetype
parseline(void)
{
	const char *cp;
	int cursym;
	int kwlen;
	Linetype retval;
	Comment_state wascomment;

	linenum++;
	if (fgets(tline, MAXLINE, input) == NULL)
		return (LT_EOF);
	if (newline == NULL) {
		if (strrchr(tline, '\n') == strrchr(tline, '\r') + 1)
			newline = newline_crlf;
		else
			newline = newline_unix;
	}
	retval = LT_PLAIN;
	wascomment = incomment;
	cp = skipcomment(tline);
	if (linestate == LS_START) {
		if (*cp == '#') {
			linestate = LS_HASH;
			firstsym = true;
			cp = skipcomment(cp + 1);
		} else if (*cp != '\0')
			linestate = LS_DIRTY;
	}
	if (!incomment && linestate == LS_HASH) {
		keyword = tline + (cp - tline);
		cp = skipsym(cp);
		kwlen = cp - keyword;
		
		if (strncmp(cp, "\\\r\n", 3) == 0 ||
		    strncmp(cp, "\\\n", 2) == 0)
			Eioccc();
		if (strlcmp("ifdef", keyword, kwlen) == 0 ||
		    strlcmp("ifndef", keyword, kwlen) == 0) {
			cp = skipcomment(cp);
			if ((cursym = findsym(cp)) < 0)
				retval = LT_IF;
			else {
				retval = (keyword[2] == 'n')
				    ? LT_FALSE : LT_TRUE;
				if (value[cursym] == NULL)
					retval = (retval == LT_TRUE)
					    ? LT_FALSE : LT_TRUE;
				if (ignore[cursym])
					retval = (retval == LT_TRUE)
					    ? LT_TRUEI : LT_FALSEI;
			}
			cp = skipsym(cp);
		} else if (strlcmp("if", keyword, kwlen) == 0)
			retval = ifeval(&cp);
		else if (strlcmp("elif", keyword, kwlen) == 0)
			retval = ifeval(&cp) - LT_IF + LT_ELIF;
		else if (strlcmp("else", keyword, kwlen) == 0)
			retval = LT_ELSE;
		else if (strlcmp("endif", keyword, kwlen) == 0)
			retval = LT_ENDIF;
		else {
			linestate = LS_DIRTY;
			retval = LT_PLAIN;
		}
		cp = skipcomment(cp);
		if (*cp != '\0') {
			linestate = LS_DIRTY;
			if (retval == LT_TRUE || retval == LT_FALSE ||
			    retval == LT_TRUEI || retval == LT_FALSEI)
				retval = LT_IF;
			if (retval == LT_ELTRUE || retval == LT_ELFALSE)
				retval = LT_ELIF;
		}
		if (retval != LT_PLAIN && (wascomment || incomment)) {
			retval += LT_DODGY;
			if (incomment)
				linestate = LS_DIRTY;
		}
		if (linestate == LS_HASH) {
			size_t len = cp - tline;
			if (fgets(tline + len, MAXLINE - len, input) == NULL) {
				
				strcpy(tline + len, newline);
				cp += strlen(newline);
				linestate = LS_START;
			} else {
				linestate = LS_DIRTY;
			}
		}
	}
	if (linestate == LS_DIRTY) {
		while (*cp != '\0')
			cp = skipcomment(cp + 1);
	}
	debug("parser line %d state %s comment %s line", linenum,
	    comment_name[incomment], linestate_name[linestate]);
	return (retval);
}

static Linetype op_strict(int *p, int v, Linetype at, Linetype bt) {
	if(at == LT_IF || bt == LT_IF) return (LT_IF);
	return (*p = v, v ? LT_TRUE : LT_FALSE);
}
static Linetype op_lt(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a < b, at, bt);
}
static Linetype op_gt(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a > b, at, bt);
}
static Linetype op_le(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a <= b, at, bt);
}
static Linetype op_ge(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a >= b, at, bt);
}
static Linetype op_eq(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a == b, at, bt);
}
static Linetype op_ne(int *p, Linetype at, int a, Linetype bt, int b) {
	return op_strict(p, a != b, at, bt);
}
static Linetype op_or(int *p, Linetype at, int a, Linetype bt, int b) {
	if (!strictlogic && (at == LT_TRUE || bt == LT_TRUE))
		return (*p = 1, LT_TRUE);
	return op_strict(p, a || b, at, bt);
}
static Linetype op_and(int *p, Linetype at, int a, Linetype bt, int b) {
	if (!strictlogic && (at == LT_FALSE || bt == LT_FALSE))
		return (*p = 0, LT_FALSE);
	return op_strict(p, a && b, at, bt);
}

struct ops;

typedef Linetype eval_fn(const struct ops *, int *, const char **);

static eval_fn eval_table, eval_unary;

static const struct ops {
	eval_fn *inner;
	struct op {
		const char *str;
		Linetype (*fn)(int *, Linetype, int, Linetype, int);
	} op[5];
} eval_ops[] = {
	{ eval_table, { { "||", op_or } } },
	{ eval_table, { { "&&", op_and } } },
	{ eval_table, { { "==", op_eq },
			{ "!=", op_ne } } },
	{ eval_unary, { { "<=", op_le },
			{ ">=", op_ge },
			{ "<", op_lt },
			{ ">", op_gt } } }
};

static Linetype
eval_unary(const struct ops *ops, int *valp, const char **cpp)
{
	const char *cp;
	char *ep;
	int sym;
	bool defparen;
	Linetype lt;

	cp = skipcomment(*cpp);
	if (*cp == '!') {
		debug("eval%d !", ops - eval_ops);
		cp++;
		lt = eval_unary(ops, valp, &cp);
		if (lt == LT_ERROR)
			return (LT_ERROR);
		if (lt != LT_IF) {
			*valp = !*valp;
			lt = *valp ? LT_TRUE : LT_FALSE;
		}
	} else if (*cp == '(') {
		cp++;
		debug("eval%d (", ops - eval_ops);
		lt = eval_table(eval_ops, valp, &cp);
		if (lt == LT_ERROR)
			return (LT_ERROR);
		cp = skipcomment(cp);
		if (*cp++ != ')')
			return (LT_ERROR);
	} else if (isdigit((unsigned char)*cp)) {
		debug("eval%d number", ops - eval_ops);
		*valp = strtol(cp, &ep, 0);
		if (ep == cp)
			return (LT_ERROR);
		lt = *valp ? LT_TRUE : LT_FALSE;
		cp = skipsym(cp);
	} else if (strncmp(cp, "defined", 7) == 0 && endsym(cp[7])) {
		cp = skipcomment(cp+7);
		debug("eval%d defined", ops - eval_ops);
		if (*cp == '(') {
			cp = skipcomment(cp+1);
			defparen = true;
		} else {
			defparen = false;
		}
		sym = findsym(cp);
		if (sym < 0) {
			lt = LT_IF;
		} else {
			*valp = (value[sym] != NULL);
			lt = *valp ? LT_TRUE : LT_FALSE;
		}
		cp = skipsym(cp);
		cp = skipcomment(cp);
		if (defparen && *cp++ != ')')
			return (LT_ERROR);
		constexpr = false;
	} else if (!endsym(*cp)) {
		debug("eval%d symbol", ops - eval_ops);
		sym = findsym(cp);
		cp = skipsym(cp);
		if (sym < 0) {
			lt = LT_IF;
			cp = skipargs(cp);
		} else if (value[sym] == NULL) {
			*valp = 0;
			lt = LT_FALSE;
		} else {
			*valp = strtol(value[sym], &ep, 0);
			if (*ep != '\0' || ep == value[sym])
				return (LT_ERROR);
			lt = *valp ? LT_TRUE : LT_FALSE;
			cp = skipargs(cp);
		}
		constexpr = false;
	} else {
		debug("eval%d bad expr", ops - eval_ops);
		return (LT_ERROR);
	}

	*cpp = cp;
	debug("eval%d = %d", ops - eval_ops, *valp);
	return (lt);
}

static Linetype
eval_table(const struct ops *ops, int *valp, const char **cpp)
{
	const struct op *op;
	const char *cp;
	int val;
	Linetype lt, rt;

	debug("eval%d", ops - eval_ops);
	cp = *cpp;
	lt = ops->inner(ops+1, valp, &cp);
	if (lt == LT_ERROR)
		return (LT_ERROR);
	for (;;) {
		cp = skipcomment(cp);
		for (op = ops->op; op->str != NULL; op++)
			if (strncmp(cp, op->str, strlen(op->str)) == 0)
				break;
		if (op->str == NULL)
			break;
		cp += strlen(op->str);
		debug("eval%d %s", ops - eval_ops, op->str);
		rt = ops->inner(ops+1, &val, &cp);
		if (rt == LT_ERROR)
			return (LT_ERROR);
		lt = op->fn(valp, lt, *valp, rt, val);
	}

	*cpp = cp;
	debug("eval%d = %d", ops - eval_ops, *valp);
	debug("eval%d lt = %s", ops - eval_ops, linetype_name[lt]);
	return (lt);
}

static Linetype
ifeval(const char **cpp)
{
	int ret;
	int val = 0;

	debug("eval %s", *cpp);
	constexpr = killconsts ? false : true;
	ret = eval_table(eval_ops, &val, cpp);
	debug("eval = %d", val);
	return (constexpr ? LT_IF : ret == LT_ERROR ? LT_IF : ret);
}

static const char *
skipcomment(const char *cp)
{
	if (text || ignoring[depth]) {
		for (; isspace((unsigned char)*cp); cp++)
			if (*cp == '\n')
				linestate = LS_START;
		return (cp);
	}
	while (*cp != '\0')
		
		if (strncmp(cp, "\\\r\n", 3) == 0)
			cp += 3;
		else if (strncmp(cp, "\\\n", 2) == 0)
			cp += 2;
		else switch (incomment) {
		case NO_COMMENT:
			if (strncmp(cp, "/\\\r\n", 4) == 0) {
				incomment = STARTING_COMMENT;
				cp += 4;
			} else if (strncmp(cp, "/\\\n", 3) == 0) {
				incomment = STARTING_COMMENT;
				cp += 3;
			} else if (strncmp(cp, "/*", 2) == 0) {
				incomment = C_COMMENT;
				cp += 2;
			} else if (strncmp(cp, "//", 2) == 0) {
				incomment = CXX_COMMENT;
				cp += 2;
			} else if (strncmp(cp, "\'", 1) == 0) {
				incomment = CHAR_LITERAL;
				linestate = LS_DIRTY;
				cp += 1;
			} else if (strncmp(cp, "\"", 1) == 0) {
				incomment = STRING_LITERAL;
				linestate = LS_DIRTY;
				cp += 1;
			} else if (strncmp(cp, "\n", 1) == 0) {
				linestate = LS_START;
				cp += 1;
			} else if (strchr(" \r\t", *cp) != NULL) {
				cp += 1;
			} else
				return (cp);
			continue;
		case CXX_COMMENT:
			if (strncmp(cp, "\n", 1) == 0) {
				incomment = NO_COMMENT;
				linestate = LS_START;
			}
			cp += 1;
			continue;
		case CHAR_LITERAL:
		case STRING_LITERAL:
			if ((incomment == CHAR_LITERAL && cp[0] == '\'') ||
			    (incomment == STRING_LITERAL && cp[0] == '\"')) {
				incomment = NO_COMMENT;
				cp += 1;
			} else if (cp[0] == '\\') {
				if (cp[1] == '\0')
					cp += 1;
				else
					cp += 2;
			} else if (strncmp(cp, "\n", 1) == 0) {
				if (incomment == CHAR_LITERAL)
					error("unterminated char literal");
				else
					error("unterminated string literal");
			} else
				cp += 1;
			continue;
		case C_COMMENT:
			if (strncmp(cp, "*\\\r\n", 4) == 0) {
				incomment = FINISHING_COMMENT;
				cp += 4;
			} else if (strncmp(cp, "*\\\n", 3) == 0) {
				incomment = FINISHING_COMMENT;
				cp += 3;
			} else if (strncmp(cp, "*/", 2) == 0) {
				incomment = NO_COMMENT;
				cp += 2;
			} else
				cp += 1;
			continue;
		case STARTING_COMMENT:
			if (*cp == '*') {
				incomment = C_COMMENT;
				cp += 1;
			} else if (*cp == '/') {
				incomment = CXX_COMMENT;
				cp += 1;
			} else {
				incomment = NO_COMMENT;
				linestate = LS_DIRTY;
			}
			continue;
		case FINISHING_COMMENT:
			if (*cp == '/') {
				incomment = NO_COMMENT;
				cp += 1;
			} else
				incomment = C_COMMENT;
			continue;
		default:
			abort(); 
		}
	return (cp);
}

static const char *
skipargs(const char *cp)
{
	const char *ocp = cp;
	int level = 0;
	cp = skipcomment(cp);
	if (*cp != '(')
		return (cp);
	do {
		if (*cp == '(')
			level++;
		if (*cp == ')')
			level--;
		cp = skipcomment(cp+1);
	} while (level != 0 && *cp != '\0');
	if (level == 0)
		return (cp);
	else
	
		return (ocp);
}

static const char *
skipsym(const char *cp)
{
	while (!endsym(*cp))
		++cp;
	return (cp);
}

static int
findsym(const char *str)
{
	const char *cp;
	int symind;

	cp = skipsym(str);
	if (cp == str)
		return (-1);
	if (symlist) {
		if (symdepth && firstsym)
			printf("%s%3d", zerosyms ? "" : "\n", depth);
		firstsym = zerosyms = false;
		printf("%s%.*s%s",
		    symdepth ? " " : "",
		    (int)(cp-str), str,
		    symdepth ? "" : "\n");
		
		return (0);
	}
	for (symind = 0; symind < nsyms; ++symind) {
		if (strlcmp(symname[symind], str, cp-str) == 0) {
			debug("findsym %s %s", symname[symind],
			    value[symind] ? value[symind] : "");
			return (symind);
		}
	}
	return (-1);
}

static void
addsym(bool ignorethis, bool definethis, char *sym)
{
	int symind;
	char *val;

	symind = findsym(sym);
	if (symind < 0) {
		if (nsyms >= MAXSYMS)
			errx(2, "too many symbols");
		symind = nsyms++;
	}
	symname[symind] = sym;
	ignore[symind] = ignorethis;
	val = sym + (skipsym(sym) - sym);
	if (definethis) {
		if (*val == '=') {
			value[symind] = val+1;
			*val = '\0';
		} else if (*val == '\0')
			value[symind] = "1";
		else
			usage();
	} else {
		if (*val != '\0')
			usage();
		value[symind] = NULL;
	}
	debug("addsym %s=%s", symname[symind],
	    value[symind] ? value[symind] : "undef");
}

static int
strlcmp(const char *s, const char *t, size_t n)
{
	while (n-- && *t != '\0')
		if (*s != *t)
			return ((unsigned char)*s - (unsigned char)*t);
		else
			++s, ++t;
	return ((unsigned char)*s);
}

static void
debug(const char *msg, ...)
{
	va_list ap;

	if (debugging) {
		va_start(ap, msg);
		vwarnx(msg, ap);
		va_end(ap);
	}
}

static void
error(const char *msg)
{
	if (depth == 0)
		warnx("%s: %d: %s", filename, linenum, msg);
	else
		warnx("%s: %d: %s (#if line %d depth %d)",
		    filename, linenum, msg, stifline[depth], depth);
	closeout();
	errx(2, "output may be truncated");
}
