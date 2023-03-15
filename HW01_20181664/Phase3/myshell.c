/* $begin shellmain */
#include "myshell.h"
#include "csapp.h"
#include <errno.h>
void initjobs(struct job_t *jobs){

    jobs->pid =0;
    jobs->job_idx = 0;
    jobs->state = 0;
    jobs->r_state = -1;

    jobs->next = NULL;
}
int max_job_idx(struct job_t *jobs){

    return next_job_idx-1;
}
int addjob(struct job_t *jobs, pid_t pid, int state,int r_state, char *argv)
{
    struct job_t *new_node = malloc(sizeof(struct job_t));
    struct job_t *tmp = NULL;
    if(pid<1)
        return 0;
    new_node->pid = pid;
    new_node->state = state;
    new_node->r_state = r_state;
    if(state == BG)
        new_node->job_idx = next_job_idx++;
    else
        new_node->job_idx = -1;
    strcpy(new_node->cmdline, argv);
    tmp = jobs;
    if(tmp->next == NULL)
    {
        tmp->next = new_node;
    }
    else{
        while(1)
        {
            if(tmp->next == NULL)
                break;
            tmp = tmp->next;

        }
    tmp->next = new_node;
    }
    jobs_cnt++;
    return 1;

}
void modi_suspend(struct job_t *jobs, pid_t pid)
{
    struct job_t *tmp = jobs->next;
    while(tmp)
    {
        if(tmp->pid == pid)
        {
            tmp->state = BG;
            tmp->r_state = S;
            tmp->job_idx = next_job_idx++;
            break;
        }
        tmp= tmp->next;

    }
    return;
}
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;
    if(pid<1||jobs_cnt<=0)
        return 0;
    struct job_t *tmp1 = NULL, *tmp2 = NULL;
    tmp1 = jobs;
    tmp2 = tmp1->next;
    while(1)
    {

        if(tmp2 == NULL)
            {
                printf("No exist in job queue\n");
                break;
            }

        
        if(tmp2->pid == pid)
        {
            tmp1->next = tmp2->next;
            free(tmp2);
            break;
        }
        tmp1 = tmp2;
        tmp2 = tmp2->next;

    }
    jobs_cnt--;
    return pid;

}
void listjobs(struct job_t *jobs)
{

    struct job_t *tmp = jobs->next;
    while(tmp)
    {
        if(tmp->state == BG){
        printf("[%d] ", tmp->job_idx);
        if(tmp->r_state == R)
            printf("running ");
        else if(tmp->r_state == S)
            printf("suspended ");
        printf("%s\n",tmp->cmdline);
        }
        
        tmp = tmp->next;
        if(tmp == NULL)
            break;

    }


}


void sigchld_handler(int sig)
{
	pid_t pid;
	int o_errno = errno;
	int status;
	struct job_t *tmp = jobs;
	sigset_t mask_all, prev_all;

	Sigfillset(&mask_all);
	while((pid = waitpid(-1, &status, WUNTRACED|WNOHANG)) > 0)
	{
		if(WIFSIGNALED(status)||WIFEXITED(status))
		
		{
			Sigprocmask(SIG_BLOCK, &mask_all,&prev_all);
			deletejob(jobs,pid);
			Sigprocmask(SIG_SETMASK,&prev_all,NULL);
		//	Sio_puts("terminated!\n");
		}
		if(WIFSTOPPED(status))
		{
			tmp = tmp->next;
			while(tmp)
			{
				if(tmp->pid == pid)
				{
					modi_suspend(jobs,pid);
					break;
				}
				tmp = tmp->next;
			}
		}
	}
	errno = o_errno;
}
void sigint_handler(int sig)
{
	pid_t pid;
	pid = get_fg_pid(jobs);

	if(pid>0)
	{

		kill(pid,SIGINT);
	}
	return;

}
void sigtstp_handler(int sig)
{

	pid_t pid;
	pid = get_fg_pid(jobs);
	if(pid>0)
	{
		kill(pid,SIGTSTP);

	}
	return;
}
int get_fg_pid(struct job_t *jobs){

	int i;
	struct job_t *tmp = jobs->next;
	while(tmp)
	{
		if(tmp->state == FG){
			return tmp->pid;
		}
		tmp= tmp->next;

	}
	
	return -1;

}
int main() 
{
	
	jobs = (struct job_t *)malloc(sizeof(struct job_t));	
    char cmdline[MAXLINE]; /* Command line */


	Signal(SIGINT, sigint_handler);
	Signal(SIGTSTP,sigtstp_handler);
	Signal(SIGCHLD,sigchld_handler);

	
	Sigemptyset(&mask);
	Sigaddset(&mask,SIGINT);
	Sigaddset(&mask,SIGTSTP);
	Sigprocmask(SIG_BLOCK, &mask, &prev_mask);

    initjobs(jobs);
	while (1) {
	/* Read * */
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
	int state;
    pid_t pid;           /* Process id */
    sigset_t mask, mask_all, prev_mask;

	Sigfillset(&mask_all);
	Sigemptyset(&mask);
	Sigaddset(&mask, SIGCHLD);
	Sigaddset(&mask, SIGINT);
	Sigaddset(&mask, SIGTSTP);


    strcpy(buf, cmdline);
    pipe_parseline(buf,&command_cnt,command);/* pipe parsing*/
//	printf("command cnt is %d \n", command_cnt);
	if(command_cnt>1) /*if command_cnt>1, make pipe line */
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
		if (argv[0] == NULL)  
			return;   /* Ignore empty lines */
		if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run	
		Sigprocmask(SIG_BLOCK, &mask,&prev_mask);

        if((pid = Fork())==0) // child proceess
		{
			Signal(SIGTSTP, sigtstp_handler);
			if(!bg)
				Sigprocmask(SIG_UNBLOCK, &mask, NULL);
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
		Sigprocmask(SIG_SETMASK, &mask_all, NULL);
		if(!bg) addjob(jobs,pid,FG,R, command[i]);
		else addjob(jobs,pid,BG,R, command[i]);
		Sigprocmask(SIG_SETMASK, &prev_mask,NULL);

	/* Parent waits for foreground job to terminate */
			if (!bg){ 
			    int status;
				int w_pid;
				sigset_t mask;

				Sigfillset(&mask);
				Sigdelset(&mask,SIGCHLD);
				Sigdelset(&mask,SIGTSTP);

				if(command_cnt>1)
				{
					close(fd[i/2][1]);
				}
				Sigsuspend(&mask);
					
					
			}
			else if(bg)//when there is backgrount process!
			{
				state = BG;
				printf("%d %s", pid, cmdline);
			
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
	if (!strcmp(argv[0], "less")||!strcmp(argv[0], "echo")||!strcmp(argv[0], "ls")||!strcmp(argv[0],"mkdir")||!strcmp(argv[0],"rmdir")||!strcmp(argv[0],"touch")||!strcmp(argv[0],"ps")||!strcmp(argv[0],"cat")||!strcmp(argv[0], "pwd")||!(strcmp(argv[0],"grep"))||!(strcmp(argv[0],"hostname"))||!(strcmp(argv[0],"chmod"))||!(strcmp(argv[0],"date")))//command 
	{
		strcpy(bin_path,"/bin/");
		strcat(bin_path,argv[0]);  //binary path append
		argv[0] = bin_path;			//argv[0] reassign
		return 0;
	}
	else if(argv[0][0] =='.'&&argv[0][1] =='/')
		return 0;
	else if(!strcmp(argv[0],"jobs"))
	{
		listjobs(jobs);
		return 1;

	}
	else if(!strcmp(argv[0],"kill")||!strcmp(argv[0],"bg")||!strcmp(argv[0],"fg"))
	{
		char *idx_point = strchr(argv[1], '%')+1;
		//printf("idx is %d\n",*idx_point - 49);
		int job_idx = *idx_point -48;
		struct job_t *tmp = jobs;
		int pid,max_jdx = max_job_idx(jobs);
		int status;
		if(job_idx>max_jdx)
			printf("No Such Job\n");
		
		else {
			tmp = tmp->next;
			while(tmp)
			{
				if(tmp->job_idx == job_idx){
					pid = tmp->pid;
					break;
				}
				tmp = tmp->next;
			}
            if(!strcmp(argv[0],"kill")){
				Kill(pid,SIGINT);
				deletejob(jobs,pid);
            }
            if(!strcmp(argv[0],"fg")){
                Kill(pid,SIGCONT);
				tmp->state = FG;
				tmp->r_state = R;
				printf("[%d] running %s\n",tmp->job_idx,tmp->cmdline);
				Waitpid(pid, &status,0);
				return 1;
            }
            if(!strcmp(argv[0],"bg")){
                Kill(pid,SIGCONT);
				tmp->r_state = R;
				printf("[%d] running %s\n", tmp->job_idx,tmp->cmdline);
                return 1;
            }

		}
		return 1;

	}
		
	
	else if(!strcmp(argv[0], "sort"))
	{
			strcpy(bin_path,"/usr/bin/");
			strcat(bin_path, argv[0]);
			argv[0] = bin_path;
			return 0;
	}


	else if (!strcmp(argv[0], "cd")) //cd command
    {
		if(chdir(argv[1])!=0)
			perror("cd");
        return 1;
    }
    
	else if (!strcmp(argv[0], "exit")) /* exit command */
	{
		Sigprocmask(SIG_SETMASK, &prev_mask,NULL);
		exit(0);
	}  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
		return 1;
    return 1;                     /* Not a builtin command */
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


