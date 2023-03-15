/* $begin shellmain */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128
#define MAX_CMD_LINE 100
#define FG 1
#define BG 2
#define R 1
#define S 0

int next_job_idx = 1;
int jobs_cnt = 0;

struct job_t{

    pid_t pid;
    int job_idx;
    int state;
    int r_state;
    char cmdline[MAX_CMD_LINE];
    struct job_t *next;
};

sigset_t prev_mask, mask;
struct job_t *jobs = NULL;


/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
void echo_parseline(char **argv);
char bin_path[MAXLINE];
void pipe_parseline(char *buf, int *comm_cnt, char **command);
int get_fg_pid(struct job_t *);


/* signal handler */

void sigchld_handler(int sig);
void sigint_handler(int sig);
void sigtstp_handler(int sig);


/*job function*/
void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int max_job_idx(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state,int r_state, char *argv);
void modi_suspend(struct job_t *jobs, pid_t pid);
int deletejob(struct job_t *jobs, pid_t pid);
void listjobs(struct job_t *jobs);
pid_t fgpid(struct job_t *jobs);
int get_fg_pid(struct job_t *jobs);
