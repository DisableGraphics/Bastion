const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const ma = @import("multialloc.zig");

pub const KernelStack = [4*page.PAGE_SIZE]u8;

pub const KernelStackAllocator = ma.MultiAlloc(KernelStack, true, 64);