const std = @import("std");
const page = @import("../memory/pagemanager.zig");
const pa = @import("../memory/pagetablealloc.zig");
const main = @import("../main.zig");
const pp = @import("physicalpage.zig");

pub const AddressSpace = struct{
	cr3: ?*page.page_table_type,
	refcount: std.atomic.Value(u32),
	pageman: *page.PageManager,
	mycpu: u32,

	pub fn init(pman: *page.PageManager) @This() {
		return .{
			.cr3 = null,
			.refcount = std.atomic.Value(u32).init(0),
			.pageman = pman,
			.mycpu = @truncate(main.mycpuid())
		};
	}

	fn fallback_on_page_table_allocation(tables: [5]?*page.page_table_type, level: usize) !void {
		for(level..5) |j| {
			if(tables[j]) |ptr| {
				try pa.PageTableAllocator.free(ptr);
			}
		}
	}

	fn add_page_tables(self: *AddressSpace, virtaddr: usize, level: usize) !void {
		var tables = [_]?*page.page_table_type{null} ** 5;
		for(level..5) |i| {
			const table = pa.PageTableAllocator.alloc() orelse {
				try fallback_on_page_table_allocation(tables, level);
				return error.NO_SPACE_LEFT;
			};
			tables[i] = table;
			const table_physaddr: *page.page_table_type = @ptrFromInt(@intFromPtr(table) - self.pageman.hhdm_offset);
			_ = switch(i) {
				0 => self.pageman.add_pml5(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x01FF_FFFF_FFFF_FFFF)),
				1 => self.pageman.add_pml4(self.cr3.?, table_physaddr, virtaddr &  ~@as(u64, 0x0000_FFFF_FFFF_FFFF)),
				2 => self.pageman.add_pml3(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_007F_FFFF_FFFF)),
				3 => self.pageman.add_pml2(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_0000_3FFF_FFFF)),
				4 => self.pageman.add_pml1(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_0000_001F_FFFF)),
				else => {}
			} catch {
				try fallback_on_page_table_allocation(tables, level);
				return error.NO_SPACE_LEFT;
			};
		}
	}

	pub fn add_mapping_4k(self: *AddressSpace, 
		physaddr: ?usize,
		virtaddr: usize,
		opt1: page.page_t,
		opt2: page.pml2_t,
		opt3: page.pml3_t,
		opt4: page.pml4_t
	) !void {
		if(self.cr3 == null) {
			self.cr3 = pa.PageTableAllocator.alloc() orelse return error.NO_SPACE_LEFT;
		}
		const level = self.pageman.last_map_level(self.cr3.?, virtaddr);
		if(level < 5) {
			try self.add_page_tables(virtaddr, level);
		}
		try self.pageman.map_4k_cascade(self.cr3.?, physaddr, virtaddr, opt1, opt2, opt3, opt4);
	}

	pub fn remove_mapping_4k(self: *AddressSpace, virtaddr: usize) !void {
		if(self.cr3 == null) return;
		try self.pageman.unmap(self.cr3.?, virtaddr);
	}

	pub fn get_physaddr(self: *AddressSpace, virtaddr: usize) ?usize {
		if(!self.pageman.is_mapped(self.cr3, virtaddr)) return null;
		return self.pageman.get_physaddr(virtaddr) catch null;
	}

	pub fn close(self: *AddressSpace) !void {
		std.debug.assert(self.refcount.load(.acq_rel) == 0);

		const root_page: ?*page.page_table_type = @ptrFromInt(@intFromPtr(self.cr3) + main.km.hhdm_offset);
		const it = try self.pageman.iterator(root_page.?);
		while(it.next()) |p| {
			const addr = try self.pageman.get_addr_from_entry(*p);
			const entry = pp.PhysicalPageManager.get(addr) orelse continue;
			const nentries = entry.refcount.fetchSub(1, .acq_rel);
			if(nentries == 1) {
				// TODO: send to the page manager
			}
		}
	}
};