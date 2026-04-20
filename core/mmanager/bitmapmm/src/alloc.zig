const limine = @import("limine");
const std = @import("std");
const spin = @import("spin.zig");

pub const bitmap_t = usize;
pub const physaddr_t = usize;
pub const virtaddr_t = usize;

pub const PageFrameAllocator = struct {
	bitmap: []bitmap_t,
	physical_n_pages: usize,
	cache: usize = 0,
	start_addr: usize,
	lock: spin.SpinLock,

	pub fn init(bitmap: []bitmap_t, start_addr: usize, physical_n_pages: usize) !PageFrameAllocator {
		const self: PageFrameAllocator = .{
			.physical_n_pages = physical_n_pages,
			.start_addr = start_addr,
			.bitmap = bitmap,
			.cache = 0,
			.lock = spin.SpinLock.init()
		};
		return self;
	}

	fn addr_to_offset(addr: usize) usize {
		return (addr) >> 12;
	}

	inline fn is_entry_active(self: *PageFrameAllocator, offset: u64) !bool {
		if(offset >= self.physical_n_pages) return error.OUT_OF_RANGE;
		const bitmap_displ = offset / @typeInfo(bitmap_t).int.bits;
		const bitmap_mask = offset % @typeInfo(bitmap_t).int.bits; 
		return self.bitmap[bitmap_displ] & (@as(bitmap_t, 1) << @truncate(bitmap_mask)) != 0;
	}

	pub inline fn set_entry(self: *PageFrameAllocator, offset: u64, val: u1) !void {
		if (offset >= self.physical_n_pages) return error.OUT_OF_RANGE;

		const bits = @typeInfo(bitmap_t).int.bits;
		const idx = offset / bits;
		const bit: u6 = @truncate(offset % bits);

		const mask = @as(bitmap_t, 1) << bit;

		if (val == 1) { // set
			self.bitmap[idx] |= mask;
		} else {
			self.bitmap[idx] &= ~mask;
		}

	}

	pub fn alloc(self: *PageFrameAllocator, npages: usize) !physaddr_t {
		self.lock.lock();
		defer self.lock.unlock();
		var consecutive_entries: usize = 0;
		var start_addr: usize = 0;
		// Find free entries
		for(self.cache..self.physical_n_pages) |i| {
			if(!(try is_entry_active(self, i))) {
				// Get start address
				if(start_addr == 0) start_addr = (i << 12);
				consecutive_entries+= 1;
				// If we have enough, allocate them
				if(consecutive_entries == npages) break;
			} else {
				// Reset the search
				consecutive_entries = 0;
				start_addr = 0;
			}
		}
		// If we cannot satisfy the allocation, return null
		if(consecutive_entries < npages) return error.NOT_ENOUGH_MEMORY; 
		// Set entries to used
		for(0..consecutive_entries) |i| {
			// As i is a page offset, we need to do i << 12 to obtain the physical page address
			const addr = start_addr + (i << 12);
			// Set the entry as allocated
			try set_entry(self, addr_to_offset(addr), 1);
		}
		self.cache = addr_to_offset(start_addr) + consecutive_entries;
		// Return start of physical memory region
		return self.start_addr + start_addr;
	}

	pub fn dealloc(self: *PageFrameAllocator, begin_addr: usize, npages: usize) !void {
		self.lock.lock();
		defer self.lock.unlock();
		// Set entries as deallocated
		for(0..npages) |i| {
			// Again, i << 12 means address of page frame number i
			const physaddr = (begin_addr - self.start_addr) + (i << 12);
			// Set it as free
			try set_entry(self, addr_to_offset(physaddr), 0);
		}
		self.cache = addr_to_offset(begin_addr - self.start_addr);
	}

	pub fn is_in_use(self: *PageFrameAllocator, address: usize) !bool {
		const offset = addr_to_offset(address);
		return self.is_entry_active(offset);
	}
};