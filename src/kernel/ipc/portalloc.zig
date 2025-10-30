const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const port = @import("port.zig");
const ma = @import("../memory/multialloc.zig");

pub const PortAllocator = ma.MultiAlloc(port.Port, false, 128, &[_]type{});