#ifndef UTF8_ASSEMBLY_H
#define UTF8_ASSEMBLY_H


typedef struct utf8_assembly
{
	unsigned char incomplete[3];
	int len;
} utf8_assembly_t;


int utf8_assembly(int fd, unsigned char *buf, int buf_len, utf8_assembly_t *u);


#endif
