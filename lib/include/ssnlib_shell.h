
/*-
 * MIT License
 *
 * Copyright (c) 2017 Susanoo G
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/**
 * @file ssnlib_shell.h
 * @brief include shell implementation
 * @author slankdev
 */


#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <memory>
#include <readline/readline.h>
#include <readline/history.h>

#include <ssnlib_sys.h>
#include <ssnlib_thread.h>
#include <ssnlib_cmd.h>


static inline char* Readline(const char* prompt)
{
    char* line = readline(prompt);
    static std::string prev_cmd = "";
    if (prev_cmd!=line && strlen(line)!=0) add_history(line);
    prev_cmd = line;
    return line;
}

namespace ssnlib {

class Shell : public ssnlib::ssn_thread {
    const std::string prompt;
    std::vector<std::unique_ptr<Command>> cmds;
public:
    Shell() : ssn_thread("shell"), prompt("SUSANOO$ ") {}
    Shell(const char* p) : ssn_thread("shell"), prompt(p) {}
    void help()
    {
        printf("Commands: \n");
        for (const auto& c : cmds) {
            printf("  %s \n", c->name.c_str());
        }
    }
    bool kill() { return false; }
    void add_cmd(ssnlib::Command* newcmd)
    {
        if (newcmd->name == "") throw slankdev::exception("Command name is empty");
        for (auto& cmd : cmds) {
            if (cmd->name == newcmd->name)
                throw slankdev::exception("command name is already registred");
        }
        cmds.push_back(std::unique_ptr<Command>(newcmd));
    }
    void exe_cmd(const char* cmd_str)
    {
        if (strlen(cmd_str) == 0) return;
        std::vector<std::string> args = slankdev::split(cmd_str, ' ');
        if (args[0] == "help") {
            help();
        } else {
            for (auto& cmd : cmds) {
                if (cmd->name == args[0]) {
                    (*cmd)(args);
                    return;
                }
            }
            printf("SUSH: command not found: %s\n", args[0].c_str());
        }
    }
    void operator()()
    {
        while (char* line = Readline(prompt.c_str())) {
            exe_cmd(line);
            free(line);
        }
        return;
    }
};



} /* namespace ssnlib */

//
//
// #include <stdio.h>
// #include <string.h>
// #include <stdlib.h>
//
// #include <string>
// #include <vector>
//
// #include <cmdline_parse.h>
// #include <cmdline_parse_num.h>
// #include <cmdline_parse_string.h>
// #include <cmdline_parse_etheraddr.h>
// #include <cmdline_socket.h>
// #include <cmdline.h>
//
// #include <slankdev/util.h>
//
//
// #define TOKEN_STRING TOKEN_STRING_INITIALIZER
// #define TOKEN_NUMBER TOKEN_NUM_INITIALIZER
// using token_str = cmdline_parse_token_string_t;
// using token_num = cmdline_parse_token_num_t;
//
//
//
// struct Command {
//     std::vector<void*> tokens;
//     cmdline_parse_inst_t* raw;
//
//     Command() : raw(nullptr) {}
//     virtual ~Command()
//     {
//         if (raw) free(raw);
//         while (tokens.size() > 0) {
//             void* tmp = tokens.back();
//             tokens.pop_back();
//             free(tmp);
//         }
//     }
//     virtual void handle(void*) = 0;
//     static void callback(void* param, struct cmdline*, void* data)
//     {
//         Command* cmd = reinterpret_cast<Command*>(data);
//         cmd->handle(param);
//     }
//     void init_raw()
//     {
//         size_t size = 3 * sizeof(void*);
//         size += sizeof(void*) * tokens.size();
//         raw = (cmdline_parse_inst_t*)malloc(size);
//     }
//     void set_raw(const char* hs)
//     {
//         raw->data = this;
//         raw->help_str = hs;
//         raw->f = Command::callback;
//         memcpy(&raw->tokens, tokens.data(), tokens.size()*sizeof(void*));
//     }
//     void append_token(token_str TT)
//     {
//         token_str* T = (token_str*)malloc(sizeof(token_str));
//         *T = TT;
//         tokens.push_back(T);
//     }
//     void append_token(token_num TT)
//     {
//         token_num* T = (token_num*)malloc(sizeof(token_num));
//         *T = TT;
//         tokens.push_back(T);
//     }
//     void token_fin() { tokens.push_back(nullptr); }
//
// };
//
//
// struct pcmd_get_params {
//     cmdline_fixed_string_t cmd;
// };
// class quit : public Command {
// public:
//     quit()
//     {
//         append_token(TOKEN_STRING(struct pcmd_get_params, cmd, "quit"));
//         token_fin();
//
//         init_raw();
//         set_raw("quit\n\tExit program");
//     }
//     void handle(void*) { exit(0); }
// };
//
//
// struct pcmd_open_params {
//     cmdline_fixed_string_t cmd;
//     uint16_t port;
// };
// class open : public Command {
// public:
//     open()
//     {
//         append_token(TOKEN_STRING(struct pcmd_open_params, cmd , "open"));
//         append_token(TOKEN_NUMBER(struct pcmd_open_params, port, UINT16));
//         token_fin();
//
//         init_raw();
//         set_raw("open\n\topen port");
//     }
//     void handle(void* p)
//     {
//         struct pcmd_open_params *params = reinterpret_cast<pcmd_open_params*>(p);
//         printf("open %u\n", params->port);
//     }
// };
//
//
// struct pcmd_ip_params {
// 	cmdline_fixed_string_t cmd;
// 	uint16_t port;
// 	cmdline_fixed_string_t opt;
// };
// class ip : public Command {
//         token_str pcmd_mtu_token_cmd    ;
//         token_num    pcmd_intstr_token_port;
//         token_str pcmd_intstr_token_opt ;
// public:
//     ip()
//     {
//         append_token(TOKEN_STRING(struct pcmd_ip_params, cmd , "ip"  ));
//         append_token(TOKEN_NUMBER(struct pcmd_ip_params, port, UINT16));
//         append_token(TOKEN_STRING(struct pcmd_ip_params, opt , NULL  ));
//         token_fin();
//
//         init_raw();
//         set_raw("ip <port_id> <mtu_value>\n     Change MTU");
//     }
//     void handle(void* p)
//     {
//         struct pcmd_ip_params* params = reinterpret_cast<pcmd_ip_params*>(p);
//         printf("%u %s \n", params->port, params->opt);
//     }
// };
//
//
// struct Shell {
//     struct ::cmdline* ctx_cmdline_;
//     std::string p;
//     std::vector<Command*> cmds;
//     std::vector<cmdline_parse_ctx_t> ctx;
//
// public:
//
//     Shell(const char* prompt) : p(prompt) {}
//     ~Shell()
//     {
//         cmdline_stdin_exit(ctx_cmdline_);
//         slankdev::vec_delete_all_ptr_elements<Command>(cmds);
//     }
//     void add_cmd(Command* cmd)
//     {
//         cmds.push_back(cmd);
//         ctx.push_back(cmd->raw);
//     }
//     void fin() { ctx.push_back(nullptr); }
//     void interact()
//     {
//         ctx_cmdline_ = cmdline_stdin_new(ctx.data(), p.c_str());
//         cmdline_interact(ctx_cmdline_);
//     }
// };
//
//
// int main()
// {
//     Shell shell("EthApp> ");
//     shell.add_cmd(new quit);
//     shell.add_cmd(new open);
//     shell.add_cmd(new ip  );
//     shell.fin();
//     shell.interact();
// }
//
//
//