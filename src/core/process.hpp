#pragma once
#include<string>
#include<vector>
#include<sys/types.h>
#include<unordered_map>
#include<mutex>
#include<optional>


namespace shepherd{
    enum class ProcState{
        Undef,
        Running,
        Stopped,
    };

    std::string state(ProcState s) noexcept;

    struct Process{
        pid_t pid = 0;
        pid_t pgid = 0;
        int jid = 0;
        std::vector<std::string> argv;
        std::string cmdline;
        ProcState state = ProcState::Undef;
    };

    class Processes{
    public:
        Processes() = default;
        void add(Process& p);

        void erase(pid_t pid);
        void set_state(pid_t pid, ProcState st);

        std::optional<Process> get_by_pid(pid_t pid) const;
        std::optional<Process> get_by_jid(int jid) const;
        
        bool contains(pid_t pid) const;

        std::vector<Process> listjobs() const;
    
    private:
        
        mutable std::mutex mu_;
        std::unordered_map<pid_t,Process> upid_;
        std::unordered_map<int,pid_t> ujid_;
        inline static int next_jid_ = 1;
    
    };
}
