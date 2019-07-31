//
// Created by Esaki Shigeki on 2019-05-23.
//

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
#ifdef CHALLENGE_LAB5
  int r;

  cprintf("testexec: Hello, world!\n");

  cprintf("testexec: going to exec /init\n");
  if ((r = execl("/init", "init", "initarg1", "initarg2", (char*)0)) < 0)
    panic("testexec: exec /init: %e\n", r);

  cprintf("SHOULD HAVE DIED\n");
#endif
}
