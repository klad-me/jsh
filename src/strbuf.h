#ifndef STRBUF_H
#define STRBUF_H


typedef struct strbuf
{
	int size, len;
	char *str;
} strbuf_t;


strbuf_t* strbuf_new(void);
void strbuf_delete(strbuf_t *str);

int strbuf_ensureCapacity(strbuf_t *str, int len);

int strbuf_appendChar(strbuf_t *str, char c);
int strbuf_appendString(strbuf_t *str, const char *s);


#endif
