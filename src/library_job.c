#include "library_job.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <mujs.h>

#include "jsh.h"


static void jsB_bg(js_State *J)
{
	if (! js_iscallable(J, 1))
		js_error(J, "bad bg() arguments");
	
	pid_t pid=fork();
	if (pid < 0)
		js_error(J, "fork() failed");
	
	if (pid == 0)
	{
		// Child process
		js_copy(J, 1);	// function to call
		js_pushnull(J);	// this value
		if (js_pcall(J, 0))
		{
			js_report(J, js_trystring(J, -1, "Error"));
			exit(-1);
		}
		exit(js_tonumber(J, -1));
	} else
	{
		// Calling process
		js_pushnumber(J, pid);
	}
}


static void jsB_wait(js_State *J)
{
	if (! js_isnumber(J, 1))
		js_error(J, "bad wait() arguments");
	pid_t pid=js_tonumber(J, 1);
	int status;
	if (waitpid(pid, &status, 0) != pid)
		js_pushundefined(J); else
		js_pushnumber(J, WEXITSTATUS(status));
}


static void jsB_checkpid(js_State *J)
{
	if (! js_isnumber(J, 1))
		js_error(J, "bad checkpid() arguments");
	pid_t pid=js_tonumber(J, 1);
	int status;
	int rt=waitpid(pid, &status, WNOHANG);
	if (rt == 0)
		js_pushnumber(J, -1); else
	if (rt == pid)
		js_pushnumber(J, WEXITSTATUS(status)); else
		js_pushundefined(J);
}


static void jsB_kill(js_State *J)
{
	if (! js_isnumber(J, 1))
		js_error(J, "bad kill() arguments");
	int sig=SIGTERM;
	if (js_isnumber(J, 2))
	{
		sig=js_tonumber(J, 2);
	} else
	if (js_isstring(J, 2))
	{
		const char *s=js_tostring(J, 2);
		struct
		{
			const char *name;
			int sig;
		} signames[] =
		{
			{ "SIGINT",		SIGINT },
			{ "SIGKILL",	SIGKILL },
			{ "SIGSTOP",	SIGSTOP },
			{ "SIGTERM",	SIGTERM },
			{ "SIGUSR1",	SIGUSR1 },
			{ "SIGUSR2",	SIGUSR2 },
		};
		sig=-1;
		for (int i=0; i<sizeof(signames)/sizeof(signames[0]); i++)
		{
			if (! strcasecmp(signames[i].name, s))
			{
				sig=signames[i].sig;
				break;
			}
		}
		if (sig < 0)
			js_error(J, "bad signal name for kill()");
	}
	
	js_pushboolean(J, kill(js_tonumber(J, 1), sig)==0);
}


static void jsB_sleep(js_State *J)
{
	if (! js_isnumber(J, 1))
		js_error(J, "bad sleep() arguments");
	float T=js_tonumber(J, 1);
	int sec=(int)T;
	int us=(T - sec) * 1000000;
	
	if (sec > 0) sleep(sec);
	if (us > 0) usleep(us);
	
	js_pushundefined(J);
}


void initLibrary_job(js_State *J)
{
	js_newcfunction(J, jsB_bg, "bg", 1);
	js_setglobal(J, "bg");
	
	js_newcfunction(J, jsB_wait, "wait", 1);
	js_setglobal(J, "wait");
	
	js_newcfunction(J, jsB_checkpid, "checkpid", 1);
	js_setglobal(J, "checkpid");
	
	js_newcfunction(J, jsB_kill, "kill", 1);
	js_setglobal(J, "kill");
	
	js_newcfunction(J, jsB_sleep, "sleep", 1);
	js_setglobal(J, "sleep");
}
