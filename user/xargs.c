#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

char *strtok(char *s, char *delim)
{
    char *start;
    static char *t;
    int i;

    if (s != 0)
        start = s;
    else
        start = t;

    if (strlen(start) < 1)
        return 0;

    for (i = 0; i < strlen(start); i++)
    {
        if (start[i] == (*delim) || start[i] == '\0' || start[i] == ' ' || start[i] == '\t')
        {
            start[i] = '\0';
            break;
        }
    }

    t = &start[i+1];

    return start;
}

int main(int argc, char **argv)
{
    char left_argv[MAXARG];
    char *xarg;
    char *nargv[MAXARG];
    int pid;

    for (int i = 1; i < argc; i++)
    {
        nargv[i-1] = malloc(strlen(argv[i]) + 1);
        strcpy(nargv[i-1], argv[i]);
    }

    read(0, left_argv, MAXARG);

    xarg = strtok(left_argv, "\n");
    while (xarg != 0)
    {
        pid = fork();
        if (pid == 0)
        {
            nargv[argc-1] = xarg;
            exec(argv[1], nargv);
            exit(0);
        }
        wait(0);

        xarg = strtok(0, "\n");
    }

    exit(0);
}
