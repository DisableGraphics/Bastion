const kmm = @import("../memory/kmm.zig");
const slab = @import("../memory/slaballoc.zig");
const tim = @import("timerman.zig");
const spn = @import("../sync/spinlock.zig");

const MAX_TIMERS = 16384;

pub const TimerAllocator = struct {
	var allocator: slab.SlabAllocator(tim.Timer) = undefined;
	var buffer: [MAX_TIMERS * @sizeOf(tim.Timer)]u8 align(@alignOf(tim.Timer)) = undefined;
	var spin: spn.SpinLock = undefined;

	// Only BSP is allowed to call init
	pub fn init() !void {
		spin = spn.SpinLock.init();
		@memset(&buffer, 0);
		allocator = try slab.SlabAllocator(tim.Timer).init(&buffer);
		try allocator.init_region();
	}

	pub fn alloc() ?*tim.Timer {
		spin.lock();
		defer spin.unlock();
		return allocator.alloc();
	}

	pub fn free(timer: *tim.Timer) !void {
		spin.lock();
		defer spin.unlock();
		try allocator.free(timer);
	}
};