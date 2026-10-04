#include "parseline.hpp"
#include<string>
#include<vector>

namespace shepherd{
    std::vector<std::string> parseline(const std::string& cmdline){
        std::vector<std::string> argv;
        size_t len = cmdline.size();
        int i = 0;
        while(i < len){
            // 跳过空格
            while(i < len && cmdline[i] == ' '){
                i++;
            }
            if(i >= len) break;

            char c = cmdline[i];
            if(c == '\'' || c == '"'){
                ++i;
                // 处理引号
                size_t pos = cmdline.find(c,i);
                if(pos == std::string::npos){
                    argv.push_back(cmdline.substr(i));
                    i = len;
                }
                else{
                    argv.push_back(cmdline.substr(i,pos-i));
                    i = (int)pos+1;
                }
            }
            else{
                int pos = cmdline.find(' ',i);
                if(pos == std::string::npos) pos = len;
                argv.push_back(cmdline.substr(i,pos-i));
                i = (int)pos+1;
            }
        }

        return argv;
    }
}
