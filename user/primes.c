#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void mapping(int n, int fds[])
{
    close(n);

    dup(fds[n]);

    close(fds[0]);
    close(fds[1]);
}

void pipe_primes()
{
    int p, n;
    int fds[2];

    if (read(0, &p, sizeof(int)))
    {
        printf("prime %d\n", p);

        pipe(fds);

        if (fork() == 0)
        {
            mapping(1, fds);

            while (read(0, &n, sizeof(int)))
            {
                if (n % p != 0)
                    write(1, &n, sizeof(int));
            }
        }
        else
        {
            wait(0);

            mapping(0, fds);

            pipe_primes();
        }
    }
}

int main(void)
{
    int fds[2];

    pipe(fds);

    if (fork() == 0)
    {
        mapping(1, fds);

        for (int i = 2; i < 36; i++)
        {
            write(1, &i, sizeof(i));
        }
    }
    else
    {
        wait(0);

        mapping(0, fds);

        pipe_primes();
    }

    exit(0);
}
