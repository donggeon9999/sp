/* $begin shellmain */
#include "myshell.h"
#include "csapp.h"
#include <errno.h>

void sigchld_handler(int sig)
{
	pid_t pid;
	int o_errno = errno;
	while((pid = wait(NULL)) > 0)
	{
		printf("IS sigchld_handler!!\n");
	}
}
int main() 
{
    char cmdline[MAXLINE]; /* Command line */
	Sigemptyset(&mask);
	Sigaddset(&mask,SIGINT);
	Sigaddset(&mask,SIGTSTP);
	Sigprocmask(SIG_BLOCK, &mask, &prev_mask);
    while (1) {
	/* Read */
	printf("CSE4100-SP-P#1> ");                   
	Fgets(cmdline, MAXLINE, stdin);
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */

void pipe_parseline(char *buf, int *comm_cnt, char **command)
{
	char *point = NULL;
	char cmd_idx =0;
	if(strchr(buf,'|') == NULL)//if single command line
	{
		*comm_cnt = 1;
		command[cmd_idx] = buf;
		return;
	}
	while(1)
	{
		command[cmd_idx] = (char *)malloc(sizeof(char)*MAXLINE);
		point = strchr(buf,'|');
		if(point ==NULL)
			break;
		*point = '\0';
		strcpy(command[cmd_idx], buf);
		command[cmd_idx][strlen(buf)] = '\n';
		command[cmd_idx][strlen(buf)+1] = '\0';
		cmd_idx++;
		buf = point+ 1;
	}
	command[cmd_idx] = buf;
	*comm_cnt = cmd_idx+1;
	return;
	
}
/* $begin eval */
/* eval - Evaluate a command line */

void eval(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
	char *command[10];
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;/* Should the job run in bg or fg? */
	int command_cnt;
	int **fd= NULL;
    pid_t pid;           /* Process id */
    
    strcpy(buf, cmdline);
    pipe_parseline(buf,&command_cnt,command);/* pipe parsing*/
//	printf("command cnt is %d \n", command_cnt);
	if(command_cnt>1)
	{
		fd = (int **)malloc(sizeof(int *)*(command_cnt-1));
		for(int i = 0;i<command_cnt-1;i++){
			fd[i] = (int *)malloc(sizeof(int)*2);
			if(pipe(fd[i])<0)
				printf("pipe error\n");
		}
	}
	for(int i = 0 ;i<command_cnt ;i++)
	{
	//	printf("%s\n", command[i]);

	
		bg = parseline(command[i], argv);
		//bg = parseline(buf, argv); 
		if (argv[0] == NULL)  
			return;   /* Ignore empty lines */
		if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run	
        if((pid = Fork())==0) // child proceess
		{
			if(command_cnt > 1)//if multi command lines
			{	
				if(i==0){
					dup2(fd[i/2][1], 1);
					close(fd[i/2][0]);
					//close(fd[1]);
				}
				else if(i == command_cnt-1)
				{
					dup2(fd[i/2][0],0);
				//	close(fd[i-1][0]);
					close(fd[i/2][1]);
				}
				else
				{
					dup2(fd[i/2+1][1],1);
					dup2(fd[i/2][0],0);
					close(fd[i/2][1]);
					close(fd[i/2+1][0]);
				}

			}
			if (execve(argv[0], argv, environ) < 0) {
				printf("%s: Command not found.\n", argv[0]);
				exit(0);
			}
		}
		
	/* Parent waits for foreground job to terminate */
			if (!bg){ 
			    int status;
				int w_pid;
			//	printf("pid is %d\n",pid);
				if(command_cnt>1)
				{
					close(fd[i/2][1]);
				}
				w_pid = Waitpid(pid,&status,0);
					if(w_pid<0)
					unix_error("waitfg: waitpid error\n");
				//Signal(SIGCHLD, sigchld_handler);	
				//printf("w_pid is %d\n",w_pid);
			}
			else//when there is backgrount process!
			{
				printf("%d %s", pid, cmdline);
				Signal(SIGCHLD,sigchld_handler);
			}
		
		}
	}
//	printf("ENd of command\n");
    return;
}
void echo_parseline(char **argv) /* echo command argv[1] parseline*/

{
	int flag = 0;
	char *pointer = *argv;
	char *quotes_pointer = NULL;
	while(1) /*parse blank, quotes, double quotation marks */
	{	
		if(*pointer == ' ')
			pointer++;
		else if(*pointer == '"')
		{
			pointer++;
			quotes_pointer = strchr(pointer,'"');
			*quotes_pointer = '\0';
		}
		else if(*pointer == '\'') 
		{
			pointer++;
			quotes_pointer = strchr(pointer,'\'');
			*quotes_pointer = '\0';
		}
		else break;
	}
	//printf("pointer is %s\n\n",pointer);
	*argv = pointer;
	printf("argv[1] is %s\n",pointer);
}
/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
	if (!strcmp(argv[0], "less")||!strcmp(argv[0], "less")||!strcmp(argv[0], "ls")||!strcmp(argv[0],"mkdir")||!strcmp(argv[0],"rmdir")||!strcmp(argv[0],"touch")||!strcmp(argv[0],"ps")||!strcmp(argv[0],"cat")||!strcmp(argv[0], "pwd")||!(strcmp(argv[0],"grep"))||!(strcmp(argv[0],"hostname"))||!(strcmp(argv[0],"chmod"))||!(strcmp(argv[0],"date")))//command 
	{
		strcpy(bin_path,"/bin/");
		strcat(bin_path,argv[0]);  //binary path append
		argv[0] = bin_path;			//argv[0] reassign
	}
	if(!strcmp(argv[0],"echo"))
	{
		strcpy(bin_path,"/bin/");
		//echo_parseline(&argv[1]);	
		strcat(bin_path,argv[0]);
		argv[0]=bin_path;
		
	}
	if(!strcmp(argv[0], "sort"))
	{
			strcpy(bin_path,"/usr/bin/");
			strcat(bin_path, argv[0]);
			argv[0] = bin_path;

	}

	if (!strcmp(argv[0], "cd")) //cd command
    {
		if(chdir(argv[1])!=0)
			perror("cd");
        return 1;
    }
    
    if (!strcmp(argv[0], "exit")) /* exit command */
	{
		Sigprocmask(SIG_SETMASK, &prev_mask,NULL);
		exit(0);
	}  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;    /* Background job? */
	int len = strlen(buf);
	char *start, *end;
    buf[len-1] = ' ';  /* Replace trailing '\n' with space */

    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
		buf++;

    /* Build the argv list */
    argc = 0;
	if(strchr(buf,'\'')==NULL&&strchr(buf,'\"')==NULL)
	{
    while ((delim = strchr(buf, ' '))) {
		
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
	}
	else
	{

		if(strchr(buf,'\''))
		{
			start = strchr(buf,'\'');
			end = strchr(start+1,'\'');
		}
		else if(strchr(buf,'\"')){
			start = strchr(buf,'\"');
			end = strchr(start+1,'\"');
		}
        while ((delim = strchr(buf, ' '))) {
            
            argv[argc++] = buf;
            *delim = '\0';
            buf = delim + 1;
            if(*buf == '\'' || *buf == '\"')
            {
                *end = '\0';
                argv[argc++] = start+1;
                
			//	printf("ARGV is %s\n", argv[argc-1]);
                buf = end+1;
            }
            while (*buf && (*buf == ' ')) /* Ignore spaces */
                buf++;
        }
	

	}
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
		return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
		argv[--argc] = NULL;

    return bg;
}
/* $end parseline */


