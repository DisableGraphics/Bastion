const PRESENT = 1;
const READ_WRITE = 1 << 1;
const USER_SUPERVISOR = 1 << 2;
const WRITE_THROUGH = 1 << 3;
const PCD = 1 << 4;
const ACCESSED = 1 << 5;
const PS = 1 << 7;
const limine = @import("limine");
const log = @import("../log.zig");
const assembly = @import("../arch/x86_64/asm.zig");
const frame = @import("physicalalloc.zig");
const std = @import("std");

const pml5_t = packed struct {
	p: u1,
	rw: u1,
	us: u1,
	pwt: u1,
	pcd: u1,
	a: u1,
	avl1: u1,
	rsvd: u1,
	avl2: u4,
	addr: u40,
	avl3: u11,
	xd: u1,
};

const pml4_t = pml5_t;
const pml3_t = packed struct {
	p: u1,
	rw: u1,
	us: u1,
	pwt: u1,
	pcd: u1,
	a: u1,
	avl1: u1,
	ps: u1,
	avl2: u4,
	addr: u40,
	avl3: u11,
	xd: u1,
};
const pml2_t = pml3_t;
const huge_page_t = packed struct {
	p: u1,
	rw: u1,
	us: u1,
	pwt: u1,
	pcd: u1,
	a: u1,
	d: u1,
	ps: u1,
	g: u1,
	avl1: u3,
	pat: u1,
	resvd: u17,
	addr: u22,
	avl3: u7,
	pk: u4,
	xd: u1,
};
const large_page_t = packed struct {
	p: u1,
	rw: u1,
	us: u1,
	pwt: u1,
	pcd: u1,
	a: u1,
	d: u1,
	ps: u1,
	g: u1,
	avl1: u3,
	pat: u1,
	resvd: u8,
	addr: u31,
	avl3: u7,
	pk: u4,
	xd: u1,
};
const page_t = packed struct {
	p: u1,
	rw: u1,
	us: u1,
	pwt: u1,
	pcd: u1,
	a: u1,
	d: u1,
	pat: u1,
	g: u1,
	avl1: u3,
	addr: u40,
	avl3: u7,
	pk: u4,
	xd: u1,
};

pub const page_table_type = [512]u64;

pub const PagingError = error {
	BAD_ALIGN,
	PATH_TO_TABLE_DOES_NOT_EXIST,
	RESERVED_BITS_SET
};
pub const PAGE_SIZE = 4096;

pub extern fn set_cr3(physaddr_root_table: usize) callconv(.C) void; 

pub const PageManager = struct {
	root_table: ?*page_table_type,
	n_levels: usize,
	hhdm_offset: usize,

	pub fn init(n_levels: usize, hhdm_offset: usize) !PageManager {
		return .{
			.root_table = null, 
			.n_levels = n_levels,
			.hhdm_offset = hhdm_offset
		};
	}

	fn flush_virtaddr(virtaddr: usize) void {
		asm volatile(
			\\invlpg  [virtaddr]
			:
			: [virtaddr] "m"(virtaddr)
		);
	}

	pub fn get_physaddr(self: *PageManager, virtaddr: usize) !usize {
		const root_table_addr: *page_table_type = @ptrFromInt(assembly.read_cr3() + self.hhdm_offset);

		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		const virtaddr_pml1 = (virtaddr >> 12) & 0x1ff;

		const pml4 = try self.get_ptl4(root_table_addr, virtaddr);
		const pml4_entry: pml4_t = @bitCast(pml4[virtaddr_pml4]);
		
		const pml3: *page_table_type = @ptrFromInt((try get_addr_from_entry(pml4_entry)) + self.hhdm_offset);
		const pml3_entry: pml3_t = @bitCast(pml3[virtaddr_pml3]);
		if(pml3_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		if(pml3_entry.ps == 1) return try get_addr_from_entry(@as(huge_page_t, @bitCast(pml3_entry)));

		const pml2: *page_table_type =@ptrFromInt((try get_addr_from_entry(pml3_entry)) + self.hhdm_offset);
		const pml2_entry: pml3_t = @bitCast(pml2[virtaddr_pml2]);
		if(pml2_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		if(pml2_entry.ps == 1) return try get_addr_from_entry(@as(large_page_t, @bitCast(pml2_entry)));

		const pml1: *page_table_type = @ptrFromInt((try get_addr_from_entry(pml2_entry)) + self.hhdm_offset);
		const pml1_entry: pml3_t = @bitCast(pml1[virtaddr_pml1]);
		if(pml1_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		const addr =  try get_addr_from_entry(@as(page_t, @bitCast(pml1_entry)));
		return addr;
	}

	fn get_ptl4(self: *PageManager, root_table: *page_table_type, virtaddr: usize) !*page_table_type {
		if(self.n_levels == 5) {
			const virtaddr_5 = (virtaddr >> 48) & 0x1ff;
			const entry: pml5_t = @bitCast(root_table[virtaddr_5]);
			const realaddr_pml4 = try get_addr_from_entry(entry);
			return @ptrFromInt(realaddr_pml4 + self.hhdm_offset);
		} else return root_table;
	}

	pub fn get_addr_from_entry(entry: anytype) !usize {
		switch(@TypeOf(entry)) {
			pml2_t => {
				if(entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
				return @as(u64, entry.addr) << 12;
			}, // pml3_t == pml2_t
			pml4_t => {
				if(entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
				return @as(u64, entry.addr) << 12;
			}, // pml5_t == pml4_t
			page_t => {
				if(entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
				return @as(u64, entry.addr) << 12;
			},
			large_page_t => {
				if(entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
				return @as(u64, entry.addr) << 21;
			},
			huge_page_t => {
				if(entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
				return @as(u64, entry.addr) << 30;
			},
			else => {
				@compileError("Expected page table entry type");
			}
		}
	}

	pub fn map_4k(self: *PageManager, root_table: *page_table_type, physaddr: usize, virtaddr: usize) !void {
		if(physaddr & 0xFFF != 0 or virtaddr & 0xFFF != 0) return error.BAD_ALIGN;
		// Entries in each table level
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		const virtaddr_pml1 = (virtaddr >> 12) & 0x1ff;
		// Look at level 4 page table
		const pml4_entry: pml4_t = @bitCast((try self.get_ptl4(root_table, virtaddr))[virtaddr_pml4]);
		if(pml4_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Look at level 3 page table 
		const pml3: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml4_entry) + self.hhdm_offset);
		const pml3_entry: pml3_t = @bitCast(pml3[virtaddr_pml3]);
		if(pml3_entry.p == 0 or pml3_entry.ps == 1) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Look at level 2 page table
		const pml2: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml3_entry) + self.hhdm_offset);
		const pml2_entry: pml3_t = @bitCast(pml2[virtaddr_pml2]);
		if(pml2_entry.p == 0 or pml2_entry.ps == 1) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Finally map at level 1 page table
		const pml1: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml2_entry) + self.hhdm_offset);
		pml1[virtaddr_pml1] = @bitCast(page_t{
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.d = 0,
			.pat = 0,
			.g = 0,
			.avl1 = 0,
			.addr = @truncate(physaddr >> 12),
			.avl3 = 0,
			.pk = 0,
			.xd = 0
		});
	}

	pub fn map_2m(self: *PageManager, root_table: *page_table_type, physaddr: usize, virtaddr: usize) !void {
		if(physaddr & 0x1FFFFF != 0 or virtaddr & 0x1FFFFF != 0) return error.BAD_ALIGN;
		// Entries in each table level
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		// Look at level 4 page table
		const pml4_entry: pml4_t = @bitCast((try self.get_ptl4(root_table, virtaddr))[virtaddr_pml4]);
		if(pml4_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Look at level 3 page table
		const pml3: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml4_entry) + self.hhdm_offset);
		const pml3_entry: pml3_t = @bitCast(pml3[virtaddr_pml3]);
		if(pml3_entry.p == 0 or pml3_entry.ps == 1) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Map at level 2page table
		const pml2: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml3_entry) + self.hhdm_offset);

		pml2[virtaddr_pml2] = @bitCast(large_page_t{
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.d = 0,
			.ps = 1,
			.g = 0,
			.avl1 = 0,
			.pat = 0,
			.resvd = 0,
			.addr = @truncate(physaddr >> 21),
			.avl3 = 0,
			.pk = 0,
			.xd = 0
		});
	}

	pub fn map_1g(self: *PageManager, root_table: *page_table_type, physaddr: usize, virtaddr: usize) !void {
		if(physaddr & 0x1FFFFF != 0 or virtaddr & 0x1FFFFF != 0) return error.BAD_ALIGN;
		// Entries in each table level
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		// Look at level 4 page table
		const pml4_entry: pml4_t = @bitCast((try self.get_ptl4(root_table, virtaddr))[virtaddr_pml4]);
		if(pml4_entry.p == 0) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Look at level 3 page table
		const pml3: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml4_entry) + self.hhdm_offset);

		pml3[virtaddr_pml3] = @bitCast(huge_page_t{
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.d = 0,
			.ps = 1,
			.g = 0,
			.avl1 = 0,
			.pat = 0,
			.resvd = 0,
			.addr = @truncate(physaddr >> 30),
			.avl3 = 0,
			.pk = 0,
			.xd = 0
		});
	}

	/// Add pml4 table to handle a region of 256 TiB
	pub fn add_pml4(self: *PageManager, root_table: *page_table_type, pml4: *page_table_type, region: usize) !void {
		if(self.n_levels < 5) return error.LEVEL_5_NOT_SUPPORTED;
		if(region & 0x0000_FFFF_FFFF_FFFF != 0) return error.BAD_ALIGN;
		const region_entry = (region >> 48) & 0x1ff;
		root_table[region_entry] = @bitCast(pml5_t {
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.avl1 = 0,
			.rsvd = 0,
			.avl2 = 0,
			.addr = @truncate(@intFromPtr(pml4) >> 12),
			.avl3 = 0,
			.xd = 0
		});
	}

	/// Add pml3 table to handle a region of 512 GiB
	pub fn add_pml3(self: *PageManager, root_table: *page_table_type, pml3: *page_table_type, region: usize) !void {
		std.log.info("Adding pml3 {x}", .{&pml3});
		if(region & 0x0000_007F_FFFF_FFFF != 0) return error.BAD_ALIGN;
		const page_table = try self.get_ptl4(root_table, region);
		const region_entry = (region >> 39) & 0x1ff;
		page_table[region_entry] = @bitCast(pml4_t {
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.avl1 = 0,
			.rsvd = 0,
			.avl2 = 0,
			.addr = @truncate(@intFromPtr(pml3) >> 12),
			.avl3 = 0,
			.xd = 0
		});
	}

	/// Add pml2 table to handle a region of 1 GiB
	pub fn add_pml2(self: *PageManager, root_table: *page_table_type, pml2: *page_table_type, region: usize) !void {
		if(region & 0x0000_0000_3FFF_FFFF != 0) return error.BAD_ALIGN;
		const ptl4 = try self.get_ptl4(root_table, region);
		const virtaddr_pml3 = (region >> 39) & 0x1ff;
		const virtaddr_pml2 = (region >> 30) & 0x1ff;
		const pml4_entry: pml4_t = @bitCast(ptl4[virtaddr_pml3]);
		const ptl3: *page_table_type = @ptrFromInt((try get_addr_from_entry(pml4_entry)) + self.hhdm_offset);

		ptl3[virtaddr_pml2] = @bitCast(pml3_t {
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.avl1 = 0,
			.ps = 0,
			.avl2 = 0,
			.addr = @truncate(@intFromPtr(pml2) >> 12),
			.avl3 = 0,
			.xd = 0
		});
	}

	/// Add pml1 table to handle a region of 2 MiB
	pub fn add_pml1(self: *PageManager, root_table: *page_table_type, pml1: *page_table_type, region: usize) !void {
		if(region & 0x0000_0000_001F_FFFF != 0) return error.BAD_ALIGN;
		const ptl4 = try self.get_ptl4(root_table, region);
		const virtaddr_pml3 = (region >> 39) & 0x1ff;
		const virtaddr_pml2 = (region >> 30) & 0x1ff;
		const virtaddr_pml1 = (region >> 21) & 0x1ff;

		const ptl3: *page_table_type = @ptrFromInt((try get_addr_from_entry(@as(pml4_t, @bitCast(ptl4[virtaddr_pml3])))) + self.hhdm_offset);
		const ptl2: *page_table_type = @ptrFromInt((try get_addr_from_entry(@as(pml3_t, @bitCast(ptl3[virtaddr_pml2])))) + self.hhdm_offset);

		ptl2[virtaddr_pml1] = @bitCast(pml2_t {
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.avl1 = 0,
			.ps = 0,
			.avl2 = 0,
			.addr = @truncate(@intFromPtr(pml1) >> 12),
			.avl3 = 0,
			.xd = 0
		});
	}

	pub fn unmap(self: *PageManager, root_table: *page_table_type, virtaddr: usize) !void {
		std.log.info("Unmapping address {x}", .{virtaddr});
		if(virtaddr & 0x1FF != 0) return error.BAD_ALIGN;
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		const virtaddr_pml1 = (virtaddr >> 12) & 0x1ff;

		const ptl4 = try self.get_ptl4(root_table, virtaddr);
		const ptl4_entry: pml4_t = @bitCast(ptl4[virtaddr_pml4]);
		const ptl3: *page_table_type = @ptrFromInt((try get_addr_from_entry(ptl4_entry)) + self.hhdm_offset);
		const ptl3_entry: pml3_t = @bitCast(ptl3[virtaddr_pml3]);
		if(ptl3_entry.ps != 0) {
			// 1 GiB page
			ptl3[virtaddr_pml3] = 0;
			return;
		}

		const ptl2: *page_table_type = @ptrFromInt((try get_addr_from_entry(ptl3_entry)) + self.hhdm_offset);
		const ptl2_entry: pml3_t = @bitCast(ptl2[virtaddr_pml2]);
		if(ptl2_entry.ps != 0) {
			// 2 MiB page
			ptl2[virtaddr_pml2] = 0;
			return;
		}

		const ptl1: *page_table_type = @ptrFromInt((try get_addr_from_entry(ptl2_entry)) + self.hhdm_offset);
		ptl1[virtaddr_pml1] = 0;
	}

	pub fn is_mapped(self: *PageManager, root_table: *page_table_type, virtaddr: usize) bool {
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		const virtaddr_pml1 = (virtaddr >> 12) & 0x1ff;

		const ptl4 = self.get_ptl4(root_table, virtaddr) catch return false;
		const ptl4_entry: pml4_t = @bitCast(ptl4[virtaddr_pml4]);
		if(ptl4_entry.p == 0) {
			return false;
		}
		const ptl3: *page_table_type = @ptrFromInt((get_addr_from_entry(ptl4_entry) catch return false) + self.hhdm_offset);
		const ptl3_entry: pml3_t = @bitCast(ptl3[virtaddr_pml3]);
		if(ptl3_entry.p == 0) {
			return false;
		}
		if(ptl3_entry.ps == 1) { return true; }

		const ptl2: *page_table_type = @ptrFromInt((get_addr_from_entry(ptl3_entry) catch return false) + self.hhdm_offset);
		const ptl2_entry: pml3_t = @bitCast(ptl2[virtaddr_pml2]);
		if(ptl2_entry.p == 0) {
			return false;
		}
		if(ptl2_entry.ps == 1) { return true; }

		const ptl1: *page_table_type = @ptrFromInt((get_addr_from_entry(ptl2_entry) catch return false) + self.hhdm_offset);
		const ptl1_entry: page_t = @bitCast(ptl1[virtaddr_pml1]);
		if(ptl1_entry.p == 0) {
			return false;
		}
		return true;
	}

	pub fn delete_entry(self: *PageManager, table: *page_table_type, entry: u9) void {
		_ = self;
		table[entry] = 0;
	}

	pub fn map_4k_alloc(self: *PageManager, root_table: *page_table_type, physaddr: usize, virtaddr: usize, alloc: *frame.PageFrameAllocator) !void {
		if(physaddr & 0xFFF != 0 or virtaddr & 0xFFF != 0) return error.BAD_ALIGN;
		// Entries in each table level
		const virtaddr_pml4 = (virtaddr >> 39) & 0x1ff;
		const virtaddr_pml3 = (virtaddr >> 30) & 0x1ff;
		const virtaddr_pml2 = (virtaddr >> 21) & 0x1ff;
		const virtaddr_pml1 = (virtaddr >> 12) & 0x1ff;
		// Look at level 4 page table
		const pml4_entry: *pml4_t = @ptrCast(@alignCast(&(try self.get_ptl4(root_table, virtaddr))[virtaddr_pml4]));
		if(pml4_entry.p == 0) {
			const ptl3 = (try alloc.alloc(1)) orelse {return error.OUT_OF_MEMORY;};
			pml4_entry.* = pml4_t {
				.p = 1,
				.rw = 1,
				.us = 0,
				.pwt = 0,
				.pcd = 0,
				.a = 0,
				.avl1 = 0,
				.rsvd = 0,
				.avl2 = 0,
				.addr = @truncate(ptl3 >> 12),
				.avl3 = 0,
				.xd = 0
			};
			try self.map_4k(self.root_table.?, ptl3, ptl3 + self.hhdm_offset);
		}
		// Look at level 3 page table 
		const pml3: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml4_entry.*) + self.hhdm_offset);
		const pml3_entry: *pml3_t = @ptrCast(@alignCast(&pml3[virtaddr_pml3]));
		if(pml3_entry.p == 0) {
			const ptl2 = (try alloc.alloc(1)) orelse {return error.OUT_OF_MEMORY;};
			pml3_entry.* = pml3_t {
				.p = 1,
				.rw = 1,
				.us = 0,
				.pwt = 0,
				.pcd = 0,
				.a = 0,
				.avl1 = 0,
				.ps = 0,
				.avl2 = 0,
				.addr = @truncate(ptl2 >> 12),
				.avl3 = 0,
				.xd = 0
			};
			try self.map_4k(self.root_table.?, ptl2, ptl2 + self.hhdm_offset);
		}
		if(pml3_entry.ps == 1) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Look at level 2 page table
		const pml2: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml3_entry.*) + self.hhdm_offset);
		const pml2_entry: *pml2_t = @ptrCast(@alignCast(&pml2[virtaddr_pml2]));
		if(pml2_entry.p == 0) {
			const ptl1 = (try alloc.alloc(1)) orelse {return error.OUT_OF_MEMORY;};
			pml2_entry.* = pml2_t {
				.p = 1,
				.rw = 1,
				.us = 0,
				.pwt = 0,
				.pcd = 0,
				.a = 0,
				.avl1 = 0,
				.ps = 0,
				.avl2 = 0,
				.addr = @truncate(ptl1 >> 12),
				.avl3 = 0,
				.xd = 0
			};
			try self.map_4k(self.root_table.?, ptl1, ptl1 + self.hhdm_offset);
		}
		if(pml2_entry.ps == 1) return error.PATH_TO_TABLE_DOES_NOT_EXIST;
		// Finally map at level 1 page table
		const pml1: *page_table_type = @ptrFromInt(try get_addr_from_entry(pml2_entry.*) + self.hhdm_offset);
		pml1[virtaddr_pml1] = @bitCast(page_t{
			.p = 1,
			.rw = 1,
			.us = 0,
			.pwt = 0,
			.pcd = 0,
			.a = 0,
			.d = 0,
			.pat = 0,
			.g = 0,
			.avl1 = 0,
			.addr = @truncate(physaddr >> 12),
			.avl3 = 0,
			.pk = 0,
			.xd = 0
		});
	}
};