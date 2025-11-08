const kmm = @import("kmm.zig");
const page = @import("pagemanager.zig");

pub const LapicCPUIDCache = struct {
	var cache: []u32 = undefined;
	pub fn init(mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const pages_cpuidcache = ((mp_cores * @sizeOf(u32)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		const ptr = (try km.alloc_virt(pages_cpuidcache)).?;
		cache = @as([*]u32, @ptrFromInt(ptr))[0..mp_cores];
		for(0..mp_cores) |i| {
			cache[i] = @truncate(i);
		}
	}

	pub inline fn get_base() u64 {
		return @intFromPtr(cache.ptr);
	}

	pub inline fn get_gs(mycpu: u64) u64 {
		return @intFromPtr(&cache[mycpu]);
	}
};