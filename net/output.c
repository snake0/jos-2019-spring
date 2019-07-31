#include "ns.h"

extern union Nsipc nsipcbuf;

void
output(envid_t ns_envid) {
  binaryname = "ns_output";

  // LAB 6: Your code here:
  // 	- read a packet request (using ipc_recv)
  //	- send the packet to the device driver (using sys_net_send)
  //	do the above things in a loop
  struct jif_pkt *jifpkt;
  int r;
  envid_t whom;
  int perm;

  for (;;) {
    r = ipc_recv(&whom, &nsipcbuf, &perm);
    if (r != NSREQ_OUTPUT)
      continue;
    jifpkt = &nsipcbuf.pkt;
    while (sys_net_send(jifpkt->jp_data, jifpkt->jp_len) < 0)
      sys_yield();
  }
}
