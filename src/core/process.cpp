#include "process.hpp"
#include<string>
#include<optional>
#include<vector>
#include<algorithm>

namespace shepherd{


    std::string state(ProcState s) noexcept{
        switch (s)
        {
        case ProcState::Undef:
            return "Undef";
        case ProcState::Running:
            return "Running";
        case ProcState::Stopped:
            return "Stopped";
        }
        return "Unknown";
    }

    void Processes::add(Process& p){
        std::lock_guard<std::mutex> lk(mu_);
        if(p.pid<1) return;
        if(upid_.count(p.pid)) return;
        p.jid = next_jid_++;
        upid_[p.pid] = p;
        ujid_[p.jid] = p.pid;
        return ;
    }

    void Processes::erase(pid_t pid){
        std::lock_guard<std::mutex> lk(mu_);
        if(upid_.find(pid)==upid_.end()) return;
        ujid_.erase(upid_[pid].jid);
        upid_.erase(pid);
        // 和 tsh 的 deletejob 一样，删完把下一个 jid 拉回 maxjid+1
        int max = 0;
        for(auto ele:upid_){
            if(ele.second.jid>max) max = ele.second.jid;
        }
        next_jid_ = max+1;
        return ;
    }

    void Processes::set_state(pid_t pid,ProcState st){
        std::lock_guard<std::mutex> lk(mu_);
        if(upid_.find(pid)!=upid_.end()) upid_[pid].state = st;
        return ;
    }

    bool Processes::contains(pid_t pid) const{
        std::lock_guard<std::mutex> lk(mu_);
        if(upid_.find(pid)!=upid_.end()){
            return 1;
        }
        return false;
    }

    std::optional<Process> Processes::get_by_pid(pid_t pid) const{
        std::lock_guard<std::mutex> lk(mu_);
        if(upid_.find(pid)!=upid_.end()){
            return upid_.at(pid);
        }
        return std::nullopt;
    }

    std::optional<Process> Processes::get_by_jid(int jid) const{
        std::lock_guard<std::mutex> lk(mu_);
        if(ujid_.find(jid)!=ujid_.end()){
            pid_t pid = ujid_.at(jid);
            if(upid_.find(pid)!=upid_.end()){
                return upid_.at(pid);
            }
        }
        return std::nullopt;
    }

    std::vector<Process> Processes::listjobs() const{
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<Process> vec;
        for(auto ele:upid_){
            vec.push_back(ele.second);
        }
        std::sort(vec.begin(),vec.end(),[](const Process& a,const Process& b){ return a.jid<b.jid; });
        return vec;
    }

}
