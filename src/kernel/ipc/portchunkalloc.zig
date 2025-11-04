const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const port = @import("port.zig");
const ma = @import("../memory/multialloc.zig");

pub const port_chunk = [256]?*port.Port;

pub const PortChunkAllocator = ma.MultiAlloc(port_chunk, true, 6, &[_]type{});