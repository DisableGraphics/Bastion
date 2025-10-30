const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const ma = @import("multialloc.zig");

pub const PageTableAllocator = ma.MultiAlloc(page.page_table_type, true, 64, 
	&[_]type{page.pml5_t, page.pml4_t, page.pml3_t, page.pml2_t, page.huge_page_t, page.large_page_t, page.page_t});