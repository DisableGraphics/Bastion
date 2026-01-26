const slab = @import("slaballoc.zig");
const kmm = @import("kmm.zig");
const main = @import("../main.zig");
const page = @import("pagemanager.zig");

pub fn MultiAlloc(comptime inner: type, comptime needs_zeroed: bool, comptime minimum_expected_per_core: u64) type {
	return struct {
		var allocators: []slab.SlabAllocator(inner) = undefined;
		pub fn init(expected_ports: u64, mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
			const allocators_pages = ((mp_cores * @sizeOf(slab.SlabAllocator(inner))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			allocators = @as([*]slab.SlabAllocator(inner), @ptrFromInt((try km.alloc_virt(allocators_pages)).?))[0..mp_cores];

			// Force at least minimum_expected_per_core elements available per core
			const expected_elements_per_core = @max(minimum_expected_per_core, (expected_ports + mp_cores - 1) / mp_cores);
			for(0..mp_cores) |i| {
				const expected_elements_pages = ((expected_elements_per_core * @sizeOf(inner)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
				const expected_elements_bytes = expected_elements_pages * page.PAGE_SIZE;
				const buffer = @as([*]u8, @ptrFromInt((try km.alloc_virt(expected_elements_pages)).?))[0..expected_elements_bytes];
				allocators[i] = try slab.SlabAllocator(inner).init(buffer);
				try allocators[i].init_region();
			}
		}

		pub fn alloc() ?*inner {
			const myid = main.mycpuid();
			if(needs_zeroed) {
				const buf = allocators[myid].alloc();
				if(buf == null) return null;
				const inner_buf_type = @typeInfo(@TypeOf(buf.?.*)).array.child;
				const memset_with_zero = if(@typeInfo(inner_buf_type) == .optional) false else true;
				if(memset_with_zero) {
					@memset(buf.?, 0);
				} else {
					@memset(buf.?, null);
				}
				return buf;
			} else {
				return allocators[myid].alloc();
			}
		}

		pub fn free(ts: *inner) !void {
			const myid = main.mycpuid();
			return allocators[myid].free(ts);
		}

		pub fn get_cpu_owner(ts: *inner) ?u64 {
			for(0..allocators.len) |i| {
				if(allocators[i].allocated_from_this_slab(ts)) return i;
			}
			return null;
		}
	};
}