#pragma once
#include<sys/types.h>
#include<signal.h>
#include<string>
#include<vector>
#include "core/process.hpp"

namespace shepherd{

    class Lshell{
        private:
        void eval(const std::string& cmdline);
        bool buildin_cmd(const std::vector<std::string>& argv);
        void do_bgfg(const std::vector<std::string>& argv);
        void waitfg(pid_t pid);
        void list_jobs();

        static void sigchld_handler(int sig);
        static void sigint_handler(int sig);
        static void sigtstp_handler(int sig);
        static void sigquit_handler(int sig);
        void install_handlers();

        // static handler 没有 this，用这个自指针回到实例
        static inline Lshell* s_self = nullptr;
        sigset_t oldset;

        Processes& jobs_;
        pid_t fg_pid = 0;
        bool emit_prompt = true;

        public:
        explicit Lshell(Processes& table):jobs_(table){ s_self = this; };
        void set_prompt(bool on){ emit_prompt = on; }
        int run();
    };
}
