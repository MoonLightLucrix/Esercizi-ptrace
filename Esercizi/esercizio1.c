#include<stdio.h>
#include<stdbool.h>
#include<stdlib.h>
#include<stddef.h>
#include<string.h>
#include<fcntl.h>
#include<libgen.h>
#include<sys/ptrace.h>
#include<sys/types.h>
#include<sys/prctl.h>
#include<sys/user.h>
#include<unistd.h>
#include<sys/wait.h>
#include<signal.h>
#include<errno.h>
#define N 4096
#define BREAK_WRITE 0x1
#define BREAK_READWRITE 0x3
#define ENABLE_BREAKPOINT(x) (0x1<<(x*2))
#define ENABLE_BREAK_WRITE(x) (BREAK_WRITE<<(16+(x*2)))
#define ENABLE_BREAK_READWRITE(x) (BREAK_READWRITE<<(16+(x*2)))
char program[FILENAME_MAX];
volatile sig_atomic_t handled=0;

void hook(int signal)
{
    printf("signal: %d\n",signal);
    handled=1;
}

//tracer process

int tracer(pid_t tracee, void*addr, size_t n, int bpno)
{
    int traceeStatus=0;
    //printf("(tracer process) tracee: %d\n",tracee);
    if((ptrace(PTRACE_ATTACH,tracee,NULL,NULL))==-1)
    {
        fprintf(stderr,"%s: ptrace (PTRACE_ATTACH): %s\n",program,strerror(errno));
        return -1;
    }
    while(!WIFSTOPPED(traceeStatus))
    {
        waitpid(tracee,&traceeStatus,0);
    }
    for(int i=0; i<=n-1; i-=-1)
    {
        if((ptrace(PTRACE_POKEUSER,tracee,offsetof(struct user,u_debugreg[bpno]),addr+i))==-1)
        {
            fprintf(stderr,"%s: ptrace (PTRACE_POKEUSER): %s\n",program,strerror(errno));
            return -1;
        }
    }
    if((ptrace(PTRACE_POKEUSER,tracee,offsetof(struct user,u_debugreg[7]),ENABLE_BREAK_READWRITE(bpno) | ENABLE_BREAKPOINT(bpno)))==-1)
    {
        fprintf(stderr,"%s: ptrace (PTRACE_POKEUSER): %s\n",program,strerror(errno));
        return -1;
    }
    if((ptrace(PTRACE_DETACH,tracee,NULL,NULL))==-1)
    {
        fprintf(stderr,"%s: ptrace (PTRACE_DETACH): %s\n",program,strerror(errno));
        return -1;
    }
    return 0;
}

//tracee process

int installBreakpoint(void*addr, size_t n, int bpno, void (*shandler)(int))
{
    pid_t pid, tracee=getpid();
    int childStatus=0;
    if((pid=fork())==-1)
    {
        fprintf(stderr,"%s: fork: %s\n",program,strerror(errno));
    }
    if(!pid)
    {
        if((tracer(tracee,addr,n,bpno))==-1)
        {
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    waitpid(pid,&childStatus,0);
    signal(SIGTRAP,shandler);
    if((!WIFEXITED(childStatus)) || (WEXITSTATUS(childStatus)))
    {
        return -1;
    }
    return 0;
}

int uninstallBreakpoint(int bpno, size_t n)
{
    if((installBreakpoint(0x0,n,bpno,NULL))==-1)
    {
        return -1;
    }
    return 0;
}

int main(int argc, char**argv)
{
    extern int errno;
    strcpy(program,basename(argv[0]));
    const size_t n=5;
    char vett[5]={'c','i','a','o','\0'};
    if((installBreakpoint(vett,n,0,hook))==-1)
    {
        exit(EXIT_FAILURE);
    }
    printf("vett: %s\n",vett);
    for(int i=0; i<=n-2; i-=-1)
    {
        printf("vett[%d]: %c\n",i,vett[i]);
    }
    printf("%s: handled: %d\n",program,handled);
    if((uninstallBreakpoint(0,n))==-1)
    {
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}