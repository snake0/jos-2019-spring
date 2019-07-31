// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW    0x800

extern void _pgfault_upcall(void);

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf) {
    void *addr = (void *) utf->utf_fault_va;
    uint32_t err = utf->utf_err;

    // Check that the faulting access was (1) a write, and (2) to a
    // copy-on-write page.  If not, panic.
    // Hint:
    //   Use the read-only page table mappings at uvpt
    //   (see <inc/memlayout.h>).

    // LAB 4: Your code here.
    if (!(err & FEC_WR) || !(uvpd[PDX(addr)] & PTE_P) ||
        (uvpt[PGNUM(addr)] & (PTE_P | PTE_COW)) != (PTE_P | PTE_COW)) {
        panic("pgfault: faulting access invalid");
    }

    // Allocate a new page, map it at a temporary location (PFTEMP),
    // copy the data from the old page to the new page, then move the new
    // page to the old page's address.
    // Hint:
    //   You should make three system calls.

    // LAB 4: Your code here.

    if (sys_page_alloc(0, PFTEMP, PTE_P | PTE_U | PTE_W))
        panic("sys_page_alloc failed");

    addr = ROUNDDOWN(addr, PGSIZE);
    memmove(PFTEMP, addr, PGSIZE);

    if (sys_page_map(0, PFTEMP, 0, addr, PTE_P | PTE_U | PTE_W))
        panic("sys_page_map failed");
    if (sys_page_unmap(0, PFTEMP))
        panic("sys_page_unmap failed");

}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn) {
    // LAB 4: Your code here.
    void *vptr = (void *) (pn * PGSIZE);
    int r;

    if (uvpt[pn] & PTE_SHARE) {
        if ((r = sys_page_map(0, vptr, envid, vptr, uvpt[pn] & PTE_SYSCALL)) < 0)
            panic("duppage :in sys_page_map %e", r);

    } else if (uvpt[pn] & (PTE_W | PTE_COW)) {

        if ((r = sys_page_map(0, vptr, envid, vptr, PTE_P | PTE_U | PTE_COW)) < 0)
            panic("duppage :in sys_page_map %e", r);
        if ((r = sys_page_map(0, vptr, 0, vptr, PTE_P | PTE_U | PTE_COW)) < 0)
            panic("duppage :in sys_page_map %e", r);

    } else if ((r = sys_page_map(0, vptr, envid, vptr, PTE_P | PTE_U)) < 0)
        panic("duppage :in sys_page_map %e", r);

    return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void) {
    // LAB 4: Your code here.
    set_pgfault_handler(pgfault);

    envid_t envid = sys_exofork();
    if (envid < 0)
        panic("fork: sys_exofork error: %e", envid);

    if (envid == 0) {
        thisenv = &envs[ENVX(sys_getenvid())];
        return 0;
    }

    uintptr_t addr;
    for (addr = 0; addr < UTOP; addr += PGSIZE) {
        if (uvpd[PDX(addr)] & PTE_P && uvpt[PGNUM(addr)] & PTE_P && addr != UXSTACKTOP - PGSIZE) {
            duppage(envid, PGNUM(addr));
        }
    }
    if (sys_page_alloc(envid, (void *) (UXSTACKTOP - PGSIZE), PTE_P | PTE_U | PTE_W))
        panic("fork: sys_page_alloc failed");
    if (sys_env_set_pgfault_upcall(envid, _pgfault_upcall))
        panic("fork: sys_env_set_pgfault_upcall failed");
    if (sys_env_set_status(envid, ENV_RUNNABLE))
        panic("fork: sys_env_set_status failed");

    return envid;

}

// Challenge!
int
sfork(void) {
    panic("sfork not implemented");
}
