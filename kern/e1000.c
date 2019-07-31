#include <kern/e1000.h>
#include <kern/pmap.h>
#include <inc/string.h>
#include <inc/error.h>

static struct E1000 *base;
char e1000_macaddr[MACADDR_SIZE];

static uint16_t
e1000_read_eeprom(uint8_t addr) {
  base->EERD = addr << 8 | E1000_EERD_START;
  while (!(base->EERD & E1000_EERD_DONE));
  return base->EERD >> 16;
}

static void e1000_mac_init() {
  for (unsigned i = 0; i < 3; ++i)
    *(uint16_t *) (e1000_macaddr + (i << 1U)) = e1000_read_eeprom(i);
}

#define N_TXDESC (PGSIZE / sizeof(struct tx_desc))
struct tx_desc tx_descs[N_TXDESC] __attribute__((aligned(16)));
char tx_bufs[N_TXDESC][E1000_PKTSIZE];

static int
e1000_tx_init() {
  // Allocate one page for descriptors
  memset(tx_descs, 0, sizeof(tx_descs));
  memset(tx_bufs, 0, sizeof(tx_bufs));

  // Initialize all descriptors
  for (int i = 0; i < N_TXDESC; ++i) {
    tx_descs[i].addr = PADDR(tx_bufs[i]);
    tx_descs[i].cmd = 0;
    tx_descs[i].status |= E1000_TX_STATUS_DD;
  }

  // Set hardward registers
  base->TDLEN = sizeof(struct tx_desc) * N_TXDESC;
  base->TDBAL = PADDR(tx_descs);
  base->TDBAH = 0;
  base->TDH = 0;
  base->TDT = 0;

  // transmit control
  base->TCTL |= E1000_TCTL_EN;
  base->TCTL |= E1000_TCTL_PSP;
  base->TCTL |= E1000_TCTL_CT_ETHER;
  base->TCTL |= E1000_TCTL_COLD_FULL_DUPLEX;

  // inter-packet gap
  base->TIPG = E1000_TIPG_DEFAULT;
  return 0;
}

#define N_RXDESC (PGSIZE / sizeof(struct rx_desc))
struct rx_desc rx_descs[N_RXDESC] __attribute__((aligned(16)));
char rx_bufs[N_RXDESC][E1000_PKTSIZE];

static int
e1000_rx_init() {
  // Allocate one page for descriptors
  memset(rx_descs, 0, sizeof(rx_descs));
  memset(rx_bufs, 0, sizeof(rx_bufs));

  // Initialize all descriptors
  // You should allocate some pages as receive buffer
  for (int i = 0; i < N_RXDESC; ++i)
    rx_descs[i].addr = PADDR(rx_bufs[i]);

  // Set hardward registers
  base->RAL = *(uint32_t *) e1000_macaddr;
  base->RAH = *(uint16_t *) (e1000_macaddr + 4) | 0x80000000;
  base->RDBAL = PADDR(rx_descs);
  base->RDBAH = 0;
  base->RDLEN = sizeof(rx_descs);
  base->RDH = 1;
  base->RDT = 0;

  base->RCTL |= E1000_RCTL_EN;
  base->RCTL |= E1000_RCTL_BSIZE_2048;
  base->RCTL |= E1000_RCTL_SECRC;
  return 0;
}


int
pci_e1000_attach(struct pci_func *pcif) {
  // Enable PCI function
  pci_func_enable(pcif);

  // Map MMIO region and save the address in 'base;
  base = (struct E1000 *) mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);
  assert(base->STATUS == 0x80080783);

  e1000_mac_init();
  e1000_tx_init();
  e1000_rx_init();
  return 0;
}

// return -E_AGAIN on queue full, 0 on OK
int
e1000_tx(const void *buf, uint32_t len) {
  // Send 'len' bytes in 'buf' to ethernet
  // Hint: buf is a kernel virtual address
  if (!buf || len > E1000_PKTSIZE)
    return -E_INVAL;

  uint32_t tdt = base->TDT;
  struct tx_desc *txDesc = &tx_descs[tdt];
  char *tx_buf = tx_bufs[tdt];

  if (!(txDesc->status & E1000_TX_STATUS_DD))
    return -E_AGAIN;

  txDesc->length = len;
  txDesc->status &= ~E1000_TX_STATUS_DD;
  txDesc->cmd |= (E1000_TX_CMD_EOP | E1000_TX_CMD_RS);
  memcpy(tx_buf, buf, len);
  base->TDT = (tdt + 1) % N_TXDESC;
  return 0;
}

int
e1000_rx(void *buf, uint32_t len) {
  // Copy one received buffer to buf
  // You could return -E_AGAIN if there is no packet
  // Check whether the buf is large enough to hold
  // the packet
  // Do not forget to reset the decscriptor and
  // give it back to hardware by modifying RDT
  if (!buf)
    return -E_INVAL;

  uint32_t rdt = (base->RDT + 1) % N_RXDESC;
  struct rx_desc *rxDesc = &rx_descs[rdt];
  char *rx_buf = rx_bufs[rdt];

  if (!(rxDesc->status & E1000_RX_STATUS_DD))
    return -E_AGAIN;
  if (rxDesc->error)
    return -E_AGAIN;

  rxDesc->status &= ~E1000_RX_STATUS_DD;
  memcpy(buf, rx_buf, rxDesc->length);
  base->RDT = rdt;
  return rxDesc->length;
}

