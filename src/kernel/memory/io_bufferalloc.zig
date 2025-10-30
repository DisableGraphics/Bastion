const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const tss = @import("tss.zig");
const ma = @import("multialloc.zig");

pub const IOBufferAllocator = ma.MultiAlloc(tss.io_bitmap_t, true, 6, &[_]type{});