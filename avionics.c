/* $Header: /usr/home/yav/lupe/RCS/avionics.c,v 1.4 1999/05/29 08:49:31 yav Exp $
 * lupe avionics system
 * written by yav <yav@mte.biglobe.ne.jp>
 */

char rcsid_avionics[] = "$Id: avionics.c,v 1.4 1999/05/29 08:49:31 yav Exp $";

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include "config.h"

#include <math.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <string.h>

#include "extern.h"
#include "timer.h"

#define PERCENT(val,rate) (((val)*(rate))/100)

#ifndef M_PI
#define	M_PI 3.14159265358979323846
#endif

/* lupe.c */
extern Display *dsp;
extern int scr;
extern Window topwin;
extern Pixmap srcpixmap;
extern Pixmap dstpixmap;
extern int srcx, srcy;
extern int srcw;
extern int srch;
extern int dstw;
extern int dsth;
extern GC linegc;
extern int osrcx, osrcy;
extern struct TMV cur_time;
extern struct TMV old_time;


static Bool info_disp = True;
static GC hudgc;
static int hud_dir_y;		/* HUD direction indicator ypos */
static int hud_dir_w;		/* HUD direction indicator width */
static int hud_dir_step = 30;
static int hud_alt_x;		/* HUD altitude indicator xpos */
static int hud_alt_h;		/* HUD altitude indicator height */
static int hud_alt_step = 25;
static int attitude_ofs;
static int attitude_len0;
static int attitude_len1;
static int attitude_len2;
static int attutude_step;

static int screen_width = 1280;
static int screen_height = 1024;

int hud_phist_size = 32;

typedef struct _PHIST {
  int p;
  int len;
  int *buf;
} PHIST;

static PHIST xhist;
static PHIST yhist;

void init_phist(PHIST *p, int len)
{
  int i;
  
  p->p = 0;
  p->len = len;
  p->buf = (int *)malloc(sizeof(int) * len);
  for (i = 0; i < len; i++) {
    p->buf[i] = 0;
  }
}

void indicator_setup()
{
  XColor col;
  XFontStruct *fsp;
  XGCValues gcval;
  
  gcval.function = GXinvert;
  hudgc = XCreateGC(dsp, topwin, GCFunction, &gcval);
  if (str_hudfg != NULL) {
    XParseColor(dsp, DefaultColormap(dsp, scr), str_hudfg, &col);
    XAllocColor(dsp, DefaultColormap(dsp, scr), &col);
    XSetForeground(dsp, hudgc, col.pixel);
  }
  if (str_hudbg != NULL) {
    XParseColor(dsp, DefaultColormap(dsp, scr), str_hudbg, &col);
    XAllocColor(dsp, DefaultColormap(dsp, scr), &col);
    XSetBackground(dsp, hudgc, col.pixel);
  }
  fsp = XLoadQueryFont(dsp, str_font);
  if (fsp == NULL)
    fprintf(stderr, "``%s'' load error!\n", str_font);
  else
    XSetFont(dsp, hudgc, fsp->fid);
  
  init_phist(&xhist, hud_phist_size);
  init_phist(&yhist, hud_phist_size);
  
  screen_width = XDisplayWidth(dsp, scr);
  screen_height = XDisplayHeight(dsp, scr);
}

void indicator_resize()
{
  hud_dir_y = PERCENT(dsth, 78);
  hud_dir_w = PERCENT(dstw, 45);
  hud_alt_x = PERCENT(dstw, 78);
  hud_alt_h = PERCENT(dsth, 45);
  attitude_ofs = PERCENT(dsth, 3);
  attitude_len0 = PERCENT(dstw, 40);
  attitude_len1 = PERCENT(dstw, 12);
  attitude_len2 = PERCENT(dstw, 20);
  attutude_step = PERCENT(dsth, 13);
}

void indicator_toggle()
{
  info_disp = !info_disp;
}

void draw_hud_string(x, y, str)
     int x;
     int y;
     char *str;
{
  int len;
  
  len = strlen(str);
  if (str_hudbg == NULL)
    XDrawString(dsp, dstpixmap, hudgc, x, y, str, len);
  else
    XDrawImageString(dsp, dstpixmap, hudgc, x, y, str, len);
}

void draw_reticle(x0, y0)
     int x0;
     int y0;
{
  int i, x, y;
  
  /* vertical line */
  y = (srch/2)*magstep-1;
  i = PERCENT(y+1, reticle_len);
  if (i > 0)
    XDrawLine(dsp, dstpixmap, linegc, x0, y, x0, y-i+1);
  y = (srch/2+1)*magstep;
  i = PERCENT(dsth-y, reticle_len);
  if (i > 0)
    XDrawLine(dsp, dstpixmap, linegc, x0, y, x0, y+i-1);
  /* horizontal line */
  x = (srcw/2)*magstep-1;
  i = PERCENT(x+1, reticle_len);
  if (i > 0)
    XDrawLine(dsp, dstpixmap, linegc, x, y0, x-i+1, y0);
  x = (srcw/2+1)*magstep;
  i = PERCENT(dstw-x, reticle_len);
  if (i > 0)
    XDrawLine(dsp, dstpixmap, linegc, x, y0, x+i-1, y0);
}

void draw_altitule(x0, y0)
     int x0;
     int y0;
{
  int x, y;
  char buf[128];
  
  /* Altitude indicator */
  x = hud_alt_x;
  XDrawLine(dsp, dstpixmap, hudgc, x, y0 - hud_alt_h/2, x, y0 + hud_alt_h/2);
  y = srcy - hud_alt_h/2;
  y -= y % hud_alt_step;
  while ((y += hud_alt_step) < srcy + hud_alt_h/2) {
    XDrawLine(dsp, dstpixmap, hudgc,
	      x+1, y - srcy + dsth/2, x+2, y - srcy + dsth/2);
    sprintf(buf, "%4d", y);
    draw_hud_string(x+3, y - srcy + dsth/2 + 3, buf);
  }
  XDrawLine(dsp, dstpixmap, hudgc, x-1, y0, x-3, y0-2);
  XDrawLine(dsp, dstpixmap, hudgc, x-1, y0, x-3, y0+2);
  sprintf(buf, "%4d", srcy);
  draw_hud_string(x-3-24, y0 - 2, buf);
}

void draw_direction(x0, y0)
     int x0;
     int y0;
{
  int x, y;
  char buf[128];
  
  /* Direction indicator */
  y = hud_dir_y;
  XDrawLine(dsp, dstpixmap, hudgc, x0 - hud_dir_w/2, y, x0 + hud_dir_w/2, y);
  x = srcx - hud_dir_w/2;
  x -= x % hud_dir_step;
  while ((x += hud_dir_step) < srcx + hud_dir_w/2) {
    XDrawLine(dsp, dstpixmap, hudgc,
	      x - srcx + dstw/2, y+1, x - srcx + dstw/2, y+2);
    sprintf(buf, "%4d", x);
    draw_hud_string(x - srcx + dstw/2 - 2*6, y+11, buf);
  }
  XDrawLine(dsp, dstpixmap, hudgc, x0, y-1, x0-2, y-3);
  XDrawLine(dsp, dstpixmap, hudgc, x0, y-1, x0+2, y-3);
  sprintf(buf, "%4d", srcx);
  draw_hud_string(x0 - 24, y - 5, buf);
}

void draw_attitude_symbol(x0, y0)
     int x0;
     int y0;
{
  int i;
  XPoint pts[7];
  static struct {
    short x;
    short y;
  } ofstbl[7] = {
    {-9, 0},
    {-4, 0},
    {-2, 4},
    {0, 1},
    {2, 4},
    {4, 0},
    {9, 0}
  };
  
  for (i = 0; i < 7; i++) {
    pts[i].x = x0 + ofstbl[i].x;
    pts[i].y = y0 + ofstbl[i].y;
  }
  XDrawLines(dsp, dstpixmap, hudgc, pts, 7, CoordModeOrigin);
}

void draw_attitude_line(x0, y0, dy, n, cv, sv)
     int x0;
     int y0;
     int dy;
     int n;
     double cv;
     double sv;
{
  int i, x1, x2, y1, y2;
  int center_skip;
  
  center_skip = dstw / 50;
  i = n / 2;
  x1 = x2 = x0 + dy * sv;
  x1 -= center_skip * cv;
  x2 -= i * cv;
  y1 = y2 = y0 + dy * cv;
  y1 += center_skip * sv;
  y2 += i * sv;
  XDrawLine(dsp, dstpixmap, hudgc, x1, y1, x2, y2);
  
  x1 = x2 = x0 + dy * sv;
  x1 += center_skip * cv;
  x2 += i * cv;
  y1 = y2 = y0 + dy * cv;
  y1 -= center_skip * sv;
  y2 -= i * sv;
  XDrawLine(dsp, dstpixmap, hudgc, x1, y1, x2, y2);
}

int avr(int n, PHIST *p)
{
  int i;
  
  p->buf[p->p] = n;
  p->p = (p->p + 1) % p->len;
  n = 0;
  for (i = 0; i < p->len; i++) {
    n += p->buf[i];
  }
  return n / p->len;
}

void draw_attitude(x0, y0)
     int x0;
     int y0;
{
  int i, vx, vy, len;
  double th, sv, cv;
  long t;
  
  t = diff_time(&cur_time, &old_time);
  if (t < 1)
    t = 1;
  vx = ((srcx - osrcx) * 1000) / t; /* pixel/sec */
  vy = ((srcy - osrcy) * 1000) / t; /* pixel/sec */
  
  vx = avr(vx, &xhist);
  vy = avr(vy, &yhist);

  th = (vx * M_PI/2) / screen_width;
  if (th > M_PI/2)
    th = M_PI/2;
  else if (th < -M_PI/2)
    th = -M_PI/2;
  
  vy = (vy * dsth) / screen_height;
  vy = -vy;
  cv = cos(th);
  sv = sin(th);
  y0 += attitude_ofs;
  for (i = -4; i <= 4; i++) {
    len = i ? ((i & 1)?attitude_len1:attitude_len2) : attitude_len0;
    draw_attitude_line(x0, y0, vy+attutude_step*i, len, cv, sv);
  }
  draw_attitude_symbol(x0, y0);
}

void target_pixel_info()
{
  XColor col;
  char buf[128];
  static XImage *img = NULL;
  
  if (img == NULL)
    img = XGetImage(dsp, srcpixmap, srcw/2, srch/2, 1, 1, -1, ZPixmap);
  else
    XGetSubImage(dsp, srcpixmap, srcw/2, srch/2, 1, 1, -1, ZPixmap, img, 0, 0);
  col.pixel = XGetPixel(img, 0, 0);
  XQueryColor(dsp, DefaultColormap(dsp, scr), &col);
  sprintf(buf, "%3ld (%04x, %04x, %04x) #%02x%02x%02x",
          col.pixel, col.red, col.green, col.blue, col.red>>8, col.green>>8, col.blue>>8);
  draw_hud_string(PERCENT(dstw, 15), PERCENT(dsth, 25), buf);
}

void draw_indicator()
{
  int x0, y0;
  
  if (info_disp) {
    x0 = (srcw/2)*magstep + magstep/2;
    y0 = (srch/2)*magstep + magstep/2;
    if (reticle_mode)
      draw_reticle(x0, y0);
    if (hud_mode) {
      draw_attitude(x0, y0);
      draw_altitule(x0, y0);
      draw_direction(x0, y0);
    }
    if (iff_mode)
      target_pixel_info();
  }
}

/* End of file */
