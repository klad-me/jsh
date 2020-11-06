#include "jsh.h"

#include <stdio.h>
#include <mujs.h>

#include "library.h"


char jsh_exceptions=1;


static int runFile(js_State *J, const char *fname)
{
	if (js_try(J))
	{
		js_report(J, js_trystring(J, -1, "Error"));
		js_pop(J, 1);
		return -1;
	}
	js_loadfile(J, fname);
	js_pushundefined(J);
	js_call(J, 0);
	int ret=0;
	if (js_isnumber(J, -1)) ret=js_tonumber(J, -1);
	js_pop(J, 1);
	js_endtry(J);
	
	return ret;
}


static int runScript(js_State *J, const char *script)
{
	if (js_try(J))
	{
		js_report(J, js_trystring(J, -1, "Error"));
		js_pop(J, 1);
		return -1;
	}
	js_loadstring(J, "[commandLine]", script);
	js_pushundefined(J);
	js_call(J, 0);
	int ret=0;
	if (js_isnumber(J, -1)) ret=js_tonumber(J, -1);
	js_pop(J, 1);
	js_endtry(J);
	
	return ret;
}


static void jsB_exceptions(js_State *J)
{
	if (! js_isboolean(J, 1))
		js_error(J, "incorrect jsh.exceptions() arguments");
	
	char prev=jsh_exceptions;
	jsh_exceptions=js_toboolean(J, 1);
	
	js_pushboolean(J, prev);
}


int main(int argc, char **argv)
{
	int ret;
	
	// Checking command line
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <script.js/code> [args]\n", argv[0]);
		return -1;
	}
	
	// Creating JavaScript engine
	js_State *J=js_newstate(NULL, NULL, JS_STRICT);
	
	// Creating jsh object
	js_newobject(J);
	
	js_newstring(J, argv[0]);
	js_setproperty(J, -2, "shell");
	
	js_newstring(J, argv[1]);
	js_setproperty(J, -2, "script");
	
	js_newarray(J);
	for (int n=0; n<argc-2; n++)
	{
		js_pushstring(J, argv[2+n]);
		js_setindex(J, n, -2);
	}
	js_setproperty(J, -2, "args");
	
	js_newcfunction(J, jsB_exceptions, "exceptions", 1);
	js_setproperty(J, -2, "exceptions");
	
	js_setglobal(J, "jsh");
	
	// Creating library
	initLibrary(J);
	
	// Checking if script is a file
	FILE *f=fopen(argv[1], "r");
	if (f)
	{
		// It's a file
		fclose(f);
		ret=runFile(J, argv[1]);
	} else
	{
		// It's a script
		ret=runScript(J, argv[1]);
	}
	
	// Deleting JS engine
	js_freestate(J);
	
	return ret;
}
