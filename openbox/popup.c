#include "openbox.h"
#include "frame.h"
#include "window.h"
#include "stacking.h"
#include "render2/render.h"

/* XXX temp */
static int theme_bevel = 1;
static int theme_bwidth = 3;

typedef struct Popup {
    ObWindow obwin;
    Window bg;

    Window icon;
    Window text;

    gboolean hasicon;
    struct RrSurface *s_bg;
    struct RrSurface *s_icon;
    struct RrSurface *s_text;
    int gravity;
    int x;
    int y;
    int w;
    int h;
} Popup;

Popup *popup_new(gboolean hasicon)
{
    XSetWindowAttributes attrib;
    Popup *self;

    self = g_new(Popup, 1);
    self->obwin.type = Window_Internal;
    self->hasicon = hasicon;
    self->gravity = NorthWestGravity;
    self->x = self->y = self->w = self->h = 0;
    stacking_add(INTERNAL_AS_WINDOW(self));
    stacking_raise(INTERNAL_AS_WINDOW(self));

    attrib.override_redirect = True;
    self->bg = XCreateWindow(ob_display, ob_root,
                             0, 0, 1, 1, 0, RrInstanceDepth(ob_render_inst),
                             InputOutput, RrInstanceVisual(ob_render_inst),
                             CWOverrideRedirect, &attrib);
    self->s_bg = RrSurfaceNew(ob_render_inst, RR_SURFACE_PLANAR, self->bg, 0);
    self->s_text = RrSurfaceNewChild(RR_SURFACE_PLANAR, self->s_bg, 1);
    self->text = RrSurfaceWindow(self->s_text);
    if (self->hasicon) {
        self->s_icon = RrSurfaceNewChild(RR_SURFACE_PLANAR, self->s_bg, 1);
        self->icon = RrSurfaceWindow(self->s_icon);
    } else {
        self->s_icon = NULL;
        self->icon = None;
    }

    /* XXX COPY THE APPEARANCES FROM THE THEME...... LIKE THIS SORTA!
    self->a_text = appearance_copy(theme_app_hilite_label);
    */

    return self;
}

void popup_free(Popup *self)
{
    RrSurfaceFree(self->s_bg);
    RrSurfaceFree(self->s_icon);
    RrSurfaceFree(self->s_text);
    XDestroyWindow(ob_display, self->bg);
    stacking_remove(self);
    g_free(self);
}

void popup_position(Popup *self, int gravity, int x, int y)
{
    self->gravity = gravity;
    self->x = x;
    self->y = y;
}

void popup_size(Popup *self, int w, int h)
{
    self->w = w;
    self->h = h;
}

void popup_size_to_string(Popup *self, char *text)
{
    int textw, texth;
    int iconw;

    RrTextureSetText(self->s_text, 0, NULL, RR_LEFT, text);
    RrSurfaceMinSize(self->s_text, &textw, &texth);
    textw += theme_bevel * 2;
    texth += theme_bevel * 2;

    self->h = texth + theme_bevel * 2 + theme_bwidth * 2;
    iconw = (self->hasicon ? texth + theme_bevel : 0);
    self->w = textw + iconw + theme_bevel * 2 + theme_bwidth * 2;
}

void popup_show(Popup *self, char *text, Icon *icon)
{
    int x, y, w, h;
    int textw, texth;
    int iconw;

    /* set up the textures */
    RrTextureSetText(self->s_text, 0, NULL, RR_LEFT, text);

    /* measure the shit out */
    RrSurfaceMinSize(self->s_text, &textw, &texth);
    textw += theme_bevel * 2;
    texth += theme_bevel * 2;

    /* set the sizes up and reget the text sizes from the calculated
       outer sizes */
    if (self->h) {
        h = self->h;
        texth = h - (theme_bevel * 2);
    } else
        h = texth + theme_bevel * 2;
    iconw = (self->hasicon ? texth : 0);
    if (self->w) {
        w = self->w;
        textw = w - (iconw + theme_bevel * (self->hasicon ? 3 : 2));
    } else
        w = textw + iconw + theme_bevel * (self->hasicon ? 3 : 2);
    /* sanity checks to avoid crashes! */
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (textw < 1) textw = 1;
    if (texth < 1) texth = 1;

    /* set up the x coord */
    x = self->x;
    switch (self->gravity) {
    case NorthGravity:
    case CenterGravity:
    case SouthGravity:
        x -= w / 2;
        break;
    case NorthEastGravity:
    case EastGravity:
    case SouthEastGravity:
        x -= w;
        break;
    }

    /* set up the y coord */
    y = self->y;
    switch (self->gravity) {
    case WestGravity:
    case CenterGravity:
    case EastGravity:
        y -= h / 2;
        break;
    case SouthWestGravity:
    case SouthGravity:
    case SouthEastGravity:
        y -= h;
        break;
    }

    /* set the surfaces up */
    RrSurfaceSetArea(self->s_bg, x, y, w, h);

    RrSurfaceSetArea(self->s_text,
                     iconw + theme_bevel * (self->hasicon ? 3 : 2) +
                     theme_bwidth,
                     theme_bevel + theme_bwidth,
                     textw, texth);

    if (self->hasicon) {
        RrTextureSetRGBA(self->s_icon, 0, icon->data, 0, 0, icon->width,
                         icon->height);
        if (iconw < 1) iconw = 1; /* sanity check for crashes */
        RrSurfaceSetArea(self->s_icon,
                         theme_bwidth + theme_bevel,
                         theme_bwidth + theme_bevel,
                         iconw, iconw);
    }

    if (!RrSurfaceVisible(self->s_bg)) {
        RrSurfaceShow(self->s_bg);
        stacking_raise(INTERNAL_AS_WINDOW(self));
    }
}

void popup_hide(Popup *self)
{
    if (RrSurfaceVisible(self->s_bg)) {
        RrSurfaceHide(self->s_bg);
    }
}
