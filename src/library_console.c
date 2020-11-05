#include "library_console.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>



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
#define READ_MAX_SIZE	65536
#define READ_BLOCK		256
	int timeout=0;
	int str_size=READ_BLOCK, str_len=0;
	char *str=(char*)malloc(READ_BLOCK);
	
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
			str[str_len]=0;
			js_pushstring(J, str);
			free(str);
			return;
		} else
		if (rt > 0)
		{
			int n_read;
			char c;
			
			if (ioctl(0, FIONREAD, &n_read) != 0) goto error;
			if (n_read <= 0) goto eof;
			
			while (n_read--)
			{
				if (str_len == str_size-1)
				{
					str_size+=READ_BLOCK;
					if (str_size > READ_MAX_SIZE) goto error;
					char *new_str=(char*)realloc(str, str_size);
					if (! new_str) goto error;
					str=new_str;
				}
				
				if (read(0, &c, 1) != 1) goto eof;
				
				str[str_len++]=c;
				if (c == '\n') goto done;
			}
		}
	}
	
error:
	free(str);
	js_error(J, "read() failed");
	return;
	
eof:
	if (str_len == 0)
	{
		js_pushnull(J);
		free(str);
		return;
	}
	
done:
	str[str_len]=0;
	js_pushstring(J, str);
	free(str);
	return;
	
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
