#include "library_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "jsh.h"


static void jsB_read(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	FILE *f;
	char *s;
	int n, t;
	
	f = fopen(filename, "rb");
	if (!f)
	{
		if (jsh_exceptions)
			js_error(J, "cannot open file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fseek(f, 0, SEEK_END) < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	n = ftell(f);
	if (n < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot tell in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fseek(f, 0, SEEK_SET) < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	s = (char*)malloc(n + 1);
	if (!s)
	{
		fclose(f);
		js_error(J, "out of memory");
	}
	
	t = fread(s, 1, n, f);
	if (t != n)
	{
		free(s);
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot read data from file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	s[n] = 0;
	
	js_pushstring(J, s);
	free(s);
	fclose(f);
	return;
	
return_error:
	js_pushnull(J);
}


static void jsB_readBinary(js_State *J)
{
	const char *filename = js_tostring(J, 1);
	FILE *f;
	unsigned char *s;
	int n, t;
	
	f = fopen(filename, "rb");
	if (!f)
	{
		if (jsh_exceptions)
			js_error(J, "cannot open file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fseek(f, 0, SEEK_END) < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	n = ftell(f);
	if (n < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot tell in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fseek(f, 0, SEEK_SET) < 0)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot seek in file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	s = (unsigned char*)malloc(n);
	if (!s)
	{
		fclose(f);
		js_error(J, "out of memory");
	}
	
	t = fread(s, 1, n, f);
	if (t != n)
	{
		free(s);
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot read data from file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	js_newarray(J);
	for (int i=0; i<n; i++)
	{
		js_newnumber(J, s[i]);
		js_setindex(J, -2, i);
	}
	
	free(s);
	fclose(f);
	return;
	
return_error:
	js_pushnull(J);
}


static void jsB_write(js_State *J)
{
	if ( (! js_isstring(J, 1)) ||
		 (! js_isstring(J, 2)) )
	{
		js_error(J, "bad write() arguments");
	}
	
	const char *filename = js_tostring(J, 1);
	const char *s = js_tostring(J, 2);
	size_t n = strlen(s);
	
	FILE *f=fopen(filename, "wb");
	if (!f)
	{
		if (jsh_exceptions)
			js_error(J, "cannot create file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fwrite(s, 1, n, f) != n)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot write data to file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	fclose(f);
	js_pushboolean(J, 1);
	return;
	
return_error:
	js_pushboolean(J, 0);
}


static void jsB_writeBinary(js_State *J)
{
	if ( (! js_isstring(J, 1)) ||
		 (! js_isarray(J, 2)) )
	{
		js_error(J, "bad writeBinary() arguments");
	}
	
	const char *filename = js_tostring(J, 1);
	size_t n = js_getlength(J, 2);
	
	unsigned char *s = (unsigned char*)malloc(n);
	if (! s)
	{
		js_error(J, "out of memory");
	}
	
	for (size_t i=0; i<n; i++)
	{
		js_getindex(J, 2, i);
		s[i] = js_tonumber(J, -1);
		js_pop(J, 1);
	}
	
	FILE *f=fopen(filename, "wb");
	if (!f)
	{
		if (jsh_exceptions)
			js_error(J, "cannot create file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	if (fwrite(s, 1, n, f) != n)
	{
		fclose(f);
		if (jsh_exceptions)
			js_error(J, "cannot write data to file '%s': %s", filename, strerror(errno)); else
			goto return_error;
	}
	
	fclose(f);
	js_pushboolean(J, 1);
	return;
	
return_error:
	js_pushboolean(J, 0);
}


void initLibrary_file(js_State *J)
{
	// 'file' object
	js_newobject(J);
	
	js_newcfunction(J, jsB_read, "read", 1);
	js_setproperty(J, -2, "read");
	
	js_newcfunction(J, jsB_readBinary, "readBinary", 1);
	js_setproperty(J, -2, "readBinary");
	
	js_newcfunction(J, jsB_write, "write", 2);
	js_setproperty(J, -2, "write");
	
	js_newcfunction(J, jsB_writeBinary, "writeBinary", 2);
	js_setproperty(J, -2, "writeBinary");
	
	js_setglobal(J, "file");
}
