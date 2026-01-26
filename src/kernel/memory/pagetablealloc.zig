const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const ma = @import("multialloc.zig");

pub const PageTableAllocator = ma.MultiAlloc(page.page_table_type, true, 64);