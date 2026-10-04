#pragma once
#include<sys/types.h>
#include<signal.h>
#include<unistd.h>
#include<string>
#include<vector>

namespace shepherd{
    // 封装一些系统底层函数，加上一些错误判断和防御
    pid_t Fork();
    pid_t Waitpid(pid_t pid, int *status, int options);

    void Setpgid(pid_t pid, pid_t pgid);
    void Execve(const std::string path, const std::vector<std::string> argv,char *const envp[]);
    void Kill(pid_t pid, int sig);

    void Sigaction(int sig, const struct sigaction *act, struct sigaction *old);
    void Sigprocmask(int how, const sigset_t *set, sigset_t *old);
    void Sigemptyset(sigset_t *set);
    void Sigaddset(sigset_t *set, int sig);
    int  Sigsuspend(const sigset_t *mask);
}
