#include "library_printf.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "strbuf.h"


static int skip_atoi(const char **s)
{
	int v=0;
	while (isdigit(**s))
		v=v*10 + (*((*s)++) - '0');
	return v;
}


static strbuf_t* internal_sprintf(js_State *J)
{
#define ALTFORM		0x0001
#define ZERO		0x0002
#define LEFT		0x0004
#define SPACE		0x0008
#define SIGN		0x0010
#define STRING		0x0020
#define FLOAT		0x0040
#define EXP			0x0080
#define UPCASE		0x0100
	int n_args=js_gettop(J)-2;
	int n=0;
	int flags, field_width, precision;
	char specifier;
	
	if ( (n_args < 0) ||
		 (! js_isstring(J, 1)) )
		js_error(J, "bad sprintf() arguments");
	
	strbuf_t *str=strbuf_new();
	if (! str) js_error(J, "out of memory");
	
	const char *fmt;
	for (fmt=js_tostring(J,1); *fmt; fmt++)
	{
		// Copying chars up to '%' sign
		if ((*fmt) != '%')
		{
			if (! strbuf_appendChar(str, *fmt)) break;
			continue;
		}
		
		// Parse flags
		flags=0;
parse_flags:
		fmt++;	// skipping previous flag or starting '%'
		switch (*fmt)
		{
			case '#':	flags|=ALTFORM; goto parse_flags;
			case '0':	flags|=ZERO; goto parse_flags;
			case '-':	flags|=LEFT; goto parse_flags;
			case ' ':	flags|=SPACE; goto parse_flags;
			case '+':	flags|=SIGN; goto parse_flags;
		}
		
		// Parse field width
		field_width=0;
		if (isdigit(*fmt))
		{
			field_width=skip_atoi(&fmt);
		} else
		if ((*fmt)=='*')
		{
			fmt++;
			if (n < n_args)
			{
				field_width=js_tonumber(J, 2+(n++));
				if (field_width < 0)
				{
					flags|=LEFT;
					field_width=-field_width;
				}
			} else
			{
				field_width=0;
			}
		}
		
		// Parse precision
		precision=6;
		if ((*fmt)=='.')
		{
			fmt++;
			if (isdigit(*fmt))
			{
				precision=skip_atoi(&fmt);
			} else
			if ((*fmt)=='*')
			{
				fmt++;
				if (n < n_args)
				{
					precision=js_tonumber(J, 2+(n++));
					if (precision < 0) precision=0;
				} else
				{
					precision=0;
				}
			}
		}
		
		// Skipping modifiers
		while ( (*fmt) && (strchr("hlqLjzZt", *fmt)) )
			fmt++;
		
		// Parsing specifier
		specifier=*fmt;
		if (! specifier) break;
		switch (*fmt)
		{
			case 'c':
			case 's':
				// String
				flags|=STRING;
				break;
			
			case 'd':
			case 'i':
			case 'u':
				// Integer
				specifier='d';
			case 'o':
			case 'x':
			case 'X':
				break;
			
			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
				// Float
				flags|=FLOAT;
				break;
			
			case '%':
				// Just '%'
				strbuf_appendChar(str, '%');
				continue;
			
			default:
				// Incorrect specifier
				continue;
		}
		
		// Check argument count
		if (n >= n_args) continue;
		
		// Format
		if (flags & STRING)
		{
			// String
			const char *s=js_tostring(J, 2+n);
			int len=strlen(s);
			if (! (flags & LEFT))
			{
				while (len < field_width--)
					strbuf_appendChar(str, ' ');
			}
			strbuf_appendString(str, s);
			while (len < field_width--)
				strbuf_appendChar(str, ' ');
		} else
		if (flags & FLOAT)
		{
			// Float
			double v=js_trynumber(J, 2+n, 0);
			
			char f[16], *s=f;
			
			// Making format for libc's sprintf
			(*s++)='%';
			if (flags & ALTFORM) (*s++)='#';
			if (flags & ZERO) (*s++)='0';
			if (flags & LEFT) (*s++)='-';
			if (flags & SPACE) (*s++)=' ';
			if (flags & SIGN) (*s++)='+';
			(*s++)='*';	// pass field_width as argument
			(*s++)='.';
			(*s++)='*';	// pass precision as argument
			(*s++)=specifier;
			(*s)=0;
			
			int len=snprintf(NULL, 0, f, field_width, precision, v);
			if (! strbuf_ensureCapacity(str, len)) break;
			sprintf(str->str+str->len, f, field_width, precision, v);
			str->len+=len;
		} else
		{
			// Integer
			long long int v=js_trynumber(J, 2+n, 0);
			
			char f[16], *s=f;
			
			// Making format for libc's sprintf
			(*s++)='%';
			if (flags & ALTFORM) (*s++)='#';
			if (flags & ZERO) (*s++)='0';
			if (flags & LEFT) (*s++)='-';
			if (flags & SPACE) (*s++)=' ';
			if (flags & SIGN) (*s++)='+';
			(*s++)='*';	// pass field_width as argument
			(*s++)='l';	// long long
			(*s++)='l';
			(*s++)=specifier;
			(*s)=0;
			
			int len=snprintf(NULL, 0, f, field_width, v);
			if (! strbuf_ensureCapacity(str, len)) break;
			sprintf(str->str+str->len, f, field_width, v);
			str->len+=len;
		}
	}
	
	return str;
}


static void jsB_sprintf(js_State *J)
{
	strbuf_t *s=internal_sprintf(J);
	js_pushstring(J, s->str);
	strbuf_delete(s);
}


static void jsB_printf(js_State *J)
{
	strbuf_t *s=internal_sprintf(J);
	fputs(s->str, stdout);
	strbuf_delete(s);
	fflush(stdout);
	js_pushundefined(J);
}


void initLibrary_printf(js_State *J)
{
	js_newcfunction(J, jsB_sprintf, "sprintf", 1);
	js_setglobal(J, "sprintf");
	
	js_newcfunction(J, jsB_printf, "printf", 1);
	js_setglobal(J, "printf");
}
