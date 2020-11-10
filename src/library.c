#include "library.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>

#include "library_console.h"
#include "library_shell.h"
#include "library_file.h"
#include "library_job.h"
#include "library_printf.h"
#include "library_exec.h"


static const unsigned char js_init[]={
#include "js_init.h"
	, 0x00	// line end
};


static void jsB_load(js_State *J)
{
	int i, n=js_gettop(J)-1;
	for (i=0; i<n; i++)
	{
		js_loadfile(J, js_tostring(J, 1+i));
		js_pushundefined(J);
		js_call(J, 0);
		if (i < n-1) js_pop(J, 1);
	}
	if (n == 0)
		js_pushundefined(J);
}


static void jsB_exit(js_State *J)
{
	int ret=0;
	if (js_isnumber(J, 1))
		ret=js_tonumber(J, 1);
	
	exit(ret);
}


void initLibrary(js_State *J)
{
	// Basic functions
	js_newcfunction(J, jsB_load, "load", 1);
	js_setglobal(J, "load");
	
	js_newcfunction(J, jsB_exit, "exit", 1);
	js_setglobal(J, "exit");
	
	// Console i/o
	initLibrary_console(J);
	
	// Shell tools
	initLibrary_shell(J);
	
	// File i/o
	initLibrary_file(J);
	
	// Job control
	initLibrary_job(J);
	
	// printf
	initLibrary_printf(J);
	
	// Exec
	initLibrary_exec(J);
	
	
	// JavaScript-written library functions
	js_dostring(J, (const char*)js_init);
}
