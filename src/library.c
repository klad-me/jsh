#include "library.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>


static const unsigned char js_init[]={
#include "js_init.h"
	, 0x00	// line end
};


static void jsB_load(js_State *J)
{
	int i, n = js_gettop(J);
	for (i = 1; i < n; ++i)
	{
		js_loadfile(J, js_tostring(J, i));
		js_pushundefined(J);
		js_call(J, 0);
		js_pop(J, 1);
	}
	js_pushundefined(J);
}


static void jsB_print(js_State *J)
{
	int i, top = js_gettop(J);
	for (i = 1; i < top; ++i)
	{
		const char *s = js_tostring(J, i);
		fputs(s, stdout);
	}
	fflush(stdout);
	js_pushundefined(J);
}


static void jsB_exit(js_State *J)
{
	int ret=0;
	if (js_isnumber(J, 1)) ret=js_tonumber(J, 1);
	
	exit(ret);
}


void initLibrary(js_State *J)
{
	// Basic functions
	js_newcfunction(J, jsB_load, "load", 1);
	js_setglobal(J, "load");
	
	js_newcfunction(J, jsB_print, "print", 0);
	js_dup(J);
	js_setglobal(J, "print");
	js_setglobal(J, "echo");
	
	js_newcfunction(J, jsB_exit, "exit", 1);
	js_setglobal(J, "exit");
	
	
	// JavaScript-written library functions
	js_dostring(J, (const char*)js_init);
}
