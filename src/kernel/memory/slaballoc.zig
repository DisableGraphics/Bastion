const slaberr = error {
	OUT_OF_MEMORY,
	OUT_OF_RANGE
};

const free_list_node = struct {
	next: ?*free_list_node
};

pub fn SlabAllocator(comptime T: type) type {
	return struct {
		region: []u8,
		free_list: ?*free_list_node,
		size: usize,

		pub fn init(region: []u8) !SlabAllocator(T) {
			var size: usize = @sizeOf(T);
			if(@sizeOf(T) < @sizeOf(free_list_node)) {
				size = @sizeOf(free_list_node);
			}
			return .{.region = region, .free_list = null, .size = size};
		}

		pub fn init_region(self: *SlabAllocator(T)) !void {
			const region_size = self.region.len;
			const n_elements = region_size / self.size;
			for(0..n_elements-1) |i| {
				const next_addr: [*]u8 = self.region.ptr + (i+1)*self.size;
				const current_addr: [*]u8 = self.region.ptr + i*self.size;

				const current: *free_list_node = @ptrCast(@alignCast(current_addr));
				const next: *free_list_node = @ptrCast(@alignCast(next_addr));
				current.next = next;
				if(i == 0) {
					self.free_list = current;
				}
			}
			const current_addr: [*]u8 = self.region.ptr + (n_elements - 1)*self.size;
			const current: *free_list_node = @ptrCast(@alignCast(current_addr));
			current.next = null;
		}

		pub fn alloc(self: *SlabAllocator(T)) ?*T {
			if(self.free_list) |free_node| {
				const addr: *T = @ptrCast(free_node);
				self.free_list = free_node.next;
				return addr;
			} else {
				return null;
			}
		}

		pub fn free(self: *SlabAllocator(T), obj: *T) !void {
			if(!self.allocated_from_this_slab(obj)) return slaberr.OUT_OF_RANGE;
			const node: *free_list_node = @ptrCast(@alignCast(obj));
			node.next = self.free_list;
			self.free_list = node;
		}
		pub fn allocated_from_this_slab(self: *SlabAllocator(T), obj: *T) bool {
			if(@intFromPtr(obj) < @intFromPtr(@as(*T, @ptrCast(@alignCast(self.region.ptr)))) 
				or @intFromPtr(obj) >= @intFromPtr(@as(*T, @ptrCast(@alignCast(self.region.ptr + self.region.len))))) {
				return false;
			}
			return true;
		}
	};
}