/*
 * freestanding_asan.h
 * Minimal freestanding ASan-like runtime (1:8 shadow) - header
 *
 * This is a lightweight reference implementation intended for hobby kernels
 * or freestanding environments. It provides the minimal set of symbols the
 * compiler typically emits when you compile with -fsanitize=address in
 * "outline" mode.
 *
 * Shadow semantics (simplified ASan-style):
 *  - Shadow covers a contiguous watched region [region_base, region_base+size).
 *  - One shadow byte represents 8 bytes of real memory.
 *  - Each shadow byte value is in range 0..8:
 *      8 => all 8 bytes in that 8-byte block are addressable (unpoisoned)
 *      0..7 => number of *lowest-address* bytes in the 8-byte block that are
 *              addressable; the remaining higher bytes are poisoned.
 *
 * Limitations / notes:
 *  - This header+impl are educational. They trade completeness for tiny
 *    code size and straightforward logic. They do *not* provide full ASan
 *    semantics (origins, leak detection, threaded reports, or global DSO
 *    registration beyond no-op stubs).
 *  - The implementation uses a statically sized shadow buffer. You can
 *    replace this with a dynamically mapped region if desired.
 */

#ifndef FREESTANDING_ASAN_H
#define FREESTANDING_ASAN_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configure max watched real memory in bytes (tune for your kernel) */
#ifndef ASAN_MAX_WATCHED_MEM
#define ASAN_MAX_WATCHED_MEM (32 * 1024 * 1024U) /* 32 MiB by default */
#endif

/* Derived shadow size (one shadow byte per 8 real bytes) */
#define ASAN_SHADOW_SIZE ((ASAN_MAX_WATCHED_MEM + 7) / 8)

/* Public API: user should call asan_set_region() early to tell the
 * runtime which virtual address region to watch, before any instrumented
 * code runs. */
void asan_set_region(void *base, size_t size);

/* Optional hooks the kernel can override to capture output / panic: */
void asan_write(const char *s);         /* write a NUL-terminated string */
void asan_write_hex(unsigned long v);   /* write a hex value and newline */
void asan_panic(void);                  /* stop the system (or reboot) */

/* Minimal set of symbols emitted by compiler instrumentation */
void __asan_init(void);

/* Poison / unpoison region in bytes (may be arbitrary ranges). */
void __asan_poison_memory_region(void *addr, size_t size);
void __asan_unpoison_memory_region(void *addr, size_t size);

/* Outline-mode access checks (the compiler may emit both _load/_store and
 * the report_ variants; we provide both). */

/* Fixed-size reports (called by slow path reporting) */
void __asan_report_load1(void *addr);
void __asan_report_load2(void *addr);
void __asan_report_load4(void *addr);
void __asan_report_load8(void *addr);
void __asan_report_load16(void *addr);

void __asan_report_store1(void *addr);
void __asan_report_store2(void *addr);
void __asan_report_store4(void *addr);
void __asan_report_store8(void *addr);
void __asan_report_store16(void *addr);

/* Generic n-byte variants */
void __asan_report_load_n(size_t size, void *addr);
void __asan_report_store_n(size_t size, void *addr);

/* Outline wrappers (compiler may call these before an access) */
void __asan_load1(void *addr);
void __asan_load2(void *addr);
void __asan_load4(void *addr);
void __asan_load8(void *addr);
void __asan_load16(void *addr);

void __asan_store1(void *addr);
void __asan_store2(void *addr);
void __asan_store4(void *addr);
void __asan_store8(void *addr);
void __asan_store16(void *addr);

/* Global registration stubs (no-op here) */
void __asan_register_globals(void *globals, size_t n);
void __asan_unregister_globals(void *globals, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* FREESTANDING_ASAN_H */
