#include "sys_wrap.hpp"
#include <system_error>
#include <cerrno>
#include <sys/wait.h>

namespace shepherd{
    pid_t Fork(){
        pid_t pid;
        if((pid = fork())<0) {
            int err = errno;
            throw std::system_error(err,std::system_category(),"fork failed");
        }
        return pid;
    }

    pid_t Waitpid(pid_t pid,int* status,int options){
        int st;
        if((st = waitpid(pid,status,options)) < 0){
            int err = errno;
            throw std::system_error(err, std::system_category(), "waitpid failed");
        }
        return st;
    }

    void Setpgid(pid_t pid,pid_t pgid){
        int st;
        if((st=setpgid(pid,pgid)) < 0){
            int err = errno;
            throw std::system_error(err, std::system_category(), "setpgid failed");
        }
        return;
    }

    void Execve(const std::string path, const std::vector<std::string> argv, char *const envp[])
    {
        int st;
        std::vector<char*> pargv;
        for(const auto& s:argv){
            pargv.push_back(const_cast<char*>(s.c_str()));
        }
        pargv.push_back(nullptr);
        if((st=execve(path.c_str(),pargv.data(),envp))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "execve failed");
        }
        return ;
    }

    void Kill(pid_t pid,int sig){
        int st;
        if((st=kill(pid,sig))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "send signal (kill) failed");
        }
        return ;
    }

    void Sigaction(int sig,const struct sigaction *act,struct sigaction *old){
        int st;
        if((st=sigaction(sig,act,old))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "sigaction error");
        }
        return ;
    }

    void Sigprocmask(int how,const sigset_t *set,sigset_t* old){
        int st;
        if((st = sigprocmask(how,set,old))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "sigprocmask error");
        }
        return ;
    }

    void Sigemptyset(sigset_t *set){
        int st;
        if((st=sigemptyset(set))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "sigemptyset error");
        }

        return ;
    }
    void Sigaddset(sigset_t *set,int sig){
        int st;
        if((st=sigaddset(set,sig))<0)
        {
            int err = errno;
            throw std::system_error(err, std::system_category(), "sigaddset error");
        }
        return ;
    }

    int Sigsuspend(const sigset_t *mask){
        int st;
        // sigsuspend 正常返回就是 -1/EINTR（被信号打断），不算错误
        if((st=sigsuspend(mask))<0 && errno!=EINTR){
            int err = errno;
            throw std::system_error(err, std::system_category(), "sigsuspend error");
        }
        return st;
    }

}