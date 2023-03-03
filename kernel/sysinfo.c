#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "proc.h"
#include "sysinfo.h"

int systeminfo(uint64 addr)
{
    struct proc *p = myproc();
    struct sysinfo info;

    info.freemem = kfreemem();
    info.nproc = nproc();

    if (copyout(p->pagetable, addr, (char *)&info, sizeof(info)) < 0)
        return -1;
    return 0;
}
