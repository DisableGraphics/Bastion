const slab = @import("../memory/slaballoc.zig");
const tsk = @import("task.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");
const ma = @import("../memory/multialloc.zig");

pub const TaskAllocator = ma.MultiAlloc(tsk.Task, false, 64);