#include <stdio.h>
#include <mujs.h>

#include "library.h"


static int runScript(js_State *J, const char *fname)
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


int main(int argc, char **argv)
{
	// Checking command line
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <script.js> [args]\n", argv[0]);
		return -1;
	}
	
	// Creating JavaScript engine
	js_State *J=js_newstate(NULL, NULL, JS_STRICT);
	
	// Getting script file name
	const char *script_fname=argv[1];
	
	// Passing args
	js_newobject(J);
	js_newarray(J);
	for (int n=0; n<argc-2; n++)
	{
		js_pushstring(J, argv[2+n]);
		js_setindex(J, n, -2);
	}
	js_setproperty(J, -2, "argv");
	js_setglobal(J, "process");
	
	// Creating library
	initLibrary(J);
	
	// Starting script
	int ret=runScript(J, script_fname);
	
	// Deleting JS engine
	js_freestate(J);
	
	printf("Return value=%d\n", ret);
	return ret;
}
