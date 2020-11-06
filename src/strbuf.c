#include "strbuf.h"

#include <stdlib.h>
#include <string.h>


strbuf_t* strbuf_new(void)
{
	strbuf_t *str=(strbuf_t*)malloc(sizeof(strbuf_t));
	if (str)
	{
		str->size=256;
		str->len=0;
		str->str=(char*)malloc(str->size);
		if (! str->str)
		{
			free(str);
			return 0;
		}
		str->str[0]=0;
	}
	return str;
}


void strbuf_delete(strbuf_t *str)
{
	if (str)
	{
		if (str->str) free(str->str);
		free(str);
	}
}


int strbuf_ensureCapacity(strbuf_t *str, int len)
{
	if (! str) return 0;
	
	if (str->len + len + 1 > str->size)
	{
		int new_size=str->len + len + 256;
		char *new_str=(char*)realloc(str->str, new_size);
		if (! new_str) return 0;
		str->size=new_size;
		str->str=new_str;
	}
	
	return 1;
}


int strbuf_appendChar(strbuf_t *str, char c)
{
	if (! strbuf_ensureCapacity(str, 1)) return 0;
	
	str->str[str->len++]=c;
	str->str[str->len]=0;
	
	return 1;
}


int strbuf_appendString(strbuf_t *str, const char *s)
{
	int l=strlen(s);
	
	if (! strbuf_ensureCapacity(str, l)) return 0;
	
	memcpy(str->str + str->len, s, l);
	str->len+=l;
	str->str[str->len]=0;
	
	return 1;
}
