#include "library_exec.h"

#include <mujs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <errno.h>

#include "strbuf.h"
#include "utf8_assembly.h"


static void jsB_exec(js_State *J)
{
	int in_arg=-1, out_arg=-1, err_arg=-1, exec_arg=-1;
	int i, top=js_gettop(J);
	char *exe=0;
	char* *args=0;
	pid_t pid;
	
	// Looking for arguments
	for (i=1; i<top; i++)
	{
		if (js_isarray(J, i))
		{
			if (exec_arg < 0)
				exec_arg=i; else
				js_error(J, "bad exec() arguments");
		} else
		if ( (js_iscallable(J, i)) ||
			 (js_isnull(J, i)) ||
			 (js_isstring(J, i)) )
		{
			if (in_arg < 0)
				in_arg=i; else
			if (out_arg < 0)
				out_arg=i; else
			if (err_arg < 0)
				err_arg=i; else
				js_error(J, "bad exec() arguments");
		} else
		{
			js_error(J, "bad exec() arguments");
		}
	}
	
	if (exec_arg < 0)
		js_error(J, "bad exec() arguments");
	
	if ( (in_arg > 0) && (js_isnull(J, in_arg)) )
		in_arg=-1;
	
	
	// Getting exec arguments
	int n_args=js_getlength(J, exec_arg);
	if (n_args < 1)
		js_error(J, "bad exec() arguments");
	
	args=(char**)malloc(sizeof(char*) * (n_args+1));
	for (i=0; i<n_args; i++)
	{
		js_getindex(J, exec_arg, i);
		args[i]=strdup(js_tostring(J, -1));
		js_pop(J, 1);
	}
	args[n_args]=0;
	
	
	// Creating pipes
	int fd_in[2], fd_out[2], fd_err[2];
	
	if (in_arg > 0) pipe(fd_in);
	if (out_arg > 0) pipe(fd_out);
	if (err_arg > 0) pipe(fd_err);
	
	
	// Creating subprocess
	pid=fork();
	if (pid < 0)
	{
		for (i=0; i<n_args; i++)
			free(args[i]);
		free(args);
		
		js_error(J, "fork: %s", strerror(errno));
	} else
	if (pid == 0)
	{
		// Child process
		
		// Reassign stdin, stdout and stderr
		if (in_arg > 0)
		{
			dup2(fd_in[0],  0);
			close(fd_in[1]);
		}
		
		if (out_arg > 0)
		{
			dup2(fd_out[1], 1);
			close(fd_out[0]);
		}
		
		if (err_arg > 0)
		{
			dup2(fd_err[1], 2);
			close(fd_err[0]);
		}
		
		// Starting process
		execvp(args[0], args);
		
		// Return mean exec() error
		perror(exe);
		exit(-1);
	} else
	{
		// Main process
		char *unsent_stdin=0;
		int unsent_stdin_len=0, unsent_stdin_pos=0;
		int stdin_paused=0;
		utf8_assembly_t utf8_stdout, utf8_stderr;
		strbuf_t *stdout_buf=0, *stderr_buf=0;
		
		// Closing unused FDs
		if (in_arg > 0) close(fd_in[0]);
		if (out_arg > 0) close(fd_out[1]);
		if (err_arg > 0) close(fd_err[1]);
		
		// Free unused args
		for (i=0; i<n_args; i++)
			free(args[i]);
		free(args);
		
		utf8_stdout.len=0;
		utf8_stderr.len=0;
		
		// Checking if stdin is a string
		if ( (in_arg > 0) && (js_isstring(J, in_arg)) )
		{
			unsent_stdin=strdup(js_tostring(J, in_arg));
			unsent_stdin_pos=0;
			unsent_stdin_len=strlen(unsent_stdin);
			in_arg=-1;
		}
		
		// Checking if stdout or stderr is string => collect output to string
		if ( (out_arg > 0) && (js_isstring(J, out_arg)) )
			stdout_buf=strbuf_new();
		
		if ( (err_arg > 0) && (js_isstring(J, err_arg)) )
			stderr_buf=strbuf_new();
		
		// Performing I/O
		while ( (in_arg > 0) || (unsent_stdin) || (out_arg > 0) || (err_arg > 0) )
		{
			fd_set fd_read, fd_write, fd_error;
			int fd_max=0, rt;
			struct timeval tv;
			
			
			// Configuring FDs to read/write
			FD_ZERO(&fd_read);
			FD_ZERO(&fd_write);
			FD_ZERO(&fd_error);
			if ( ( (in_arg > 0) || (unsent_stdin) ) &&
				 (! stdin_paused) )
			{
				FD_SET(fd_in[1], &fd_write);
				FD_SET(fd_in[1], &fd_error);
				if (fd_in[1] > fd_max) fd_max=fd_in[1];
			}
			if (out_arg > 0)
			{
				FD_SET(fd_out[0], &fd_read);
				FD_SET(fd_out[0], &fd_error);
				if (fd_out[0] > fd_max) fd_max=fd_out[0];
			}
			if (err_arg > 0)
			{
				FD_SET(fd_err[0], &fd_read);
				FD_SET(fd_err[0], &fd_error);
				if (fd_err[0] > fd_max) fd_max=fd_err[0];
			}
			tv.tv_sec=10;
			tv.tv_usec=0;
			
			// Waiting for events
			rt=select(fd_max+1, &fd_read, &fd_write, &fd_error, &tv);
			if (rt < 0) break;
			if (rt == 0) continue;
			
			// Processing stdin
			if ( ( (in_arg > 0) || (unsent_stdin) ) && (FD_ISSET(fd_in[1], &fd_error)) )
			{
				close(fd_in[1]);
				in_arg=-1;
				if (unsent_stdin)
				{
					free(unsent_stdin);
					unsent_stdin=0;
				}
			} else
			if ( ( (in_arg > 0) || (unsent_stdin) ) && (FD_ISSET(fd_in[1], &fd_write)) )
			{
				if ( (! unsent_stdin) && (in_arg > 0) )
				{
					if (unsent_stdin) free(unsent_stdin);
					unsent_stdin=0;
					
					js_copy(J, in_arg);	// function
					js_pushnull(J);		// this
					if (js_pcall(J, 0))
					{
						fprintf(stderr, "stdin callback: %s\n", js_trystring(J, -1, "Error"));	// don't throw exception
						close(fd_in[1]);
						in_arg=-1;
					} else
					{
						if (js_isstring(J, -1))
						{
							const char *s=js_tostring(J, -1);
							if (! *s)
							{
								stdin_paused=1;
							} else
							{
								stdin_paused=0;
								unsent_stdin=strdup(s);
								unsent_stdin_pos=0;
								unsent_stdin_len=strlen(unsent_stdin);
							}
						} else
						{
							// EOF
							in_arg=-1;
							close(fd_in[1]);
						}
					}
					js_pop(J, 1);
				}
				
				if (unsent_stdin)
				{
					rt=write(fd_in[1], unsent_stdin, unsent_stdin_len-unsent_stdin_pos);
					if (rt > 0)
					{
						unsent_stdin_pos+=rt;
						if (unsent_stdin_pos >= unsent_stdin_len)
						{
							free(unsent_stdin);
							unsent_stdin=0;
						}
					} else
					{
						close(fd_in[1]);
						in_arg=-1;
						if (unsent_stdin)
						{
							free(unsent_stdin);
							unsent_stdin=0;
						}
					}
				}
			}
			
			
			// Processing stdout
			if ( (out_arg > 0) && (FD_ISSET(fd_out[0], &fd_error)) )
			{
				close(fd_out[0]);
				out_arg=-1;
			} else
			if ( (out_arg > 0) && (FD_ISSET(fd_out[0], &fd_read)) )
			{
				unsigned char tmp[1024];
				rt=utf8_assembly(fd_out[0], tmp, sizeof(tmp), &utf8_stdout);
				if (rt > 0)
				{
					if (js_iscallable(J, out_arg))
					{
						js_copy(J, out_arg);	// function
						js_pushnull(J);			// this
						js_pushstring(J, (char*)tmp);
						if (js_pcall(J, 1))
						{
							fprintf(stderr, "stdout callback: %s\n", js_trystring(J, -1, "Error"));	// don't throw exception
							close(fd_out[0]);
							out_arg=-1;
						}
						js_pop(J, 1);
					} else
					if (js_isstring(J, out_arg))
					{
						if ( (! stdout_buf) || (! strbuf_appendString(stdout_buf, (char*)tmp)) )
						{
							close(fd_out[0]);
							out_arg=-1;
						}
					}
					
					// Unpause stdin after output handling
					stdin_paused=0;
				} else
				if (rt < 0)
				{
					close(fd_out[0]);
					out_arg=-1;
				}
			}
			
			
			// Processing stderr
			if ( (err_arg > 0) && (FD_ISSET(fd_err[0], &fd_error)) )
			{
				close(fd_err[0]);
				err_arg=-1;
			} else
			if ( (err_arg > 0) && (FD_ISSET(fd_err[0], &fd_read)) )
			{
				unsigned char tmp[1024];
				rt=utf8_assembly(fd_err[0], tmp, sizeof(tmp), &utf8_stderr);
				if (rt > 0)
				{
					if (js_iscallable(J, err_arg))
					{
						js_copy(J, err_arg);	// function
						js_pushnull(J);			// this
						js_pushstring(J, (char*)tmp);
						if (js_pcall(J, 1))
						{
							fprintf(stderr, "stderr callback: %s\n", js_trystring(J, -1, "Error"));	// don't throw exception
							close(fd_err[0]);
							err_arg=-1;
						}
						js_pop(J, 1);
					} else
					if (js_isstring(J, err_arg))
					{
						if ( (! stderr_buf) || (! strbuf_appendString(stderr_buf, (char*)tmp)) )
						{
							close(fd_err[0]);
							err_arg=-1;
						}
					}
					
					// Unpause stdin after output handling
					stdin_paused=0;
				} else
				if (rt < 0)
				{
					close(fd_err[0]);
					err_arg=-1;
				}
			}
		}
		
		// Waiting for child process to exit
		int status;
		waitpid(pid, &status, 0);
		
		// Creating result
		js_newobject(J);
		js_pushnumber(J, WEXITSTATUS(status));
		js_setproperty(J, -2, "status");
		if (stdout_buf)
		{
			js_pushstring(J, stdout_buf->str);
			js_setproperty(J, -2, "stdout");
			strbuf_delete(stdout_buf);
		}
		if (stderr_buf)
		{
			js_pushstring(J, stderr_buf->str);
			js_setproperty(J, -2, "stderr");
			strbuf_delete(stderr_buf);
		}
	}
}


void initLibrary_exec(js_State *J)
{
	js_newcfunction(J, jsB_exec, "exec", 1);
	js_setglobal(J, "exec");
}
