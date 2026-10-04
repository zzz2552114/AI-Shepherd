#include "parseline.hpp"
#include<string>
#include<vector>

namespace shepherd{
    std::vector<std::string> parseline(const std::string& cmdline){
        std::vector<std::string> argv;
        int len = (int)cmdline.size();
        int i = 0;
        while(i < len){
            while(i < len && cmdline[i] == ' '){
                i++;
            }
            if(i >= len) break;
            std::string arg;
            while(i < len && cmdline[i] != ' '){
                if(cmdline[i] == '\'' || cmdline[i] == '"'){
                    // 引号内整段算一个参数，引号本身丢掉；没闭合就取到行尾
                    size_t pos = cmdline.find(cmdline[i],i+1);
                    if(pos == std::string::npos){
                        arg += cmdline.substr(i+1);
                        i = len;
                    }
                    else{
                        arg += cmdline.substr(i+1,pos-i-1);
                        i = (int)pos+1;
                    }
                }
                else{
                    arg += cmdline[i];
                    i++;
                }
            }
            argv.push_back(arg);
        }
        return argv;
    }
}
