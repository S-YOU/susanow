
#pragma once


class Cmd_quit : public Command {
public:
    Cmd_quit(const char* str) : Command(str) {}
    void exec(shell* sh)
    {
        close(sh->fd);
        sh->fd = -1;
        sh->closed = true;
    }
};
class Cmd_halt : public Command {
public:
       Cmd_halt(const char* str) : Command(str) {}
       void exec(shell*)
       {
           exit(0);
       }
};
