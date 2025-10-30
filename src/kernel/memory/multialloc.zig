const slab = @import("slaballoc.zig");
const kmm = @import("kmm.zig");
const main = @import("../main.zig");
const page = @import("pagemanager.zig");

pub fn MultiAlloc(comptime inner: type, comptime needs_zeroed: bool, comptime minimum_expected_per_core: u64, comptime cast_types: []const type) type {
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
				@memset(buf.?, 0);
				return buf;
			} else {
				return allocators[myid].alloc();
			}
		}

		pub fn free(ts: *inner) !void {
			const myid = main.mycpuid();
			return allocators[myid].free(ts);
		}

		pub fn alloc_as(comptime T: type) ?*T {
			if (cast_types.len != 0) {
				comptime {
					var ok = false;
					for (cast_types) |A| {
						if (A == T) {
							ok = true;
							break;
						}
					}
					if (!ok) @compileError("alloc_as: requested type is not in allowed casts");
				}
				return @ptrCast(alloc());
			}
			@compileError("A whitelist must be provided in order to use alloc_as");
		}

		pub fn free_as(comptime T: type, ts: *T) !void {
			if (cast_types.len != 0) {
				comptime {
					var ok = false;
					for (cast_types) |A| {
						if (A == T) {
							ok = true;
							break;
						}
					}
					if (!ok) @compileError("free_as: requested type is not in allowed casts");
				}
				return free(@as(*inner, @ptrCast(ts)));
			}
			@compileError("A whitelist must be provided in order to use free_as");
		}
	};
}