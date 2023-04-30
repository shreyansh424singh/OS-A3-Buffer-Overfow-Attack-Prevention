#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "x86.h"
#include "elf.h"
#include "aslr.h"

// static unsigned int bseed = 0;

// unsigned int basic_rand(void) {
//   const unsigned int a = 1664525;
//   const unsigned int c = 1013904223;
//   const unsigned int m = 2294967296; // 2^32
//   bseed = (a * bseed + c) % m;
//   return bseed;
// }

int
exec(char *path, char **argv)
{
  if(aslr_enabled==1) cprintf("exec %s: \n", path);

  cprintf("ASLR value %d\n", aslr_enabled);

  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3+MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    if(aslr_enabled==1) cprintf("exec: fail\n");
    return -1;
  }
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  uint rnd = 10; // generate a random offset value
  if (aslr_enabled) {
    // uint rnd = random() % PGSIZE; // generate a random offset value
    if(aslr_enabled==1) cprintf("Random offset: %x\n", rnd);
    if(aslr_enabled==1) cprintf("entry point: %x\n", elf.entry);
    elf.entry += rnd++;            // apply the offset to program entry point
    if(aslr_enabled==1) cprintf("New entry point: %x\n", elf.entry);
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
      if (readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
        goto bad;
      if(aslr_enabled==1) cprintf("segment address: %x\n", ph.vaddr);
      ph.vaddr += rnd++;           // apply the offset to program segment
      if(aslr_enabled==1) cprintf("New segment address: %x\n", ph.vaddr);
    }
    for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
      if (readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
        goto bad;
      if(aslr_enabled==1) cprintf("New segment address: %x\n", ph.vaddr);
    }
  }


if(aslr_enabled==1) cprintf("Pgdir before %d\n", pgdir);
  if((pgdir = setupkvm()) == 0)
    goto bad;
if(aslr_enabled==1) cprintf("Pgdir after %d\n", pgdir);    

  // if(aslr_enabled==1){
  //   // uint rnd = random() % PGSIZE; // generate a random offset value
  //   ph.vaddr += 1;
  //   ph.memsz += 10;
  //   ph.paddr += 100;
  // }

  // Load program into memory.
  sz = 0;
  // int ads=0;

  // if(aslr_enabled==1) cprintf("fffffff %d %d %d  \n", path[4], path[5], path[6]);
  // if(path[5] != 110 && path[5]!=0){
  //   ads=4096;
  // }


  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){


    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;

if(aslr_enabled==1) cprintf("ph type %d \n", ph.type);
if(aslr_enabled==1) cprintf("ph off %d \n", ph.off);
if(aslr_enabled==1) cprintf("ph vaddr %d \n", ph.vaddr);
if(aslr_enabled==1) cprintf("ph paddr %d \n", ph.paddr);
if(aslr_enabled==1) cprintf("ph filesz %d \n", ph.filesz);
if(aslr_enabled==1) cprintf("ph memsz %d \n", ph.memsz);
if(aslr_enabled==1) cprintf("ph flags %d \n", ph.flags);
if(aslr_enabled==1) cprintf("ph align %d \n", ph.align);

// ph.vaddr += ads;           // apply the offset to program segment
// ph.filesz += ads;
// sz += ads;
  if(aslr_enabled==1) cprintf("sz before %d %d %d\n", sz, ph.vaddr, ph.memsz);
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
  if(aslr_enabled==1) cprintf("sz after %d\n", sz);
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  if(aslr_enabled==1) cprintf("3 sz %d\n", sz);
  sz = PGROUNDUP(sz);
  if(aslr_enabled==1) cprintf("4 sz %d\n", sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  if(aslr_enabled==1) cprintf("5 sz %d\n", sz);
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  if(aslr_enabled==1) cprintf("6 sz %d\n", sz);
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[3+argc] = sp;
  }
  ustack[3+argc] = 0;

  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc+1)*4;  // argv pointer

  sp -= (3+argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry;  // main
  curproc->tf->esp = sp;
  switchuvm(curproc);
  freevm(oldpgdir);

if(aslr_enabled==1) cprintf("EXEC fucntion completed succesfully\n");

  return 0;

 bad:
 if(aslr_enabled==1) cprintf("BAD***************************\n");
  if(pgdir)
    freevm(pgdir);
  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}
