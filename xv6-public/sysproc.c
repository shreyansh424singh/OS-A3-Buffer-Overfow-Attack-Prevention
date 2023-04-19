#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int sys_fork(void)
{
  return fork();
}

int sys_exit(void)
{
  exit();
  return 0; // not reached
}

int sys_wait(void)
{
  return wait();
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// LCG parameters
#define A 1664525
#define C 1013904223
#define M 4297 // 2^32

unsigned int seed = 12345; // initial seed value

// Generate a pseudo-random number between 0 and M-1
unsigned int random()
{
  seed = (A * seed + C) % M;
  return seed;
}

int sys_random(void)
{
  struct rtcdate rtime;
  // Get the current system time
  cmostime(&rtime);
  seed = (rtime.hour + 60 * rtime.minute + 3600 * rtime.second);

  int rand_num;
  // Generate a random number between 0 and M-1
  // for (int i = 0; i < 100; i++)
  rand_num = random();
  return rand_num;
}
