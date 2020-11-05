#include "library_shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>

#include "jsh.h"


extern char **environ;


static void jsB_env(js_State *J)
{
	if (js_isstring(J, 1))
	{
		const char *find=js_tostring(J, 1);
		int find_len=strlen(find);
		
		int n=0;
		while (environ[n])
		{
			if ( (strncasecmp(environ[n], find, find_len) == 0) &&
				 (environ[n][find_len] == '=') )
			{
				js_pushstring(J, environ[n]+find_len+1);
				return;
			}
			
			n++;
		}
		
		js_pushundefined(J);
	} else
	{
		js_newobject(J);
		
		int n=0;
		while (environ[n])
		{
			char *value=strchr(environ[n], '=');
			if (! value)
			{
				n++;
				continue;
			}
			int name_len=value - environ[n];
			char name[name_len+1];
			strncpy(name, environ[n], name_len);
			name[name_len]=0;
			value++;
			
			js_pushstring(J, value);
			js_setproperty(J, -2, name);
			
			n++;
		}
	}
}


static void jsB_set(js_State *J)
{
	if ( (! js_isstring(J, 1)) ||
		 (! js_isstring(J, 2)) )
	{
		js_error(J, "bad set() arguments");
	}
	
	const char *var=js_tostring(J, 1);
	const char *value=js_tostring(J, 2);
	
	const char *ss;
	if ( (! isalpha(var[0])) && (var[0] != '_') )
		js_error(J, "bad environment variable name");
	for (ss=var+1; (*ss); ss++)
		if ( (! isalnum(*ss)) && ((*ss) != '_') )
			js_error(J, "bad environment variable name");
	
	setenv(var, value, 1);
	
	js_pushundefined(J);
}


static void jsB_unset(js_State *J)
{
	if (! js_isstring(J, 1))
	{
		js_error(J, "bad unset() arguments");
	}
	
	unsetenv(js_tostring(J, 1));
	
	js_pushundefined(J);
}


static void jsB_pwd(js_State *J)
{
	char *wd=getcwd(NULL, 0);
	if (! wd)
	{
		js_error(J, "pwd failed");
	}
	
	js_pushstring(J, wd);
	free(wd);
}


static void jsB_cd(js_State *J)
{
	if (! js_isstring(J, 1))
	{
		js_error(J, "bad cd() arguments");
	}
	
	if (chdir(js_tostring(J, 1)) != 0)
	{
		if (jsh_exceptions)
		{
			js_error(J, "%s", strerror(errno));
		} else
		{
			js_pushboolean(J, 0);
			return;
		}
	}
	
	js_pushboolean(J, 1);
}


static void jsB_mkdir(js_State *J)
{
	if (! js_isstring(J, 1))
	{
		js_error(J, "bad mkdir() arguments");
	}
	
	const char *dirname = js_tostring(J, 1);
	
#ifndef _WIN32
	if (mkdir(dirname, 0755) != 0)
#else
	if (mkdir(dirname) != 0)
#endif
	{
		if (jsh_exceptions)
		{
			js_error(J, "cannot mkdir '%s': %s", dirname, strerror(errno));
		} else
		{
			js_pushboolean(J, 0);
			return;
		}
	}
	
	js_pushboolean(J, 1);
}


static void jsB_rmdir(js_State *J)
{
	if (! js_isstring(J, 1))
	{
		js_error(J, "bad rmdir() arguments");
	}
	
	const char *dirname = js_tostring(J, 1);
	
	if (rmdir(dirname) != 0)
	{
		if (jsh_exceptions)
		{
			js_error(J, "cannot rmdir '%s': %s", dirname, strerror(errno));
		} else
		{
			js_pushboolean(J, 0);
			return;
		}
	}
	
	js_pushboolean(J, 1);
}


static char internal_stat(js_State *J, const char *fname)
{
	struct stat st;
	
	if (lstat(fname, &st) != 0) return 0;
	
	js_newobject(J);
	
	if (S_ISLNK(st.st_mode))
	{
		// Try to read link
		char link[st.st_size+1];
		if (readlink(fname, link, st.st_size) == st.st_size)
		{
			link[st.st_size]=0;
			js_pushstring(J, link);
			js_setproperty(J, -2, "symlink");
		}
		
		// Read real stat
		if (stat(fname, &st) != 0)
		{
			// Broken link ?
		}
	}
	
	if (S_ISREG(st.st_mode))
		js_pushstring(J, "file"); else
	if (S_ISDIR(st.st_mode))
		js_pushstring(J, "dir"); else
	if (S_ISCHR(st.st_mode))
		js_pushstring(J, "char"); else
	if (S_ISBLK(st.st_mode))
		js_pushstring(J, "block"); else
	if (S_ISFIFO(st.st_mode))
		js_pushstring(J, "fifo"); else
	if (S_ISSOCK(st.st_mode))
		js_pushstring(J, "socket"); else
		js_pushundefined(J);
	js_setproperty(J, -2, "type");
	
	js_pushnumber(J, st.st_mode & 07777);
	js_setproperty(J, -2, "mode");
	
	js_pushnumber(J, st.st_uid);
	js_setproperty(J, -2, "uid");
	
	js_pushnumber(J, st.st_gid);
	js_setproperty(J, -2, "gid");
	
	if ( (S_ISCHR(st.st_mode)) || (S_ISBLK(st.st_mode)) )
	{
		js_pushnumber(J, (st.st_rdev >> 8) & 0xff);
		js_setproperty(J, -2, "major");
		
		js_pushnumber(J, st.st_rdev & 0xff);
		js_setproperty(J, -2, "minor");
	}
	
	js_pushnumber(J, st.st_size);
	js_setproperty(J, -2, "size");
	
	return 1;
}


static void jsB_stat(js_State *J)
{
	if (! js_isstring(J, 1))
	{
		js_error(J, "bad stat() arguments");
	}
	
	if (! internal_stat(J, js_tostring(J, 1)))
	{
		if (jsh_exceptions)
		{
			js_error(J, "stat() failed");
		} else
		{
			js_pushnull(J);
		}
	}
}


static void jsB_ls(js_State *J)
{
	const char *dirname = ".";
	
	if (js_isstring(J, 1))
		dirname = js_tostring(J, 1);
	
	char showHidden=js_toboolean(J, 2);
	
	DIR *dir=opendir(dirname);
	if (! dir)
	{
		if (jsh_exceptions)
		{
			js_error(J, "cannot read dir '%s': %s", dirname, strerror(errno));
		} else
		{
			js_pushnull(J);
			return;
		}
	}
	
	js_newobject(J);
	
	struct dirent *de;
	while ( (de=readdir(dir)) )
	{
		if ( (!strcmp(de->d_name, ".")) || (!strcmp(de->d_name, "..")) ) continue;
		if ( (de->d_name[0] == '.') && (! showHidden) ) continue;
		
		char fname[strlen(dirname)+1+strlen(de->d_name)+1];
		strcpy(fname, dirname);
		strcat(fname, "/");
		strcat(fname, de->d_name);
		
		if (internal_stat(J, fname))
		{
			js_setproperty(J, -2, de->d_name);
		}
	}
}


void initLibrary_shell(js_State *J)
{
	// Global functions
	js_newcfunction(J, jsB_env, "env", 0);
	js_setglobal(J, "env");
	
	js_newcfunction(J, jsB_set, "set", 0);
	js_setglobal(J, "set");
	
	js_newcfunction(J, jsB_unset, "unset", 0);
	js_setglobal(J, "unset");
	
	js_newcfunction(J, jsB_pwd, "pwd", 0);
	js_setglobal(J, "pwd");
	
	js_newcfunction(J, jsB_cd, "cd", 1);
	js_setglobal(J, "cd");
	
	js_newcfunction(J, jsB_mkdir, "mkdir", 1);
	js_setglobal(J, "mkdir");
	
	js_newcfunction(J, jsB_rmdir, "rmdir", 1);
	js_setglobal(J, "rmdir");
	
	js_newcfunction(J, jsB_stat, "stat", 1);
	js_setglobal(J, "stat");
	
	js_newcfunction(J, jsB_ls, "ls", 0);
	js_setglobal(J, "ls");
}
