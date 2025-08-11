/*
 * freestanding_asan.c
 * Minimal freestanding ASan-like runtime (1:8 shadow)
 *
 * See freestanding_asan.h for API and semantics.
 */

#include "asanhooks.hpp"
#include <kernel/kernel/log.hpp>
#include <kernel/kernel/panic.hpp>

#ifdef __cplusplus
extern "C" {
#endif

/* Static shadow array. One shadow byte per 8 bytes of watched memory.
 * Shadow value semantics:
 *   8 => all 8 bytes accessible
 *   0..7 => number of lowest-address bytes (starting at the 8-byte block's
 *           beginning) that are accessible. The remaining high bytes are
 *           poisoned.
 */
static uint8_t asan_shadow[ASAN_SHADOW_SIZE];
static void *asan_region_base = NULL;
static size_t asan_region_size = 0;
static size_t asan_shadow_len = 0; /* number of shadow bytes in use */

/* Weak hooks: override these in your kernel to route output/panic */
__attribute__((weak)) void asan_write(const char *s) { log(INFO, "%s", s); }
__attribute__((weak)) void asan_write_hex(unsigned long v) { log(INFO, "%p", v); }
__attribute__((weak)) void asan_panic(void) { kn::panic("ASAN PANIC"); }

/* Helper: return 1 if address inside watched region */
static inline int asan_in_region(uintptr_t a)
{
    if (!asan_region_base) return 0;
    uintptr_t base = (uintptr_t)asan_region_base;
    return (a >= base && a < base + asan_region_size);
}

/* Map an address to shadow index and offset within the 8-byte block. */
static inline void asan_map(uintptr_t a, size_t *out_idx, unsigned *out_off)
{
    uintptr_t base = (uintptr_t)asan_region_base;
    size_t rel = (size_t)(a - base);
    *out_idx = rel >> 3;          /* /8 */
    *out_off = (unsigned)(rel & 7); /* mod 8 */
}

void asan_set_region(void *base, size_t size)
{
    asan_region_base = base;
    asan_region_size = size;
    asan_shadow_len = (size + 7) / 8;
    if (asan_shadow_len > ASAN_SHADOW_SIZE) {
        /* clamp to static buffer; user should increase ASAN_MAX_WATCHED_MEM */
        asan_shadow_len = ASAN_SHADOW_SIZE;
    }
    /* Default to fully-unpoisoned */
    for (size_t i = 0; i < asan_shadow_len; ++i) asan_shadow[i] = 8;
}

void __asan_init(void) { /* no-op; region must be set explicitly */ }

/* Compute how many lowest bytes in an 8-byte block are unpoisoned given a
 * desired unpoisoned mask (start..end). We take the union: if any byte in
 * the prefix is unpoisoned, it's considered in prefix count. Simpler: the
 * number of contiguous unpoisoned bytes starting at block start. */
static inline unsigned compute_unpoison_prefix(uintptr_t block_start, uintptr_t unpoison_start, uintptr_t unpoison_end)
{
    uintptr_t block_end = block_start + 8;
    /* compute intersection of [block_start, block_end) and [unpoison_start, unpoison_end) */
    uintptr_t lo = (unpoison_start > block_start) ? unpoison_start : block_start;
    uintptr_t hi = (unpoison_end < block_end) ? unpoison_end : block_end;
    if (hi <= lo) return 0;
    /* number of unpoisoned bytes in this block */
    unsigned count = (unsigned)(hi - block_start);
    if (count > 8) count = 8;
    return count;
}

void __asan_poison_memory_region(void *addr, size_t size)
{
    if (!asan_region_base || size == 0) return;
    uintptr_t a = (uintptr_t)addr;
    uintptr_t end = a + size;
    uintptr_t base = (uintptr_t)asan_region_base;

    /* Clamp to watched region */
    if (a < base) a = base;
    if (end > base + asan_region_size) end = base + asan_region_size;
    if (a >= end) return;

    size_t start_idx = (a - base) >> 3;
    size_t end_idx = ((end - base) + 7) >> 3; /* ceil */
    if (start_idx >= asan_shadow_len) return;
    if (end_idx > asan_shadow_len) end_idx = asan_shadow_len;

    for (size_t i = start_idx; i < end_idx; ++i) {
        uintptr_t block_start = base + (i << 3);
        /* After poisoning a region, the number of unpoisoned prefix bytes
         * is: bytes in [block_start, block_start+8) that are NOT inside [a,end).
         * That is: unpoisoned = intersection of block with complement of [a,end).
         * Simpler to compute unpoisoned_prefix = max(0, min(block_end, a) - block_start)
         * but if a <= block_start then prefix could be 0. We'll compute how many
         * lowest bytes remain unpoisoned. */
        uintptr_t block_end = block_start + 8;
        /* number of lowest bytes in block not covered by poison range */
        uintptr_t unpoison_lo = block_start;
        uintptr_t unpoison_hi = (a > block_start) ? ((a < block_end) ? a : block_end) : block_start;
        unsigned prefix = 0;
        if (unpoison_hi > unpoison_lo) prefix = (unsigned)(unpoison_hi - block_start);
        /* If the poison range doesn't start within the block, but ends within it,
         * this logic still works because we only track prefix. */
        asan_shadow[i] = prefix; /* 0..8 */
    }
}

void __asan_unpoison_memory_region(void *addr, size_t size)
{
    if (!asan_region_base || size == 0) return;
    uintptr_t a = (uintptr_t)addr;
    uintptr_t end = a + size;
    uintptr_t base = (uintptr_t)asan_region_base;

    /* Clamp to watched region */
    if (a < base) a = base;
    if (end > base + asan_region_size) end = base + asan_region_size;
    if (a >= end) return;

    size_t start_idx = (a - base) >> 3;
    size_t end_idx = ((end - base) + 7) >> 3; /* ceil */
    if (start_idx >= asan_shadow_len) return;
    if (end_idx > asan_shadow_len) end_idx = asan_shadow_len;

    for (size_t i = start_idx; i < end_idx; ++i) {
        uintptr_t block_start = base + (i << 3);
        uintptr_t block_end = block_start + 8;
        /* If the unpoison covers the whole block, mark 8; otherwise compute
         * the number of lowest bytes that remain unpoisoned after this operation.
         * For simplicity we will set the prefix to the maximum available prefix
         * given the unpoison range starting position. */
        if (a <= block_start && end >= block_end) {
            asan_shadow[i] = 8;
        } else {
            /* compute how many bytes from block start are unpoisoned after
             * marking [a,end) unpoisoned. That is intersection of [block_start,block_end)
             * with [block_start, end) clipped by a. We conservatively set prefix
             * to the max contiguous prefix available given current op. */
            uintptr_t lo = (a > block_start) ? a : block_start;
            uintptr_t hi = (end < block_end) ? end : block_end;
            unsigned prefix = 0;
            if (hi > block_start) prefix = (unsigned)(hi - block_start);
            if (prefix > 8) prefix = 8;
            asan_shadow[i] = prefix;
        }
    }
}

/* Check access of `size` bytes at `addr`. Returns 0 if OK, 1 if access
 * overlaps a poisoned byte. */
static int asan_check_access(uintptr_t addr, size_t size)
{
    if (!asan_region_base) return 0;
    uintptr_t base = (uintptr_t)asan_region_base;
    uintptr_t end = addr + size;
    if (addr < base || addr >= base + asan_region_size) return 0; /* outside watched */

    size_t idx = (addr - base) >> 3;
    unsigned off = (unsigned)((addr - base) & 7);

    while (addr < end) {
        if (idx >= asan_shadow_len) break;
        uint8_t s = asan_shadow[idx]; /* 0..8 */
        if (s == 8) {
            /* whole block available from offset 0..7 */
            size_t can = 8 - off;
            size_t step = (end - addr) < can ? (end - addr) : can;
            addr += step;
            idx++;
            off = 0;
            continue;
        }
        /* s is number of lowest accessible bytes in block */
        if (off >= s) return 1; /* first accessed byte is poisoned */
        /* the maximum bytes we can read in this block */
        size_t can = s - off;
        size_t step = (end - addr) < can ? (end - addr) : can;
        addr += step;
        if (addr >= end) return 0;
        idx++;
        off = 0;
    }
    return 0;
}

static void __asan_report_error_common(const char *type, uintptr_t addr, size_t size)
{
    asan_write("ASAN: ");
    asan_write(type);
    asan_write(" at 0x");
    asan_write_hex((unsigned long)addr);
    asan_write(" size=");
    asan_write_hex((unsigned long)size);
    asan_panic();
}

/* Report helpers for fixed-size accesses. */
void __asan_report_load1(void *addr) { if (asan_check_access((uintptr_t)addr, 1)) __asan_report_error_common("load1", (uintptr_t)addr, 1); }
void __asan_report_load2(void *addr) { if (asan_check_access((uintptr_t)addr, 2)) __asan_report_error_common("load2", (uintptr_t)addr, 2); }
void __asan_report_load4(void *addr) { if (asan_check_access((uintptr_t)addr, 4)) __asan_report_error_common("load4", (uintptr_t)addr, 4); }
void __asan_report_load8(void *addr) { if (asan_check_access((uintptr_t)addr, 8)) __asan_report_error_common("load8", (uintptr_t)addr, 8); }
void __asan_report_load16(void *addr) { if (asan_check_access((uintptr_t)addr, 16)) __asan_report_error_common("load16", (uintptr_t)addr, 16); }

void __asan_report_store1(void *addr) { if (asan_check_access((uintptr_t)addr, 1)) __asan_report_error_common("store1", (uintptr_t)addr, 1); }
void __asan_report_store2(void *addr) { if (asan_check_access((uintptr_t)addr, 2)) __asan_report_error_common("store2", (uintptr_t)addr, 2); }
void __asan_report_store4(void *addr) { if (asan_check_access((uintptr_t)addr, 4)) __asan_report_error_common("store4", (uintptr_t)addr, 4); }
void __asan_report_store8(void *addr) { if (asan_check_access((uintptr_t)addr, 8)) __asan_report_error_common("store8", (uintptr_t)addr, 8); }
void __asan_report_store16(void *addr) { if (asan_check_access((uintptr_t)addr, 16)) __asan_report_error_common("store16", (uintptr_t)addr, 16); }

/* Generic n-byte variants (the compiler may emit these). */
void __asan_report_load_n(size_t size, void *addr) { if (asan_check_access((uintptr_t)addr, size)) __asan_report_error_common("load_n", (uintptr_t)addr, size); }
void __asan_report_store_n(size_t size, void *addr) { if (asan_check_access((uintptr_t)addr, size)) __asan_report_error_common("store_n", (uintptr_t)addr, size); }

/* Outline check stubs: compilers may call these before loads/stores (tiny wrappers). */
void __asan_load1(void *addr)  { __asan_report_load1(addr); }
void __asan_load2(void *addr)  { __asan_report_load2(addr); }
void __asan_load4(void *addr)  { __asan_report_load4(addr); }
void __asan_load8(void *addr)  { __asan_report_load8(addr); }
void __asan_load16(void *addr) { __asan_report_load16(addr); }

void __asan_store1(void *addr)  { __asan_report_store1(addr); }
void __asan_store2(void *addr)  { __asan_report_store2(addr); }
void __asan_store4(void *addr)  { __asan_report_store4(addr); }
void __asan_store8(void *addr)  { __asan_report_store8(addr); }
void __asan_store16(void *addr) { __asan_report_store16(addr); }

/* Registration stubs for globals (no-op in this minimal runtime). */
void __asan_register_globals(void *globals, size_t n) { (void)globals; (void)n; }
void __asan_unregister_globals(void *globals, size_t n) { (void)globals; (void)n; }

#ifdef __cplusplus
}
#endif
