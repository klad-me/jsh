#include "library_console.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "strbuf.h"


static void internal_print(js_State *J)
{
	int i, top = js_gettop(J);
	for (i = 1; i < top; ++i)
	{
		const char *s = js_tostring(J, i);
		fputs(s, stdout);
	}
	js_pushundefined(J);
}


static void jsB_print(js_State *J)
{
	internal_print(J);
	fflush(stdout);
}


static void jsB_echo(js_State *J)
{
	internal_print(J);
	fputs("\n", stdout);
	fflush(stdout);
}


static void jsB_read(js_State *J)
{
	int timeout=0;
	strbuf_t *str=strbuf_new();
	
	if (! str) js_error(J, "out of memory");
	
	if (js_isnumber(J, 1))
		timeout=(int)(js_tonumber(J, 1) * 1000.0);
	
	while (1)
	{
		fd_set fds;
		struct timeval tv;
		
		FD_ZERO(&fds);
		FD_SET(0, &fds);	// stdin
		
		if (timeout > 0)
		{
			tv.tv_sec=timeout/1000;
			tv.tv_usec=(timeout%1000)*1000;
		} else
		{
			tv.tv_sec=1000;
			tv.tv_usec=0;
		}
		
		int rt=select(1, &fds, NULL, NULL, &tv);
		if (rt < 0) goto error;
		if ( (rt == 0) && (timeout > 0) )
		{
			goto done;
		} else
		if (rt > 0)
		{
			int n_read;
			char c;
			
			if (ioctl(0, FIONREAD, &n_read) != 0) goto error;
			if (n_read <= 0) goto eof;
			
			while (n_read--)
			{
				if (read(0, &c, 1) != 1) goto eof;
				
				if (! strbuf_appendChar(str, c)) goto error;
				if (c == '\n') goto done;
			}
		}
	}
	
error:
	strbuf_delete(str);
	js_error(J, "read() failed");
	return;
	
eof:
	if (str->len == 0)
	{
		js_pushnull(J);
		strbuf_delete(str);
		return;
	}
	
done:
	js_pushstring(J, str->str);
	strbuf_delete(str);
}


void initLibrary_console(js_State *J)
{
	js_newcfunction(J, jsB_print, "print", 0);
	js_setglobal(J, "print");
	
	js_newcfunction(J, jsB_echo, "echo", 0);
	js_setglobal(J, "echo");
	
	js_newcfunction(J, jsB_read, "read", 0);
	js_setglobal(J, "read");
}
