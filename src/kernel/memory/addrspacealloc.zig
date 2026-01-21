const as = @import("addrspace.zig");
const ma = @import("multialloc.zig");

pub const AddressSpaceAllocator = ma.MultiAlloc(as.AddressSpace, false, 1024, 
	&[_]type{}); 