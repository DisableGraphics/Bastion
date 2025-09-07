const std = @import("std");
const framealloc = @import("physicalalloc.zig");
const page = @import("pagemanager.zig");
const spin = @import("../sync/spinlock.zig");
const limine = @import("limine");

const kernel_tables = struct{
	pml3_kernel: *page.page_table_type,
	pml2_kernel: *page.page_table_type,
	pml1_kernel: []page.page_table_type
};


pub const KernelMemoryManager = struct {
	hhdm_offset: usize,
	kernel_offset: usize,
	kernel_size: usize,
	pfa_lock: spin.SpinLock,
	pfa: *framealloc.PageFrameAllocator,
	page_lock: spin.SpinLock,
	pm: *page.PageManager,
	finished_setup: std.atomic.Value(bool),

	pub fn init(fralloc: *framealloc.PageFrameAllocator,
		pageman: *page.PageManager,
		hhdm_offset: usize,
		kernel_offset: usize,
		kernel_size: usize) !KernelMemoryManager {
		
		return .{
			.hhdm_offset = hhdm_offset,
			.kernel_offset = kernel_offset,
			.kernel_size = kernel_size,
			.pfa_lock = spin.SpinLock.init(),
			.pfa = fralloc,
			.page_lock = spin.SpinLock.init(),
			.pm = pageman,
			.finished_setup = std.atomic.Value(bool).init(false),
		};
	}

	// ------------------------------------ Helpers --------------------------------------------

	pub fn alloc_virt(self: *KernelMemoryManager, npages: usize) !?framealloc.virtaddr_t {
		const val: usize = value: {
			self.pfa_lock.lock();
			defer self.pfa_lock.unlock();
			const c = (try self.pfa.alloc(npages)) orelse {
				self.pfa_lock.unlock();
				return error.OUT_OF_MEMORY;
			};
			break :value c;
		};
		
		if(self.finished_setup.load(.acquire)) {
			self.page_lock.lock();
			defer self.page_lock.unlock();
			for(0..npages) |i| {
				try self.pm.map_4k(self.pm.root_table.?, val + (i << 12), val + self.hhdm_offset + (i << 12));
			}
		}

		@memset(@as([*]u8, @ptrFromInt(val+self.hhdm_offset))[0..npages*page.PAGE_SIZE], 0);
		
		return val + self.hhdm_offset;
	}

	pub fn alloc_phys(self: *KernelMemoryManager, npages: usize) !?framealloc.virtaddr_t {
		self.pfa_lock.lock();
		defer self.pfa_lock.unlock();
		return self.pfa.alloc(npages);
	}

	pub fn dealloc_virt(self: *KernelMemoryManager, begin_addr: usize, npages: usize) !void {
		{
			self.pfa_lock.lock();
			defer self.pfa_lock.unlock();
			try self.pfa.dealloc(begin_addr - self.hhdm_offset, npages);
		}
		if(self.finished_setup.load(.acquire)) {
			self.page_lock.lock();
			defer self.page_lock.unlock();
			for(0..npages) |i| {
				try self.pm.unmap(self.pm.root_table.?, begin_addr + (i<<12));
			}
		}
	}

	pub fn dealloc_phys(self: *KernelMemoryManager, begin_addr: usize, npages: usize) !void {
		self.pfa_lock.lock();
		defer self.pfa_lock.unlock();
		return self.pfa.dealloc(begin_addr, npages);
	}

	pub fn dealloc_virt_noerr(self: *KernelMemoryManager, begin_addr: usize, npages: usize) void {
		self.dealloc_virt(begin_addr, npages) catch return;
	}

	pub fn dealloc_phys_noerr(self: *KernelMemoryManager, begin_addr: usize, npages: usize) void {
		self.dealloc_phys(begin_addr, npages) catch return;
	}

	fn truncate_to_page(addr: usize) usize {
		return addr & ~@as(usize, 0xFFF);
	}

	fn round_to_page(addr: usize) usize {
		return ((addr + 0xFFF) & ~@as(usize, 0xFFF));
	}

	// ------------------------------------ Frame allocator setup --------------------------------------------

	pub fn setup_frame_allocator(self: *KernelMemoryManager, map: *limine.MemoryMapResponse) !void {
		const bitmap_data = try get_bitmap_minimum_size(map);
		for(bitmap_data.map) |m| {
			std.log.info("{x} to {x} made the cut ({})", .{m.base, m.base + m.length, m.type});
		}
		
		const physical_n_pages = ((bitmap_data.len + (1 << 12) - 1) >> 12); // Number of pages is bytes rounded up to 4096
		const size_bitmap_bytes = (physical_n_pages + (1 << 3) - 1) >> 3; // (bytes)
		const bitmap_n_entries = (size_bitmap_bytes+@sizeOf(framealloc.bitmap_t)-1)/@sizeOf(framealloc.bitmap_t);
		self.pfa.bitmap_n_entries = bitmap_n_entries;
		self.pfa.physical_n_pages = physical_n_pages;
		self.pfa.bitmap = try self.find_first_bitmap_region(bitmap_data.map, size_bitmap_bytes, self.hhdm_offset);

		@memset(self.pfa.bitmap.?, std.math.maxInt(framealloc.bitmap_t));
		try self.setup_bitmap_regions(bitmap_data.map, self.hhdm_offset);
	}

	fn addr_to_offset(addr: usize) usize {
		return (addr) >> 12;
	}

	fn find_first_bitmap_region(self: *KernelMemoryManager, map: []*limine.MemoryMapEntry, size: usize, higher_half_offset: usize) ![]framealloc.bitmap_t {
		for(map) |entry| {
			if(entry.type == .usable and entry.length >= size) {
				std.log.info("Yes it is: 0x{x} is the new address of the bitmap", .{entry.base});
				return @as([*]framealloc.bitmap_t, @ptrFromInt(entry.base + higher_half_offset))[0..self.pfa.bitmap_n_entries];
			}
		}
		return error.NO_ENTRY_SUITABLE;
	}

	fn setup_bitmap_regions(self: *KernelMemoryManager, map: []*limine.MemoryMapEntry, higher_half_offset: usize) !void {
		for(map) |entry| {
			if(entry.type == .usable) {
				std.log.info("0x{x} - 0x{x} is usable.", .{entry.base, entry.base + entry.length});
				const region_n_pages = (entry.length + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
				for(0..region_n_pages) |i| {
					const off = addr_to_offset(entry.base + (i << 12));
					try self.pfa.set_entry(off, 0); // Set as free
				}
			}
		}
		// Setup all the bitmap as used pages
		const bitmap_pages = (self.pfa.bitmap_n_entries*@sizeOf(framealloc.bitmap_t)) / page.PAGE_SIZE;
		const bitmap_offset = addr_to_offset(@intFromPtr(self.pfa.bitmap.?.ptr) - higher_half_offset);
		for(0..bitmap_pages) |i| {
			const bitmap_offset_p = i + bitmap_offset;
			try self.pfa.set_entry(bitmap_offset_p, 1);
		}
	}

	fn get_bitmap_minimum_size(map: *limine.MemoryMapResponse) ! struct {len: usize, map: []*limine.MemoryMapEntry} {
		// Basically we iterate searching for the biggest continuous region.
		// I say continuous because some device drivers have MMIO at very high
		// addresses which will bloat the bitmap
		var prev_addr = map.getEntries().ptr[0].base + map.getEntries().ptr[0].length;
		var length = map.getEntries().ptr[0].length;
		var last: usize = 0;
		
		for(map.getEntries()[1..], 1..map.getEntries().len) |entry, i| {
			const entry_rounded = truncate_to_page(entry.base);
			const prev_rounded = truncate_to_page(prev_addr);
			std.log.info("{x} vs {x}", .{entry_rounded, prev_rounded});
			// Basically check that the distance between them is not so large that it will make the bitmap massive
			if(entry_rounded >= prev_rounded and prev_rounded + (1 << 30) >= entry_rounded) {
				length += truncate_to_page(entry_rounded - prev_rounded);
				prev_addr = truncate_to_page(entry.base) + round_to_page(entry.length);
				length += round_to_page(entry.length);
			} else {
				break;
			}
			last = i;
		}
		
		return .{.len = length, .map = map.entries.?[0..last]};
	}

	// ------------------------------------ Virtual Memory Setup --------------------------------------------
	pub fn setup_virtual_stage_one(self: *KernelMemoryManager, map: *limine.MemoryMapResponse) !void {
		self.pm.root_table = @ptrFromInt((try self.alloc_virt(1)).?);
		const kt = try self.setup_kernel();
		try self.setup_hhdm(kt, map);
	}

	fn setup_kernel(self: *KernelMemoryManager) !kernel_tables {
		const pml3_kernel: *page.page_table_type = @ptrFromInt((try self.alloc_phys(1)).?);
		const pml2_kernel: *page.page_table_type = @ptrFromInt((try self.alloc_phys(1)).?);

		const kernel_pml3_region = self.kernel_offset & ~@as(u64, 0x0000_007F_FFFF_FFFF);
		try self.pm.add_pml3(self.pm.root_table.?, pml3_kernel, kernel_pml3_region);

		const kernel_pml2_region = self.kernel_offset & ~@as(u64, 0x0000_0000_3FFF_FFFF);
		try self.pm.add_pml2(self.pm.root_table.?, pml2_kernel, kernel_pml2_region);

		
		const n_2m_regions = (self.kernel_size + (1 << 21) - 1) >> 21;
		const pml1_kernel: []page.page_table_type = @as([*]page.page_table_type, @ptrFromInt((try self.alloc_phys(n_2m_regions)).?))[0..n_2m_regions];
		for(pml1_kernel,0..pml1_kernel.len) |*pt1, i| {
			const region = kernel_pml2_region + (i << 21);
			try self.pm.add_pml1(self.pm.root_table.?, pt1, region);
		}
		var it = kernel_pml2_region;
		std.log.info("Kernel size: {}", .{self.kernel_size});
		const max_kernel_page = (kernel_pml2_region + self.kernel_size + (1 << 12) - 1) & ~@as(u64, (1 << 12) - 1);
		while(it < max_kernel_page) : (it += (1 << 12)) {
			const physical = try self.pm.get_physaddr(it);
			std.log.info("2m addr: {x}, physical: {x}", .{it, physical});
			try self.pm.map_4k(self.pm.root_table.?, physical, it);
		}
		return .{.pml3_kernel = pml3_kernel, .pml2_kernel = pml2_kernel, .pml1_kernel = pml1_kernel};
	}

	const pair = struct {
		lower_bound: u64,
		upper_bound: u64,
		pub fn format(
            self: pair,
            comptime fmt: []const u8,
            options: std.fmt.FormatOptions,
            writer: anytype,
        ) !void {
            _ = fmt;
            _ = options;
            try writer.print("pair{{ .lower_bound = 0x{X}, .upper_bound = 0x{X} }}", .{
                self.lower_bound,
                self.upper_bound,
            });
        }

	};

	fn get_n_regions(range: pair, size: u64) usize {
		const start = (range.lower_bound) & ~@as(u64, size - 1);
		const end = (range.upper_bound + size - 1) & ~@as(u64, size - 1);
		var it = start;
		var ret: u64 = 0;
		while(it < end) : (it += size) {
			ret += 1;
		}
		return ret;
	}

	fn get_page_table_pages(self: *KernelMemoryManager, map: *limine.MemoryMapResponse, allocator: std.mem.Allocator) !struct {
		pt3: std.ArrayList(pair),
		pt3_count: usize,
		pt2: std.ArrayList(pair),
		pt2_count: usize,
		pt1: std.ArrayList(pair),
		pt1_count: usize,
	} {
		var pt3_count: u64 = 0;
		var pt2_count: u64 = 0;
		var pt1_count: u64 = 0;

		var pt3_list = std.ArrayList(pair).init(allocator);
		var pt2_list = std.ArrayList(pair).init(allocator);
		var pt1_list = std.ArrayList(pair).init(allocator);

		var prev_3: u64 = 0;
		var prev_2: u64 = 0;
		var prev_1: u64 = 0;

		for(map.getEntries()) |region| {
			if(region.type != .bad_memory and region.type != .reserved) {

				const start = (self.hhdm_offset + region.base) & ~@as(u64, page.PAGE_SIZE - 1);
				const end = (self.hhdm_offset + region.base + region.length + page.PAGE_SIZE	 - 1) & ~@as(u64, page.PAGE_SIZE - 1);
				var it = start;
				while(it < end) : (it += page.PAGE_SIZE) {
					const pt3_disp = it >> 39;
					const pt2_disp = it >> 30;
					const pt1_disp = it >> 21;
					if(pt1_disp != prev_1) {
						if(prev_1 + 1 != pt1_disp) {
							// Not contiguous, adding a new range
							pt1_count += 1;
							try pt1_list.append(.{.lower_bound = pt1_disp << 21, .upper_bound = (pt1_disp + 1) << 21});
						} else {
							// Yes contiguous, making the current one larger
							pt1_list.items[pt1_count-1].upper_bound = ((pt1_disp + 1) << 21);
						}
						prev_1 = pt1_disp;
					}
					if(pt2_disp != prev_2) {
						if(prev_2 + 1 != pt2_disp) {
							// Not contiguous, adding a new range
							pt2_count += 1;
							try pt2_list.append(.{.lower_bound = pt2_disp << 30, .upper_bound = (pt2_disp + 1) << 30});
						} else {
							// Yes contiguous, making the current one larger
							pt2_list.items[pt2_count-1].upper_bound = ((pt2_disp + 1) << 30);
						}
						prev_2 = pt2_disp;
					}
					if(pt3_disp != prev_3) {
						if(prev_3 + 1 != pt3_disp) {
							// Not contiguous, adding a new range
							pt3_count += 1;
							try pt3_list.append(.{.lower_bound = pt3_disp << 39, .upper_bound = (pt3_disp + 1) << 39});
						} else {
							// Yes contiguous, making the current one larger
							pt3_list.items[pt3_count-1].upper_bound = ((pt3_disp + 1) << 39);
						}
						prev_3 = pt3_disp;
					}
				}
			}
		}

		var pt3_ntables: u64 = 0;
		for(pt3_list.items) |region| {
			pt3_ntables += get_n_regions(region, 1 << 39);
		}
		var pt2_ntables: u64 = 0;
		for(pt2_list.items) |region| {
			pt2_ntables += get_n_regions(region, 1 << 30);
		}
		var pt1_ntables: u64 = 0;
		for(pt1_list.items) |region| {
			pt1_ntables += get_n_regions(region, 1 << 21);
		}

		return .{
			.pt3 = pt3_list,
			.pt3_count = pt3_ntables,
			.pt2 = pt2_list,
			.pt2_count = pt2_ntables,
			.pt1 = pt1_list,
			.pt1_count = pt1_ntables
		};
	}

	fn get_region_offset(self: *KernelMemoryManager, regions_list: *const std.ArrayList(pair), region_step: usize, region: usize) ?usize {
		_ = self;
		var rs = region;
		for(regions_list.items) |rgr| {
			const start = (rgr.lower_bound) & ~@as(u64, region_step - 1);
			const end = (rgr.upper_bound + region_step - 1) & ~@as(u64, region_step - 1);
			
			var it = start;
			while(it < end) : (it += region_step) {
				if(rs == 0) return it;
				rs -= 1;
			}
			
		}
		return null;
	}

	fn setup_hhdm(self: *KernelMemoryManager, kt: kernel_tables, map: *limine.MemoryMapResponse) !void {
		const buffer_alloc_pages = 32;
		const buffer = (try self.alloc_virt(buffer_alloc_pages)).?;
		defer self.dealloc_virt(buffer, buffer_alloc_pages) catch {};

		const fixedbuffer: []u8 = @as([*]u8, @ptrFromInt(buffer))[0..buffer_alloc_pages*page.PAGE_SIZE];
		var fba = std.heap.FixedBufferAllocator.init(fixedbuffer);
		const allocator = fba.allocator();

		const n_page_tables = try get_page_table_pages(self, map, allocator);
		defer n_page_tables.pt1.deinit();
		defer n_page_tables.pt2.deinit();
		defer n_page_tables.pt3.deinit();

		std.log.info("{any} ({} tables)\n {any} ({} tables)\n {any} ({} tables)", 
			.{
				n_page_tables.pt1.items,
				n_page_tables.pt1_count,
				n_page_tables.pt2.items,
				n_page_tables.pt2_count,
				n_page_tables.pt3.items,
				n_page_tables.pt3_count,
		});
		const pt3_array: []page.page_table_type = @as([*]page.page_table_type, @ptrFromInt((try self.alloc_virt(n_page_tables.pt3_count)).?))[0..n_page_tables.pt3_count];
		const pt2_array: []page.page_table_type = @as([*]page.page_table_type, @ptrFromInt((try self.alloc_virt(n_page_tables.pt2_count)).?))[0..n_page_tables.pt2_count];
		const pt1_array: []page.page_table_type = @as([*]page.page_table_type, @ptrFromInt((try self.alloc_virt(n_page_tables.pt1_count)).?))[0..n_page_tables.pt1_count];
		var reg_counter: usize = 0;
		for(pt3_array) |*pt3_virttable| {
			const pt3_physaddr: *page.page_table_type = @ptrFromInt(@intFromPtr(pt3_virttable) - self.hhdm_offset);
			const region = self.get_region_offset(&n_page_tables.pt3, (1 << 39), reg_counter).?;
			try self.pm.add_pml3(self.pm.root_table.?, pt3_physaddr, region);
			reg_counter += 1;
		}

		reg_counter = 0;
		for(pt2_array) |*pt2_virttable| {
			const pt2_physaddr: *page.page_table_type = @ptrFromInt(@intFromPtr(pt2_virttable) - self.hhdm_offset);
			const region = self.get_region_offset(&n_page_tables.pt2, (1 << 30), reg_counter).?;
			try self.pm.add_pml2(self.pm.root_table.?, pt2_physaddr, region);
			reg_counter += 1;
		}

		reg_counter = 0;
		for(pt1_array) |*pt1_virttable| {
			const pt1_physaddr: *page.page_table_type = @ptrFromInt(@intFromPtr(pt1_virttable) - self.hhdm_offset);
			const region = self.get_region_offset(&n_page_tables.pt1, (1 << 21), reg_counter).?;
			try self.pm.add_pml1(self.pm.root_table.?, pt1_physaddr, region);
			reg_counter += 1;
		}		
		std.log.info("Finished adding mappings for all memory regions", .{});

		std.log.info("Mapping page tables", .{});

		for(pt3_array) |*pt3_virttable| {
			const pt3_physaddr:usize = @intFromPtr(pt3_virttable) - self.hhdm_offset;
			try self.pm.map_4k(self.pm.root_table.?, pt3_physaddr, @intFromPtr(pt3_virttable));
		}
		for(pt2_array) |*pt2_virttable| {
			const pt2_physaddr:usize = @intFromPtr(pt2_virttable) - self.hhdm_offset;
			try self.pm.map_4k(self.pm.root_table.?, pt2_physaddr, @intFromPtr(pt2_virttable));
		}
		for(pt1_array) |*pt1_virttable| {
			const pt1_physaddr:usize = @intFromPtr(pt1_virttable) - self.hhdm_offset;
			try self.pm.map_4k(self.pm.root_table.?, pt1_physaddr, @intFromPtr(pt1_virttable));
		}
		std.log.info("Mapped page tables", .{});

		std.log.info("Mapping buffer", .{});
		for(0..buffer_alloc_pages) |i| {
			const virtaddr = @intFromPtr(fixedbuffer.ptr) + (i << 12);
			const physaddr = virtaddr - self.hhdm_offset;
			try self.pm.map_4k(self.pm.root_table.?, physaddr, virtaddr);
		}
		std.log.info("Mapped buffer", .{});

		std.log.info("Mapping all necessary memory regions", .{});
		for(map.getEntries()) |entry| {
			if(entry.type != .usable and entry.type != .reserved and entry.type != .bad_memory) {
				const start_page = entry.base & ~@as(u64, page.PAGE_SIZE - 1);
				const end_page = (entry.base + entry.length + page.PAGE_SIZE - 1) & ~@as(u64, page.PAGE_SIZE - 1);

				var it = start_page;
				while(it < end_page) : (it += page.PAGE_SIZE) {
					const physaddr = it;
					const virtaddr = it + self.hhdm_offset;
					try self.pm.map_4k(self.pm.root_table.?, physaddr, virtaddr);
				}
			}
		}
		std.log.info("Mapped all necessary memory regions", .{});
		std.log.info("Mapping page tables and bitmap", .{});

		try self.pm.map_4k(self.pm.root_table.?, @intFromPtr(self.pm.root_table.?) - self.hhdm_offset, @intFromPtr(self.pm.root_table.?));
		try self.pm.map_4k(self.pm.root_table.?, @intFromPtr(kt.pml3_kernel), @intFromPtr(kt.pml3_kernel) + self.hhdm_offset);
		try self.pm.map_4k(self.pm.root_table.?, @intFromPtr(kt.pml2_kernel), @intFromPtr(kt.pml2_kernel) + self.hhdm_offset);

		const bitmap_size_pages =  (self.pfa.bitmap_n_entries*@sizeOf(framealloc.bitmap_t)) / page.PAGE_SIZE;
		for(0..bitmap_size_pages) |i| {
			const virtaddr = @intFromPtr(self.pfa.bitmap.?.ptr) + (i << 12);
			const physaddr = virtaddr - self.hhdm_offset;
			try self.pm.map_4k(self.pm.root_table.?, physaddr, virtaddr);
		}

		std.log.info("Mapped page tables and bitmap", .{});
		std.log.info("Kernel start: 0x{x}, kernel end: 0x{x}", .{self.kernel_offset, self.kernel_offset + self.kernel_size});
		const pt_phys_addr = try self.pm.get_physaddr(@intFromPtr(self.pm.root_table.?));

		// Do a temporary mapping
		const tmp_mapping: *page.page_table_type = @ptrFromInt((try self.alloc_virt(1)) orelse return error.OUT_OF_MEMORY);
		defer self.dealloc_virt_noerr(@intFromPtr(tmp_mapping), 1);
		try self.pm.add_pml3(self.pm.root_table.?, @ptrFromInt(@intFromPtr(tmp_mapping) - self.hhdm_offset), 0);
		try self.pm.map_1g(self.pm.root_table.?, 0, 0);
		try self.pm.map_4k(self.pm.root_table.?, @intFromPtr(tmp_mapping) - self.hhdm_offset, @intFromPtr(tmp_mapping));

		page.set_cr3(pt_phys_addr);
		self.pm.delete_entry(self.pm.root_table.?, 0);
		std.log.info("Tables changed", .{});

		self.finished_setup.store(true, .release);
	}
};