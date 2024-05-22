#include "widget.h"

#include "x11adapter.h"

#include <poser/core.h>
#include <stdlib.h>
#include <string.h>

static void destroy(void *obj);
static int show(void *obj);
static int hide(void *obj);
static Size minSize(const void *obj);

static MetaWidget mo = MetaWidget_init("Widget",
	destroy, 0, show, hide, minSize);

struct Widget
{
    Object base;
    PSC_Event *shown;
    PSC_Event *hidden;
    PSC_Event *sizeRequested;
    PSC_Event *sizeChanged;
    Widget *parent;
    ColorSet *colorSet;
    Rect geometry;
    Rect clip;
    Box padding;
    xcb_drawable_t drawable;
    xcb_render_picture_t picture;
    int visible;
    ColorRole backgroundRole;
    int drawBackground;
};

static void destroy(void *obj)
{
    Widget *self = obj;
    ColorSet_destroy(self->colorSet);
    PSC_Event_destroy(self->sizeChanged);
    PSC_Event_destroy(self->sizeRequested);
    PSC_Event_destroy(self->hidden);
    PSC_Event_destroy(self->shown);
    free(self);
}

static int show(void *obj)
{
    Widget *self = Object_instance(obj);
    if (!self->visible)
    {
	self->visible = 1;
	PSC_Event_raise(self->shown, 0, 0);
    }
    return 0;
}

static int hide(void *obj)
{
    Widget *self = Object_instance(obj);
    if (self->visible)
    {
	self->visible = 0;
	PSC_Event_raise(self->hidden, 0, 0);
    }
    return 0;
}

static Size minSize(const void *obj)
{
    (void)obj;
    return (Size){0, 0};
}

Widget *Widget_createBase(void *derived, void *parent)
{
    REGTYPE(0);

    Widget *self = PSC_malloc(sizeof *self);
    memset(self, 0, sizeof *self);
    if (!derived) derived = self;
    self->base.base = Object_create(derived);
    self->base.type = OBJTYPE;
    self->shown = PSC_Event_create(self);
    self->hidden = PSC_Event_create(self);
    self->sizeRequested = PSC_Event_create(self);
    self->sizeChanged = PSC_Event_create(self);
    self->parent = parent;
    self->padding = (Box){ 3, 3, 3, 3 };

    if (parent) Object_own(parent, derived);
    else self->colorSet = ColorSet_create(0xffffffff, 0x000000ff);
    return self;
}

PSC_Event *Widget_shown(void *self)
{
    Widget *w = Object_instance(self);
    return w->shown;
}

PSC_Event *Widget_hidden(void *self)
{
    Widget *w = Object_instance(self);
    return w->hidden;
}

PSC_Event *Widget_sizeRequested(void *self)
{
    Widget *w = Object_instance(self);
    return w->sizeRequested;
}

PSC_Event *Widget_sizeChanged(void *self)
{
    Widget *w = Object_instance(self);
    return w->sizeChanged;
}

Widget *Widget_parent(const void *self)
{
    const Widget *w = Object_instance(self);
    return w->parent;
}

int Widget_draw(void *self)
{
    Widget *w = Object_instance(self);
    if (!w->drawable) return -1;
    if (!w->visible) return 0;
    int rc = -1;
    if (memcmp(&w->geometry, &w->clip, sizeof w->geometry))
    {
	w->clip = w->geometry;
	xcb_rectangle_t clip = {0, 0, w->clip.size.width, w->clip.size.height};
	CHECK(xcb_render_set_picture_clip_rectangles(X11Adapter_connection(),
		    w->picture, w->clip.pos.x, w->clip.pos.y, 1, &clip),
		"Cannot set clipping region on 0x%x", (unsigned)w->drawable);
    }
    if (w->drawBackground)
    {
	xcb_rectangle_t rect = {w->clip.pos.x, w->clip.pos.y,
	    w->clip.size.width, w->clip.size.height};
	Color color = Widget_color(w, w->backgroundRole);
	CHECK(xcb_render_fill_rectangles(X11Adapter_connection(),
		    XCB_RENDER_PICT_OP_OVER, w->picture, Color_xcb(color),
		    1, &rect),
		"Cannot draw widget background on 0x%x", (unsigned)w->picture);
    }
    Object_vcall(rc, Widget, draw, self, w->picture);
    return rc;
}

int Widget_show(void *self)
{
    int rc = -1;
    Object_vcall(rc, Widget, show, self);
    return rc;
}

int Widget_hide(void *self)
{
    int rc = -1;
    Object_vcall(rc, Widget, hide, self);
    return rc;
}

void Widget_setSize(void *self, Size size)
{
    Widget *w = Object_instance(self);
    if (memcmp(&w->geometry.size, &size, sizeof size))
    {
	SizeChangedEventArgs args = { w->geometry.size, size };
	w->geometry.size = size;
	PSC_Event_raise(w->sizeChanged, 0, &args);
    }
}

Size Widget_minSize(const void *self)
{
    Size size = { 0, 0 };
    Object_vcall(size, Widget, minSize, self);
    return size;
}

Size Widget_size(const void *self)
{
    const Widget *w = Object_instance(self);
    return w->geometry.size;
}

void Widget_setPadding(void *self, Box padding)
{
    Widget *w = Object_instance(self);
    w->padding = padding;
}

Box Widget_padding(const void *self)
{
    Widget *w = Object_instance(self);
    return w->padding;
}

void Widget_setOrigin(void *self, Pos pos)
{
    Widget *w = Object_instance(self);
    w->geometry.pos = pos;
}

Pos Widget_origin(const void *self)
{
    const Widget *w = Object_instance(self);
    return w->geometry.pos;
}

const ColorSet *Widget_colorSet(const void *self)
{
    const Widget *w = Object_instance(self);
    if (w->colorSet) return w->colorSet;
    if (w->parent) return Widget_colorSet(w->parent);
    return 0;
}

Color Widget_color(const void *self, ColorRole role)
{
    const ColorSet *colorSet = Widget_colorSet(self);
    if (!colorSet) return 0x00000000;
    return ColorSet_color(colorSet, role);
}

void Widget_setColor(void *self, ColorRole role, Color color)
{
    Widget *w = Object_instance(self);
    if (!w->colorSet)
    {
	const ColorSet *cs = Widget_colorSet(self);
	if (cs) w->colorSet = ColorSet_clone(cs);
	else w->colorSet = ColorSet_create(0xffffffff, 0x000000ff);
    }
    ColorSet_setColor(w->colorSet, role, color);
}

void Widget_setBackground(void *self, int enabled, ColorRole role)
{
    Widget *w = Object_instance(self);
    w->backgroundRole = role;
    w->drawBackground = enabled;
}

xcb_drawable_t Widget_drawable(const void *self)
{
    Widget *w = Object_instance(self);
    return w->drawable;
}

void Widget_setDrawable(void *self, xcb_drawable_t drawable)
{
    Widget *w = Object_instance(self);
    xcb_connection_t *c = X11Adapter_connection();
    if (w->drawable) xcb_render_free_picture(c, w->picture);
    if (drawable)
    {
	w->drawable = drawable;
	w->picture = xcb_generate_id(c);
	CHECK(xcb_render_create_picture(c, w->picture, drawable,
		    X11Adapter_rootformat(), 0, 0),
		"Cannot create XRender picture for 0x%x", (unsigned)drawable);
    }
    else
    {
	w->drawable = 0;
	w->picture = 0;
    }
}

int Widget_visible(const void *self)
{
    const Widget *w = Object_instance(self);
    return w->visible;
}

void Widget_requestSize(void *self)
{
    Widget *w = Object_instance(self);
    PSC_Event_raise(w->sizeRequested, 0, 0);
}

