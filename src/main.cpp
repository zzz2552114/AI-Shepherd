#include<cstdio>
#include<string>
#include<unistd.h>
#include "core/process.hpp"
#include "legacy/Lshell.hpp"

int main(int argc,char** argv){
    bool legacy = false;
    bool prompt = true;
    for(int i=1;i<argc;i++){
        std::string a = argv[i];
        if(a=="--legacy") legacy = true;
        else if(a=="-p") prompt = false;
        else if(a=="-h" || a=="--help"){
            printf("Usage: shepherd-core [--legacy] [-p] [-h]\n");
            return 0;
        }
    }

    if(!legacy){
        printf("daemon 模式将在 Day5 启用\n");
        return 0;
    }

    // 和 tsh 一样把 stderr 并到 stdout，方便 driver 抓全部输出
    dup2(1,2);
    shepherd::Processes jobs;
    shepherd::Lshell sh(jobs);
    sh.set_prompt(prompt);
    return sh.run();
}
