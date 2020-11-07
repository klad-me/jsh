#include "utf8_assembly.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>


int utf8_assembly(int fd, unsigned char *buf, int buf_len, utf8_assembly_t *u)
{
	if (u->len > 0)
		memcpy(buf, u->incomplete, u->len);
	int rt=read(fd, buf + u->len, buf_len - u->len - 1);
	if (rt <= 0) return -1;
	
	// Processing UTF8
	unsigned char *utf=buf;
	rt+=u->len;
	u->len=0;
	int i;
	for (i=0; i<rt; i++)
	{
		unsigned char c=buf[i];
		
		if (! (c & 0x80))
		{
			// 1 byte
			(*utf++)=c;
		} else
		{
			// Multi-byte
			if (c & 0x40)
			{
				// First octet
				int l;
				if (! (c & 0x20))
				{
					l=2;
				} else
				if (! (c & 0x10))
				{
					l=3;
				} else
				if (! (c & 0x08))
				{
					l=4;
				} else
				{
					// Incorrect UTF-8 first octet
					(*utf++)='?';
					continue;
				}
				
				int left=rt-i;
				if (left < l)
				{
					// Have to wait for more data
					break;
				}
				
				l--;	// remove first octet from count
				
				// Checking secondary octets
				char ok=1, j;
				for (j=0; j<l-1; j++)
				{
					if ((buf[i+j] & 0xC0) != 0x80)
					{
						ok=0;
						break;
					}
				}
				
				if (ok)
				{
					// Copying full sybmol
					(*utf++)=c;
					for (j=0; j<l; j++)
					{
						(*utf++)=buf[i+1+j];
					}
					i+=l;
				} else
				{
					// Skipping first octet of incorrect symbol
					(*utf++)='?';
				}
			} else
			{
				// Unwanted octet
				(*utf++)='?';
			}
		}
	}
	
	// Saving incomplete data
	memcpy(u->incomplete, buf+i, rt-i);
	u->len=rt-i;
	
	// Return number of assembled bytes
	(*utf)=0;
	return utf-buf;
}
