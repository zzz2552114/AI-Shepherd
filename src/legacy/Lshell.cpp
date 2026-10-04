#include "Lshell.hpp"
#include "parseline.hpp"
#include "util/sys_wrap.hpp"
#include "core/process.hpp"
#include<cstdio>
#include<cstdlib>
#include<cerrno>
#include<optional>
#include<string>
#include<vector>
#include<iostream>
#include<unistd.h>
#include<sys/wait.h>

extern char** environ;

namespace shepherd{

    void Lshell::eval(const std::string& cmdline){
        sigset_t mask;
        std::vector<std::string> argv = parseline(cmdline);
        if(argv.empty()) return;
        int bg = 0;
        if(argv.back()=="&"){
            bg = 1;
            argv.pop_back();
        }
        if(argv.empty()) return;
        if(Lshell::buildin_cmd(argv)) return;
        pid_t cpid;

        Sigemptyset(&mask);
        Sigaddset(&mask,SIGCHLD);
        Sigprocmask(SIG_BLOCK,&mask,&oldset);
        
        if((cpid=Fork())==0){
            Sigprocmask(SIG_SETMASK,&oldset,nullptr);
            Setpgid(0,0);
            try{
                Execve(argv[0],argv,environ);
            }
            catch(...){
                printf("%s: Command not found\n",argv[0].c_str());
                exit(1);
            }
        }
        else{
            Process job;
            job.pid = cpid;
            job.cmdline = cmdline;
            job.argv = argv;
            job.state = ProcState::Running;
            jobs_.add(job);
            if(!bg){
                fg_pid = cpid;
                waitfg(cpid);
                Sigprocmask(SIG_SETMASK,&oldset,nullptr);
            }
            else{
                Sigprocmask(SIG_SETMASK,&oldset,nullptr);
                printf("[%d] (%d) %s\n",job.jid,job.pid,cmdline.c_str());
            }
        }
        return ;
    }

    bool Lshell::buildin_cmd(const std::vector<std::string>& argv){
        if(argv[0]=="quit"){
            exit(0);
        }
        else if(argv[0]=="jobs"){
            list_jobs();
            return true;
        }
        else if(argv[0]=="bg" || argv[0]=="fg"){
            do_bgfg(argv);
            return true;
        }
        return false;
    }

    void Lshell::do_bgfg(const std::vector<std::string>& argv){
        pid_t cpid;
        std::optional<Process> cjob;

        if(argv.size()<2){
            printf("%s command requires PID or %%jobid argument\n",argv[0].c_str());
            return;
        }
        if(argv[1][0]=='%'){
            int jid = atoi(argv[1].c_str()+1);
            cjob = jobs_.get_by_jid(jid);
            if(!cjob){
                printf("%s: No such job\n",argv[1].c_str());
                return;
            }
            cpid = cjob->pid;
        }
        else if(argv[1][0]>='1' && argv[1][0]<='9'){
            cpid = atoi(argv[1].c_str());
            cjob = jobs_.get_by_pid(cpid);
            if(!cjob){
                printf("(%d): No such process\n",cpid);
                return;
            }
        }
        else{
            printf("%s: argument must be a PID or %%jobid\n",argv[0].c_str());
            return;
        }

        if(argv[0]=="fg"){
            jobs_.set_state(cpid,ProcState::Running);
            fg_pid = cpid;
            Kill(-cpid,SIGCONT);
            waitfg(cpid);
        }
        else{
            jobs_.set_state(cpid,ProcState::Running);
            Kill(-cpid,SIGCONT);
            printf("[%d] (%d) %s\n",cjob->jid,cjob->pid,cjob->cmdline.c_str());
        }
        return ;
    }

    void Lshell::waitfg(pid_t pid){
        while(fg_pid==pid){
            Sigsuspend(&oldset);
        }
        return ;
    }

    void Lshell::list_jobs(){
        for(auto job:jobs_.listjobs()){
            printf("[%d] (%d) %s %s\n",job.jid,job.pid,state(job.state).c_str(),job.cmdline.c_str());
        }
        return ;
    }

void Lshell::sigchld_handler(int sig){
        int olderrno = errno;
        pid_t pid;
        int st;
        while((pid=waitpid(-1,&st,WNOHANG|WUNTRACED))>0){
            std::optional<Process> cjob = s_self->jobs_.get_by_pid(pid);
            if(!cjob) continue;
            if(WIFSTOPPED(st)){
                printf("Job [%d] (%d) stopped by signal %d\n",cjob->jid,pid,WSTOPSIG(st));
                s_self->jobs_.set_state(pid,ProcState::Stopped);
            }
            else{
                if(WIFSIGNALED(st))
                    printf("Job [%d] (%d) terminated by signal %d\n",cjob->jid,pid,WTERMSIG(st));
                s_self->jobs_.erase(pid);
            }
            if(s_self->fg_pid==pid) s_self->fg_pid = 0;
        }
        errno = olderrno;
        return ;
    }

    void Lshell::sigint_handler(int sig){
        int olderrno = errno;
        if(s_self->fg_pid!=0)
            kill(-s_self->fg_pid,SIGINT);
        errno = olderrno;
        return ;
    }

    void Lshell::sigtstp_handler(int sig){
        int olderrno = errno;
        if(s_self->fg_pid!=0)
            kill(-s_self->fg_pid,SIGTSTP);
        errno = olderrno;
        return ;
    }

    void Lshell::sigquit_handler(int sig){
        printf("Terminating after receipt of SIGQUIT signal\n");
        exit(1);
    }

    void Lshell::install_handlers(){
        struct sigaction act;
        act.sa_handler = sigchld_handler;
        Sigemptyset(&act.sa_mask);
        act.sa_flags = SA_RESTART;
        Sigaction(SIGCHLD,&act,nullptr);

        act.sa_handler = sigint_handler;
        Sigaction(SIGINT,&act,nullptr);

        act.sa_handler = sigtstp_handler;
        Sigaction(SIGTSTP,&act,nullptr);

        act.sa_handler = sigquit_handler;
        Sigaction(SIGQUIT,&act,nullptr);
        return ;
    }

    int Lshell::run(){
        install_handlers();
        std::string cmdline;
        while(true){
            if(emit_prompt){
                printf("tsh> ");
                fflush(stdout);
            }
            if(!std::getline(std::cin,cmdline)){
                fflush(stdout);
                return 0;
            }
            eval(cmdline);
            fflush(stdout);
        }
    }

}
