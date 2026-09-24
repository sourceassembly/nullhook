/*
 * util.hpp
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <unistd.h>
#include <string>
#include <cstdio>
#include <array>
#include <fstream>
#include <sstream>

struct ProcStat
{
    int pid;
    std::string comm;
    char state;
    int ppid;
    int pgrp;
    int session;
    int tty_nr;
    int tpgid;
    unsigned long flags;
    unsigned long minflt;
    unsigned long cminflt;
    unsigned long majflt;
    unsigned long cmajflt;
    unsigned long utime;
    unsigned long stime;
    long cutime;
    long cstime;
    long priority;
    long nice;
    long num_threads;
    long itrealvalue;
    unsigned long starttime;
    unsigned long vsize;
    long rss;
    unsigned long rlim;
    unsigned long startcode;
    unsigned long endcode;
    unsigned long startstack;
    unsigned long kstkesp;
    unsigned long kstkeip;
    unsigned long signal;
    unsigned long blocked;
    unsigned long sigignore;
    unsigned long sigcatch;
    unsigned long wchan;
    unsigned long nswap;
    unsigned long cnswap;
    int exit_signal;
    int processor;
    unsigned long rt_priority;
    unsigned long policy;
    unsigned long long delayacct_blkio_ticks;
};

inline int ReadStat(pid_t pid, ProcStat *s)
{
    std::string procfile = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream proc(procfile);

    if (proc)
    {
        std::string line;
        std::getline(proc, line);
        std::stringstream ss(line);

        if (ss >> s->pid >> s->comm >> s->state >> s->ppid >> s->pgrp >> s->session >> s->tty_nr >> s->tpgid >> s->flags >> s->minflt >> s->cminflt >> s->majflt >> s->cmajflt >> s->utime >> s->stime >> s->cutime >> s->cstime >> s->priority >> s->nice >> s->num_threads >> s->itrealvalue >> s->starttime >> s->vsize >> s->rss >> s->rlim >> s->startcode >> s->endcode >> s->startstack >> s->kstkesp >> s->kstkeip >> s->signal >> s->blocked >> s->sigignore >> s->sigcatch >> s->wchan >> s->nswap >> s->cnswap >> s->exit_signal >> s->processor >> s->rt_priority >> s->policy >> s->delayacct_blkio_ticks)
            return 1;
    }
    return 0;
}