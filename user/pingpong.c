#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void)
{
    int fds[2];
    int pid;
    char buf[5];

    pipe(fds);

    pid = fork();
    if (pid > 0)
    {
        write(fds[1], "ping", 4);

        wait(0);

        read(fds[0], buf, 4);
        buf[4] = '\0';
        printf("%d: received %s\n", getpid(), buf);
    }
    else
    {
        read(fds[0], buf, 4);
        buf[4] = '\0';
        printf("%d: received %s\n", getpid(), buf);

        write(fds[1], "pong", 4);
        exit(0);
    }

    close(fds[0]);
    close(fds[1]);

    exit(0);
}
