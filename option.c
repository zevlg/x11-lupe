/* $Header: /usr/home/yav/lupe/RCS/option.c,v 1.3 1998/08/26 07:37:01 yav Exp $
 * Lupe option parse routine
 * written by yav <yav@mte.biglobe.ne.jp>
 */

char rcsid_option[] = "$Id: option.c,v 1.3 1998/08/26 07:37:01 yav Exp $";

#include <stdio.h>
#include "config.h"

/* for strtol prototype */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "extern.h"

#define OPT_FUNC	1
#define OPT_SET_INT	2
#define OPT_GET_INT	3
#define OPT_GET_STR	4

/* for (void *) not supported C-compiler */
#ifdef __STDC__
typedef void *VOIDPTR;
#else
typedef char *VOIDPTR;
#endif

typedef struct {
  char *name;
  int type;
  VOIDPTR parm;
  int setvalue;			/* for OPT_SET_INT */
} OPTION;

static OPTION opttbl[] = {
  {"-help",	OPT_FUNC,	(VOIDPTR)usage,			0},
  {"-V",	OPT_FUNC,	(VOIDPTR)usage,			0},
  {"-h",	OPT_FUNC,	(VOIDPTR)usage,			0},
  {"-?",	OPT_FUNC,	(VOIDPTR)usage,			0},
  {"-display",	OPT_GET_STR,	(VOIDPTR)&str_display,		0},
  {"-geometry",	OPT_GET_STR,	(VOIDPTR)&str_geometry,		0},
  {"-border",	OPT_GET_STR,	(VOIDPTR)&str_border,		0},
  {"-hudfg",	OPT_GET_STR,	(VOIDPTR)&str_hudfg,		0},
  {"-hudbg",	OPT_GET_STR,	(VOIDPTR)&str_hudbg,		0},
  {"-font",	OPT_GET_STR,	(VOIDPTR)&str_font,		0},
  {"-bw",	OPT_GET_INT,	(VOIDPTR)&bw,			0},
  {"-mag",	OPT_GET_INT,	(VOIDPTR)&magstep,		0},
  {"-reticlelen",	OPT_GET_INT,	(VOIDPTR)&reticle_len,	0},
  {"-timer",	OPT_GET_INT,	(VOIDPTR)&photo_delay,		0},
  {"-shape",	OPT_SET_INT,	(VOIDPTR)&shape_mode,		1},
  {"+shape",	OPT_SET_INT,	(VOIDPTR)&shape_mode,		0},
  {"-noshape",	OPT_SET_INT,	(VOIDPTR)&shape_mode,		0},
  {"-update",	OPT_GET_INT,	(VOIDPTR)&update_time,		0},
  {"-delay",	OPT_GET_INT,	(VOIDPTR)&delay_time,		0},
  {"-reticle",	OPT_SET_INT,	(VOIDPTR)&reticle_mode,		1},
  {"-noreticle",OPT_SET_INT,	(VOIDPTR)&reticle_mode,		0},
  {"-hud",	OPT_SET_INT,	(VOIDPTR)&hud_mode,		1},
  {"-nohud",	OPT_SET_INT,	(VOIDPTR)&hud_mode,		0},
  {"-iff",	OPT_SET_INT,	(VOIDPTR)&iff_mode,		1},
  {"-noiff",	OPT_SET_INT,	(VOIDPTR)&iff_mode,		0},
  {"-noimage",	OPT_SET_INT,	(VOIDPTR)&image_mode,		0},
  {NULL,	0,		NULL,				0}
};

int parse_option(ac, av)
     int *ac;
     char ***av;
{
  OPTION *p;
  typedef void (*VOIDFUNCPTR)();
  
  for (p = opttbl; p->name != NULL; p++) {
    if (strcmp(**av, p->name) == 0) {
      --*ac;
      ++*av;
      switch(p->type) {
      case OPT_FUNC:
	((VOIDFUNCPTR)(p->parm))();
	break;
      case OPT_SET_INT:
	*((int *)(p->parm)) = p->setvalue;
	break;
      case OPT_GET_INT:
	if (*ac) {
	  *((int *)(p->parm)) = strtol(**av, NULL, 0);
	  ++*av;
	  --*ac;
	}
	break;
      case OPT_GET_STR:
	if (*ac) {
	  *((char **)(p->parm)) = **av;
	  ++*av;
	  --*ac;
	}
	break;
      }
      return 0;
    }
  }
  return 1;
}

/* End of file */
