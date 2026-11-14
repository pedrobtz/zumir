/* MIR's c-tests corpus through zumir's compile path (design §8, #20): each
 * test is compiled by c2mir from memory after cfunction()'s prelude, with
 * no header read from a file, linked against D6's list plus the corpus
 * harness's printf, abort and exit (declared by a test-only stdio.h and
 * stdlib.h, src/zumir_c2mir.c), and its main run on one tier in a child
 * process. The verdict follows c-tests/runtests.sh: the exit status must be
 * the test's .expectrc (0 without one), and the output its .expect when it
 * has one. Built without R by tools/ci/platform.sh:
 *
 *   cc -Isrc -Isrc/vendor/mir -DNDEBUG -DZUMIR_C2MIR_TEST_HEADERS \
 *     tests/c/c-tests-runner.c src/zumir_mir.c src/zumir_mir_gen.c \
 *     src/zumir_c2mir.c src/zumir_prelude.c src/zumir_imports_d6.c \
 *     src/zumir_mir_glue.c src/zumir_mir_stdio.c src/zumir_trap.c -lm
 *
 * Usage: c-tests-runner <c-tests dir> <gen|interp> <dir>...
 * Prints one line per test: "<path> <verdict> <reason>", verdict one of
 * pass, fail or skip (as runtests.sh skips), and a total. POSIX only (fork). */
#include <dirent.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* The real stderr, taken before zumir_mir_api.h redirects the name. */
static FILE *runner_stderr(void) {
  return stderr;
}

#include "zumir_imports.h"
#include "zumir_mir_api.h"
#include "zumir_mir_sink.h"
#include "zumir_prelude.h"
#include "zumir_trap.h"
#include "vendor/mir/c2mir/c2mir.h"
#include "vendor/mir/mir-gen.h"

/* zumir_mir_api.h redirects stderr, the stdio writers, exit and abort into
 * MIR's message buffer and the trap, for MIR's code; the runner writes its
 * own reasons to the real stderr (runner_stderr ()), and its children
 * really exit. */
#undef fprintf
#undef vfprintf
#undef fputc
#undef perror
#undef exit
#undef abort

/* The harness's headers: the three names the corpus's oracle needs, and
 * what the real headers would also give the tests that use them (NULL;
 * assert, which fails by abort, as the corpus's tests do). */
const char zumir_test_stdio_h[] =
  "#ifndef _ZUMIR_TEST_STDIO_H\n#define _ZUMIR_TEST_STDIO_H 1\n"
  "#include <stddef.h>\nint printf (const char *, ...);\n#endif\n";
const char zumir_test_stdlib_h[] =
  "#ifndef _ZUMIR_TEST_STDLIB_H\n#define _ZUMIR_TEST_STDLIB_H 1\n"
  "#include <stddef.h>\nvoid abort (void);\nvoid exit (int);\n#endif\n";
const char zumir_test_assert_h[] =
  "#ifndef _ZUMIR_TEST_ASSERT_H\n#define _ZUMIR_TEST_ASSERT_H 1\n"
  "void abort (void);\n#define assert(e) ((e) ? (void) 0 : abort ())\n#endif\n";

/* c2m's exit status for a source it cannot compile or link, so a test
 * that expects that (havoc's .expectrc) passes as it does under c2m. */
enum { RC_ERROR = 1, RC_NOMAIN = 203, RC_READ = 204 };

static char *read_file(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) return NULL;
  size_t cap = 4096, len = 0;
  char *buf = malloc(cap);
  size_t n;
  while ((n = fread(buf + len, 1, cap - len - 1, f)) > 0) {
    len += n;
    if (cap - len < 2) buf = realloc(buf, cap *= 2);
  }
  fclose(f);
  buf[len] = '\0';
  return buf;
}

static int exists(const char *path) {
  return access(path, F_OK) == 0;
}

static int ends_with(const char *s, const char *suffix) {
  size_t a = strlen(s), b = strlen(suffix);
  return a >= b && strcmp(s + a - b, suffix) == 0;
}

/* The same text cfunction() gives c2mir (src/zumir_cfun.c). */
#define SOURCE_PARTS 5
typedef struct {
  const char *part[SOURCE_PARTS];
  int k;
  size_t pos;
} source_reader;

static int source_getc(void *data) {
  source_reader *r = data;
  while (r->k < SOURCE_PARTS) {
    unsigned char c = (unsigned char) r->part[r->k][r->pos];
    if (c != '\0') {
      r->pos++;
      return c;
    }
    r->k++;
    r->pos = 0;
  }
  return EOF;
}

static int compile_c(MIR_context_t ctx, const char *text, const char *name) {
  struct c2mir_options options;
  memset(&options, 0, sizeof options);
  options.message_file = ZUMIR_MIR_STDERR;
  options.module_num = 0;
  source_reader r = {{zumir_prelude_prefix, name, zumir_prelude_line0, text, "\n"}, 0, 0};
  return c2mir_compile(ctx, &options, source_getc, &r, name, NULL);
}

/* In the child: compile, link, run main; the exit status is main's, or
 * one of RC_* with the reason on stderr. */
/* The corpus root, cut from the names c2mir and the verdicts give. */
static size_t root_len;

static const char *rel(const char *path) {
  return path + root_len + (path[root_len] == '/');
}

static void child(const char *path, const char *add, int gen) {
  char *src = read_file(path), *add_src = add != NULL ? read_file(add) : NULL;
  if (src == NULL || (add != NULL && add_src == NULL)) _exit(RC_READ);
  MIR_context_t volatile ctx = NULL;
  zumir_arena *volatile arena = NULL;
  void *volatile fn = NULL;
  zumir_trap t;
  zumir_trap_push(&t);
  if (setjmp(t.env) == 0) {
    ctx = zumir_mir_new(&arena);
    zumir_mir_message_clear();
    c2mir_init(ctx);
    int ok = compile_c(ctx, src, rel(path));
    if (ok && add_src != NULL) {
      if (ends_with(add, ".mir"))
        MIR_scan_string(ctx, add_src);
      else
        ok = compile_c(ctx, add_src, rel(add));
    }
    c2mir_finish(ctx);
    if (!ok) {
      fprintf(runner_stderr(), "compile: %s", zumir_mir_message());
      _exit(RC_ERROR);
    }
    MIR_item_t main_item = NULL;
    for (MIR_module_t m = DLIST_HEAD(MIR_module_t, *MIR_get_module_list(ctx)); m != NULL;
         m = DLIST_NEXT(MIR_module_t, m)) {
      MIR_load_module(ctx, m);
      for (MIR_item_t it = DLIST_HEAD(MIR_item_t, m->items); it != NULL;
           it = DLIST_NEXT(MIR_item_t, it))
        if (it->item_type == MIR_func_item && strcmp(it->u.func->name, "main") == 0)
          main_item = it;
    }
    size_t n;
    const zumir_import_entry *d6 = zumir_import_d6(&n);
    for (size_t i = 0; i < n; i++) MIR_load_external(ctx, d6[i].name, d6[i].addr);
    MIR_load_external(ctx, "printf", (void *) printf);
    MIR_load_external(ctx, "abort", (void *) abort);
    MIR_load_external(ctx, "exit", (void *) exit);
    if (gen) {
      MIR_gen_init(ctx);
      MIR_link(ctx, MIR_set_gen_interface, NULL);
    } else {
      MIR_link(ctx, MIR_set_interp_interface, NULL);
    }
    if (main_item == NULL) _exit(RC_NOMAIN);
    fn = main_item->addr;
    zumir_trap_pop(&t);
  } else {
    fprintf(runner_stderr(), "link: %s\n", t.message);
    _exit(RC_ERROR);
  }
  int (*main_fn)(int, char **);
  memcpy(&main_fn, (void *const *) &fn, sizeof main_fn);
  char *argv[] = {(char *) path, NULL};
  int rc = main_fn(1, argv);
  fflush(stdout);
  _exit(rc & 255);
}

/* Reads fd to the end into a malloc'd string. */
static char *slurp(int fd) {
  size_t cap = 4096, len = 0;
  char *buf = malloc(cap);
  ssize_t n;
  while ((n = read(fd, buf + len, cap - len - 1)) > 0) {
    len += (size_t) n;
    if (cap - len < 2) buf = realloc(buf, cap *= 2);
  }
  buf[len] = '\0';
  return buf;
}

/* The text without carriage returns, as diff --strip-trailing-cr. */
static void strip_cr(char *s) {
  char *w = s;
  for (; *s; s++)
    if (*s != '\r') *w++ = *s;
  *w = '\0';
}

static int all, passed, failed, skipped;

static void verdict(const char *path, const char *v, const char *why) {
  printf("%s %s %s\n", rel(path), v, why);
  if (strcmp(v, "pass") == 0) passed++;
  if (strcmp(v, "fail") == 0) failed++;
  if (strcmp(v, "skip") == 0) skipped++;
}

/* The first line of s, at most 200 bytes, with no newline. */
static const char *first_line(const char *s, char *buf, size_t size) {
  size_t i = 0;
  while (s[i] != '\0' && s[i] != '\n' && i + 1 < size && i < 200) {
    buf[i] = s[i];
    i++;
  }
  buf[i] = '\0';
  return buf;
}

static void run_test(const char *path, const char *add, int gen, const char *arch) {
  char aux[4096], why[512];
  snprintf(aux, sizeof aux, "%s.mach", path);
  if (exists(aux)) {
    char *m = read_file(aux);
    int ok = m != NULL && strstr(m, arch) != NULL;
    free(m);
    if (!ok) return verdict(path, "skip", "runtests.sh: .mach");
  }
  snprintf(aux, sizeof aux, "%s.nomach", path);
  if (exists(aux)) {
    char *m = read_file(aux);
    int no = m != NULL && strstr(m, arch) != NULL;
    free(m);
    if (no) return verdict(path, "skip", "runtests.sh: .nomach");
  }
  snprintf(aux, sizeof aux, "%s.opt", path);
  if (exists(aux)) return verdict(path, "skip", "runtests.sh: .opt (a c2m option)");
  snprintf(aux, sizeof aux, "%s.disable", path);
  if (exists(aux)) {
    char *m = read_file(aux);
    int off = m != NULL && strstr(m, arch) != NULL;
    free(m);
    if (off) return verdict(path, "skip", "runtests.sh: .disable");
  }
  all++;
  int expect_rc = 0;
  snprintf(aux, sizeof aux, "%s.expectrc", path);
  if (exists(aux)) {
    char *m = read_file(aux);
    expect_rc = m != NULL ? atoi(m) : 0;
    free(m);
  }
  char *expect = NULL;
  snprintf(aux, sizeof aux, "%s.expect", path);
  if (exists(aux)) expect = read_file(aux);
  if (expect == NULL && ends_with(path, ".c")) {
    snprintf(aux, sizeof aux, "%.*s.expect", (int) (strlen(path) - 2), path);
    if (exists(aux)) expect = read_file(aux);
  }
  int out[2], err[2];
  if (pipe(out) != 0 || pipe(err) != 0) {
    perror("pipe");
    exit(2);
  }
  fflush(stdout);
  pid_t pid = fork();
  if (pid == 0) {
    dup2(out[1], 1);
    dup2(err[1], 2);
    close(out[0]);
    close(err[0]);
    close(out[1]);
    close(err[1]);
    alarm(30);
    child(path, add, gen);
  }
  close(out[1]);
  close(err[1]);
  /* stderr is small (one reason line); stdout may be large. Read stdout
   * first, then stderr, which a child cannot block on for long. */
  char *got = slurp(out[0]), *errs = slurp(err[0]);
  close(out[0]);
  close(err[0]);
  int st;
  waitpid(pid, &st, 0);
  char line[256];
  if (getenv("ZUMIR_CTESTS_SHOW_STDERR") != NULL && errs[0] != '\0')
    fprintf(runner_stderr(), "--- %s stderr:\n%s\n", rel(path), errs);
  if (WIFSIGNALED(st)) {
    snprintf(why, sizeof why, "signal %d%s", WTERMSIG(st), WTERMSIG(st) == SIGALRM ? " (timeout)" : "");
    verdict(path, "fail", why);
  } else {
    int rc = WEXITSTATUS(st);
    if (rc != expect_rc && errs[0] != '\0') {
      verdict(path, "fail", first_line(errs, line, sizeof line));
    } else if (rc == RC_NOMAIN) {
      verdict(path, "fail", "no main");
    } else if (rc == RC_READ) {
      verdict(path, "fail", "cannot read the test");
    } else if (rc != expect_rc) {
      snprintf(why, sizeof why, "exit %d, expected %d", rc, expect_rc);
      verdict(path, "fail", why);
    } else if (expect != NULL) {
      strip_cr(expect);
      strip_cr(got);
      verdict(path, strcmp(expect, got) == 0 ? "pass" : "fail",
              strcmp(expect, got) == 0 ? "-" : "output differs from .expect");
    } else {
      verdict(path, "pass", "-");
    }
  }
  free(got);
  free(errs);
  free(expect);
}

static int cmp_str(const void *a, const void *b) {
  return strcmp(*(char *const *) a, *(char *const *) b);
}

int main(int argc, char **argv) {
  if (argc < 4) {
    fprintf(runner_stderr(), "usage: %s <c-tests dir> <gen|interp> <dir>...\n", argv[0]);
    return 2;
  }
  const char *root = argv[1];
  root_len = strlen(root);
  int gen = strcmp(argv[2], "gen") == 0;
#if defined(__x86_64__)
  const char *arch = "x86_64";
#elif defined(__aarch64__)
  const char *arch = "aarch64";
#else
  const char *arch = "other";
#endif
  for (int d = 3; d < argc; d++) {
    char dir[4096];
    snprintf(dir, sizeof dir, "%s/%s", root, argv[d]);
    DIR *dp = opendir(dir);
    if (dp == NULL) {
      fprintf(runner_stderr(), "cannot open %s\n", dir);
      return 2;
    }
    char *names[4096];
    int nn = 0;
    struct dirent *e;
    while ((e = readdir(dp)) != NULL && nn < 4096)
      if (ends_with(e->d_name, ".c")) names[nn++] = strdup(e->d_name);
    closedir(dp);
    qsort(names, (size_t) nn, sizeof names[0], cmp_str);
    char path[4096], add[4096];
    snprintf(path, sizeof path, "%s/main.c", dir);
    if (exists(path)) {
      run_test(path, NULL, gen, arch);
    } else {
      for (int i = 0; i < nn; i++) {
        if (strncmp(names[i], "add-", 4) == 0) continue;
        snprintf(path, sizeof path, "%s/%s", dir, names[i]);
        const char *a = NULL;
        snprintf(add, sizeof add, "%s/add-%s", dir, names[i]);
        if (exists(add)) a = add;
        else {
          snprintf(add, sizeof add, "%s/add-%.*s.mir", dir, (int) strlen(names[i]) - 2, names[i]);
          if (exists(add)) a = add;
        }
        run_test(path, a, gen, arch);
      }
    }
    for (int i = 0; i < nn; i++) free(names[i]);
  }
  printf("Tests %d, passed %d, failed %d, skipped %d\n", all, passed, failed, skipped);
  return 0;
}
