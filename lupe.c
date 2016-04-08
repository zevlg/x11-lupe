/* $Header: /usr/home/yav/lupe/RCS/lupe.c,v 1.4 1999/05/29 08:59:28 yav Exp $
 * Lupe - real-time magnifying glass for X11
 * written by yav <yav@mte.biglobe.ne.jp>
 */

char rcsid_lupe[] = "$Id: lupe.c,v 1.4 1999/05/29 08:59:28 yav Exp $";

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xos.h>
#include <stdlib.h>
#include <stdio.h>
#include "config.h"

#if USE_SHAPE
#include <X11/extensions/shape.h>
#endif

#include "version.h"
#include "timer.h"

#ifdef HAVE_STRING_H
#include <string.h>
#else
#define USE_BCOPY
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#endif

#ifndef USE_BCOPY
#define bcopy(src,dst,len) memcpy(dst,src,len)
#endif

char *str_display = "";
char *str_geometry = "";
char *str_border = NULL;	/* border color */
char *str_hudfg = "#808080";	/* HUD indicator foreground color */
char *str_hudbg = NULL;		/* HUD indicator background color */
char *str_font = "6x10";	/* HUD font */
int magstep = 6;		/* magnify factor */
int bw = 2;			/* border width */
int update_time = 333;		/* 1/3 sec. */
int delay_time = 5000;		/* 5 sec. */
int shape_mode = 1;		/* != 0  use shape extension */
int reticle_mode = 1;		/* != 0  display reticle */
int hud_mode = 1;		/* != 0  Head-Up Display on */
int iff_mode = 1;		/* != 0  IFF in active */
int reticle_len = 15;		/* percent of reticle len 0:none 100:to edge */
int photo_delay = 5;		/* sec. -> millisec. */
int image_mode = 1;
int override_redirect_mode = 0;

Display *dsp;
int scr;
Window topwin;
Pixmap srcpixmap = None;
Pixmap dstpixmap = None;
int srcx, srcy;
int srcw = 25;			/* source width */
int srch = 25;			/* source height */
int dstw;			/* destination width = srcw * magstep */
int dsth;			/* destination height = srch * magstep */
GC linegc;
struct TMV cur_time;		/* current time */
struct TMV old_time;		/* old time */
int osrcx, osrcy;

static char *str_lupe = "Lupe";
static Window rootwin;
static GC gc;
static GC outgc;
static int oargc;
static char **oargv;
static Bool mapped = False;
static Bool quit_flag;
static int photo_mode = 0;	/* 1:count down 2:shot */
static struct TMV photo_time;	/* photo start time */
static XImage *srcimage = NULL;

#include "icon.xbm"

/* option.c */
extern int parse_option();
/* avionics.c */
extern void indicator_setup();
extern void indicator_resize();
extern void indicator_toggle();
extern void draw_indicator();

void usage()
{
  fprintf(stderr, "* Lupe version %s *\n", LUPE_VERSION);
  fprintf(stderr, "written by yav <yav@mte.biglobe.ne.jp>\n");
  fprintf(stderr, "usage: %s [-display DISP] [-geometry WxH+X+Y] [-bw N] [-mag N]\n", *oargv);
  fprintf(stderr, "  [-font <FONT>]\n");
  fprintf(stderr, "  [-shape|-noshape] [-reticle|-noreticle] [-hud|-nohud] [-iff|-noiff]\n");
  fprintf(stderr, "  [-override_redirect|-nooverride_redirect]\n");
  fprintf(stderr, "  [-update milliseconds] [-delay milliseconds] [-timer seconds]\n");
  exit(1);
}

void expose(x, y, w, h)
     int x;
     int y;
     int w;
     int h;
{
  XCopyArea(dsp, dstpixmap, topwin, gc, x, y, w, h, x, y);
}

void magnify()
{
  int i, x, y, x0, y0;
  
  if (image_mode) {
    for (y = 0; y < srch; y++) {
      for (x = 0; x < srcw; x++) {
	XSetForeground(dsp, gc, XGetPixel(srcimage, x, y));
	XFillRectangle(dsp, dstpixmap, gc, x*magstep, y*magstep,
		       magstep, magstep);
      }
    }
  } else {
    for (x = 0; x < srcw; x++) {
      x0 = x*magstep;
      for (i = 0; i < magstep; i++) {
	XCopyArea(dsp, srcpixmap, dstpixmap, gc, x, 0, 1, srch, x0+i, 0);
      }
    }
    x0 = dstw;
    for (y = srch-1; y >= 0; --y) {
      y0 = y*magstep;
      for (i = magstep-1; i >= 0; --i) {
	XCopyArea(dsp, dstpixmap, dstpixmap, gc, 0, y, x0, 1, 0, y0+i);
      }
    }
  }
  draw_indicator();
  expose(0, 0, dstw, dsth);
}

int get_image()
{
  int x, y;
  int rootx, rooty;
  unsigned int kb;
  Window root, child;
  int pointer_move;
  
  bcopy(&cur_time, &old_time, sizeof(struct TMV));
  current_time(&cur_time);
  if (photo_mode == 1 && diff_time(&cur_time, &photo_time) >= photo_delay) {
    photo_mode = 2;
    XBell(dsp, 0);
  }
  if (photo_mode == 2)
    return 0;
  XQueryPointer(dsp, rootwin, &root, &child, &rootx, &rooty, &x, &y, &kb);
  osrcx = srcx;
  osrcy = srcy;
  srcx = x;
  srcy = y;
  pointer_move = srcx != osrcx || srcy != osrcy;
  XFillRectangle(dsp, srcpixmap, outgc, 0, 0, srcw, srch);
  XCopyArea(dsp, rootwin, srcpixmap, gc, x-srcw/2, y-srch/2, srcw, srch, 0, 0);
  if (image_mode)
    XGetSubImage(dsp, srcpixmap, 0,0, srcw, srch, -1, ZPixmap, srcimage, 0, 0);
  magnify();
  return pointer_move;
}

void set_shape()
{
#if USE_SHAPE
  Pixmap bitmap;
  GC gc;
  
  bitmap = XCreatePixmap(dsp, rootwin, dstw, dsth, 1);
  gc = XCreateGC(dsp, bitmap, 0, NULL);
  XSetForeground(dsp, gc, 0);
  XFillRectangle(dsp, bitmap, gc, 0, 0, dstw, dsth);
  XSetForeground(dsp, gc, 1);
  XFillArc(dsp, bitmap, gc, bw, bw, dstw-bw*2, dsth-bw*2, 0, 360*64);
  XShapeCombineMask(dsp, topwin, ShapeClip, 0, 0, bitmap, ShapeSet);
  XFillArc(dsp, bitmap, gc, 0, 0, dstw, dsth, 0, 360*64);
  XShapeCombineMask(dsp, topwin, ShapeBounding, 0, 0, bitmap, ShapeSet);
  XFreeGC(dsp, gc);
  XFreePixmap(dsp, bitmap);
#endif
}

void resize()
{
  srcw = dstw / magstep;
  srch = dsth / magstep;
  if (srcpixmap != None)
    XFreePixmap(dsp, srcpixmap);
  srcpixmap = XCreatePixmap(dsp, rootwin, srcw, srch, DefaultDepth(dsp, scr));
  if (dstpixmap != None)
    XFreePixmap(dsp, dstpixmap);
  dstpixmap = XCreatePixmap(dsp, rootwin, dstw, dsth, DefaultDepth(dsp, scr));
  XFillRectangle(dsp, dstpixmap, gc, 0, 0, dstw, dsth);
  if (shape_mode)
    set_shape();
  if (srcimage != NULL)
    XDestroyImage(srcimage);
  srcimage = XGetImage(dsp, srcpixmap, 0, 0, srcw, srch, -1, ZPixmap);
  indicator_resize();
}

void init_screen()
{
  int f, x, y, w, h;
  XSizeHints hint;
  unsigned long border_pixel;
  XColor col;
  XGCValues gcval;
  Pixmap icon_pixmap;
  int event_base, error_base;
  static char dash_list[2] = {1,1};
  
  dsp = XOpenDisplay(str_display);
  if (dsp == NULL) {
    fprintf(stderr, "Cannot open display connection!\n");
    exit(1);
  }
  scr = DefaultScreen(dsp);
  rootwin = DefaultRootWindow(dsp);
#if USE_SHAPE
  if (shape_mode && !XShapeQueryExtension(dsp, &event_base, &error_base))
    shape_mode = 0;
#else
  shape_mode = 0;
#endif
  border_pixel = BlackPixel(dsp, scr);
  if (str_border != NULL &&
      XParseColor(dsp, DefaultColormap(dsp, scr), str_border, &col) &&
      XAllocColor(dsp, DefaultColormap(dsp, scr), &col))
    border_pixel = col.pixel;
  icon_pixmap = 
    XCreateBitmapFromData(dsp, DefaultRootWindow(dsp), 
			  icon_bits, icon_width, icon_height);
  hint.min_width = hint.min_height =
    hint.width_inc = hint.height_inc = magstep;
  hint.flags = PMinSize|PResizeInc;
  f = XGeometry(dsp, scr, str_geometry, "",
		bw, magstep, magstep, 0, 0, &x, &y, &w, &h);
  if (f & WidthValue) {
    hint.flags |= USSize;
    srcw = w;
  }
  if (f & HeightValue) {
    hint.flags |= USSize;
    srch = h;
  }
  if (f & XValue)
    hint.flags |= USPosition;
  else
    x = 0;
  if (f & YValue)
    hint.flags |= USPosition;
  else
    y = 0;
  dstw = srcw * magstep;
  dsth = srch * magstep;
  hint.x = x;
  hint.y = y;
  hint.width = dstw;
  hint.height = dsth;
  topwin = XCreateSimpleWindow(dsp, rootwin,
			       hint.x, hint.y, hint.width, hint.height,
			       bw, border_pixel, WhitePixel(dsp, scr));
  if (override_redirect_mode) {
    XSetWindowAttributes attr;
    attr.override_redirect = 1;
    XChangeWindowAttributes(dsp, topwin, CWOverrideRedirect, &attr);
  }
        
  XSetWindowBorderPixmap(dsp, topwin, CopyFromParent);
  XSetWindowBackgroundPixmap(dsp, topwin, None);
  XSelectInput(dsp, topwin, ExposureMask|ButtonPressMask|StructureNotifyMask|
	       KeyPressMask);
  XSetStandardProperties(dsp, topwin, str_lupe, str_lupe, icon_pixmap,
			 oargv, oargc, &hint);
  /* setup GC */
  gc = XCreateGC(dsp, rootwin, 0, NULL);
  XSetSubwindowMode(dsp, gc, IncludeInferiors);
  outgc = XCreateGC(dsp, topwin, 0, NULL);
  XSetForeground(dsp, outgc, BlackPixel(dsp, scr));
  gcval.function = GXinvert;
  linegc = XCreateGC(dsp, topwin, GCFunction, &gcval);
  XSetLineAttributes(dsp, linegc, 0, LineDoubleDash, CapButt, JoinMiter);
  XSetDashes(dsp, linegc, 0, dash_list, sizeof(dash_list));
  indicator_setup();
  resize();
  XMapRaised(dsp, topwin);
}

void key_event(p)
     XKeyEvent *p;
{
  int i;
  int keycode;
  KeySym key;
  XComposeStatus cs;
  char buf[32];
  
  i = XLookupString(p, buf, sizeof(buf)-1, &key, &cs);
  buf[i] = '\0';
  keycode = buf[0];
  switch(key) {
  case XK_Escape:
    keycode = 0x1b;
    break;
  case XK_Return:
    keycode = 0x0d;
    break;
  }
  switch(keycode) {
  case ' ':
    indicator_toggle();
    magnify();
    break;
  case 0x0d:
    if (photo_mode) {
      photo_mode = 0;
    } else {
      current_time(&photo_time);
      photo_mode = 1;
    }
    break;
  case 0x1b:
    XIconifyWindow(dsp, topwin, scr);
    break;
  case 'q':
    quit_flag = True;
    break;
  }
}

void adjust_option()
{
  if (magstep < 1)
    magstep = 1;
  if (srcw < 1)
    srcw = 1;
  if (srch < 1)
    srch = 1;
  photo_delay *= 1000;		/* sec. to millisec. */
}

void main(argc, argv)
     int argc;
     char **argv;
{
  XEvent ev;
  struct TMV tm0;
  struct TMV tm1;

  oargc = argc--;
  oargv = argv++;
  while (argc > 0) {
    if (parse_option(&argc, &argv)) {
      fprintf(stderr, "%s: unknown option ``%s''\n", *oargv, *argv);
      usage();
    }
  }
  adjust_option();
  init_screen();
  quit_flag = False;
  current_time(&tm0);
  while (!quit_flag) {
    while (mapped && XEventsQueued(dsp, QueuedAfterReading) == 0) {
      XFlush(dsp);
      current_time(&tm1);
      if (diff_time(&tm1, &tm0) >= delay_time)
	sleep_time(update_time);
      get_image();
    }
    XNextEvent(dsp, &ev);
    switch (ev.type) {
    case MotionNotify:
      while (XCheckTypedEvent(dsp, MotionNotify, &ev))
	;
      if (mapped)
	get_image();
      current_time(&tm0);
      break;
    case Expose:
      expose(ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height);
      break;
    case ButtonPress:
      switch(ev.xbutton.button) {
      case Button1:
	XIconifyWindow(dsp, topwin, scr);
	break;
      case Button2:
      case Button3:
	quit_flag = True;
	break;
      }
      break;
    case MapNotify:
      XSelectInput(dsp, rootwin, PointerMotionHintMask|PointerMotionMask);
      mapped = True;
      break;
    case UnmapNotify:
      XSelectInput(dsp, rootwin, 0);
      mapped = False;
      break;
    case ConfigureNotify:
      dstw = ev.xconfigure.width;
      dsth = ev.xconfigure.height;
      resize();
      break;
    case KeyPress:
      key_event(&ev.xkey);
      break;
    case MappingNotify:
      XRefreshKeyboardMapping(&ev.xmapping);
      break;
    }
  }
  XCloseDisplay(dsp);
  exit(0);
}

/* End of file */
