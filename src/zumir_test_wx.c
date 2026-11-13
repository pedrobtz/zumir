/* Test-only (Linux): refuse writable-and-executable memory in this process
 * from now on, as SELinux deny_execmem or a seccomp policy would, so a
 * test can see the tier probe fail cleanly (tests/testthat/test-tier.R).
 * The filter cannot be removed: call it only in a child R process. From
 * the spike's harness (pedrobtz/mirspike). */
#define R_NO_REMAP
#include <R.h>
#include <Rinternals.h>

#if defined(__linux__)
#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/syscall.h>

/* Returns TRUE when the filter is in place and bites: a W+X mapping is
 * then refused with EACCES. */
SEXP zumir_test_deny_wx(void) {
  struct sock_filter f[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mprotect, 2, 0),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mmap, 1, 0),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, args[2])),
    BPF_STMT(BPF_ALU | BPF_AND | BPF_K, PROT_WRITE | PROT_EXEC),
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, PROT_WRITE | PROT_EXEC, 0, 1),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | (EACCES & SECCOMP_RET_DATA)),
    BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
  };
  struct sock_fprog prog = {(unsigned short) (sizeof f / sizeof f[0]), f};
  if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) return Rf_ScalarLogical(FALSE);
  if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog) != 0) return Rf_ScalarLogical(FALSE);
  void *p = mmap(NULL, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (p != MAP_FAILED) {
    munmap(p, 4096);
    return Rf_ScalarLogical(FALSE);
  }
  return Rf_ScalarLogical(TRUE);
}
#else
SEXP zumir_test_deny_wx(void) {
  return Rf_ScalarLogical(NA_LOGICAL);
}
#endif
