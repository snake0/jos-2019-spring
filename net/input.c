#include "ns.h"

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid) {
  binaryname = "ns_input";

  // LAB 6: Your code here:
  // 	- read a packet from the device driver
  //	- send it to the network server (using ipc_send with NSREQ_INPUT as value)
  //	do the above things in a loop
  // Hint: When you IPC a page to the network server, it will be
  // reading from it for a while, so don't immediately receive
  // another packet in to the same physical page.
  char buf[2048];

  for(;;) {
    int len;
    if ((len = sys_net_recv(buf, 0)) < 0) {
      sys_sleep(30);
      continue;
    }
    if (sys_page_alloc(0, &nsipcbuf, PTE_P | PTE_W | PTE_U) < 0)
      continue;
    memmove(nsipcbuf.pkt.jp_data, buf, len);
    nsipcbuf.pkt.jp_len = len;
    ipc_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_P | PTE_W | PTE_U);
  }
}
