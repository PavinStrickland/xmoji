#include "xmoji.h"

#include "font.h"
#include "textlabel.h"
#include "valuetypes.h"
#include "vbox.h"
#include "window.h"
#include "x11adapter.h"

#include <inttypes.h>
#include <poser/core.h>
#include <stdio.h>
#include <stdlib.h>

static const char *fontname;
static const char *emojifontname = "emoji";
static PSC_LogLevel loglevel = PSC_L_WARNING;

static Font *font;
static Font *emojifont;
static Window *win;

static struct { int argc; char **argv; } startupargs;

static void onclose(void *receiver, void *sender, void *args)
{
    (void)receiver;
    (void)sender;
    (void)args;

    PSC_Service_quit();
}

static void onprestartup(void *receiver, void *sender, void *args)
{
    (void)receiver;
    (void)sender;

    PSC_EAStartup *ea = args;

    if (X11Adapter_init(
		startupargs.argc, startupargs.argv, "Xmoji") < 0) goto error;
    if (Font_init(.15) < 0) goto error;
    font = Font_create(3, fontname);
    emojifont = Font_create(0, emojifontname);
    if (!(win = Window_create(0))) goto error;
    Window_setTitle(win, "Xmoji 😀 äöüß");
    Widget_setColor(win, COLOR_BG_NORMAL, Color_fromRgb(50, 60, 70));
    Widget_setColor(win, COLOR_NORMAL, Color_fromRgb(200, 255, 240));

    VBox *box = VBox_create(win);
    Window_setMainWidget(win, box);

    TextLabel *label = TextLabel_create(box, font);
    TextLabel_setText(label, "Hello, World!\n\n"
	    "This is just a quick little\n"
	    "text rendering test.\n\n"
	    "The quick brown fox jumps over the lazy dog");
    Widget_setAlign(label, AH_CENTER|AV_MIDDLE);
    Widget_show(label);
    VBox_addWidget(box, label);

    label = TextLabel_create(box, emojifont);
    TextLabel_setText(label, "😀🤡🇩🇪👺🧩🔮🐅🍻🧑🏾‍🤝‍🧑🏻");
    Widget_setAlign(label, AH_CENTER|AV_MIDDLE);
    Widget_show(label);
    VBox_addWidget(box, label);

    Widget_show(box);

    PSC_Event_register(Window_closed(win), 0, onclose, 0);
    PSC_Event_register(Window_errored(win), 0, onclose, 0);
    return;

error:
    PSC_EAStartup_return(ea, EXIT_FAILURE);
}

static void onstartup(void *receiver, void *sender, void *args)
{
    (void)receiver;
    (void)sender;
    (void)args;

#ifndef DEBUG
    PSC_Log_setAsync(1);
#endif
    Widget_show(win);
}

static void onshutdown(void *receiver, void *sender, void *args)
{
    (void)receiver;
    (void)sender;
    (void)args;

    Font_destroy(emojifont);
    emojifont = 0;
    Font_destroy(font);
    font = 0;
    Object_destroy(win);
    win = 0;
    Font_done();
    X11Adapter_done();
    PSC_Log_setAsync(0);
}

SOLOCAL int Xmoji_run(int argc, char **argv)
{
    startupargs.argc = argc;
    startupargs.argv = argv;

    for (int i = 1; i < argc; ++i)
    {
	if (i < argc - 1 && !strcmp(argv[i], "-font"))
	{
	    fontname = argv[i+1];
	}
	else if (i < argc - 1 && !strcmp(argv[i], "-emojifont"))
	{
	    emojifontname = argv[i+1];
	}
	else if (loglevel < PSC_L_INFO && !strcmp(argv[i], "-v"))
	{
	    loglevel = PSC_L_INFO;
	}
	else if (loglevel < PSC_L_DEBUG && !strcmp(argv[i], "-vv"))
	{
	    loglevel = PSC_L_DEBUG;
	}
    }

    PSC_RunOpts_foreground();
    PSC_Log_setFileLogger(stderr);
    PSC_Log_setMaxLogLevel(loglevel);
    PSC_Event_register(PSC_Service_prestartup(), 0, onprestartup, 0);
    PSC_Event_register(PSC_Service_startup(), 0, onstartup, 0);
    PSC_Event_register(PSC_Service_shutdown(), 0, onshutdown, 0);
    return PSC_Service_run();
}
