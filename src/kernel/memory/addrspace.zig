const std = @import("std");
const page = @import("../memory/pagemanager.zig");
const pa = @import("../memory/pagetablealloc.zig");
const main = @import("../main.zig");
const pp = @import("physicalpage.zig");
const ipcfn = @import("../ipc/ipcfn.zig");
const syc = @import("../syscalls/syscall_setup.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");
const spin = @import("../sync/spinlock.zig");

const N_LEVELS_FOR_ALLOC = 4;

pub const AddressSpace = struct{
	cr3: ?*page.page_table_type,
	refcount: std.atomic.Value(u32),
	pageman: *page.PageManager,
	mycpu: u32,
	lock: spin.SpinLock,

	pub fn init(pman: *page.PageManager) @This() {
		return .{
			.cr3 = null,
			.refcount = std.atomic.Value(u32).init(0),
			.pageman = pman,
			.mycpu = @truncate(main.mycpuid()),
			.lock = spin.SpinLock.init()
		};
	}

	fn fallback_on_page_table_allocation(tables: [N_LEVELS_FOR_ALLOC]?*page.page_table_type, level: usize) !void {
		for(level..N_LEVELS_FOR_ALLOC) |j| {
			if(tables[j]) |ptr| {
				try pa.PageTableAllocator.free(ptr);
			}
		}
	}

	fn add_page_tables(self: *AddressSpace, virtaddr: usize, level: usize) !void {
		var tables = [_]?*page.page_table_type{null} ** N_LEVELS_FOR_ALLOC;
		for(level..N_LEVELS_FOR_ALLOC) |i| {
			const table = pa.PageTableAllocator.alloc() orelse {
				try fallback_on_page_table_allocation(tables, level);
				return error.NO_SPACE_LEFT;
			};
			tables[i] = table;
			const table_physaddr: *page.page_table_type = @ptrFromInt(@intFromPtr(table) - self.pageman.hhdm_offset);
			_ = switch(i) {
				0 => self.pageman.add_pml4(self.cr3.?, table_physaddr, virtaddr &  ~@as(u64, 0x0000_FFFF_FFFF_FFFF)),
				1 => self.pageman.add_pml3(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_007F_FFFF_FFFF)),
				2 => self.pageman.add_pml2(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_0000_3FFF_FFFF)),
				3 => self.pageman.add_pml1(self.cr3.?, table_physaddr, virtaddr & ~@as(u64, 0x0000_0000_001F_FFFF)),
				else => {}
			} catch |err| {
				try fallback_on_page_table_allocation(tables, level);
				return err;
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
		self.lock.lock();
		defer self.lock.unlock();
		if(self.cr3 == null) {
			self.cr3 = pa.PageTableAllocator.alloc() orelse return error.NO_SPACE_LEFT;
			@memcpy(self.cr3.?, self.pageman.root_table.?);
		}
		const level = self.pageman.last_map_level(self.cr3.?, virtaddr);
		if(level < N_LEVELS_FOR_ALLOC) {
			try self.add_page_tables(virtaddr, level);
		}
		try self.pageman.map_4k_cascade(self.cr3.?, physaddr, virtaddr, opt1, opt2, opt3, opt4);
		if(physaddr) |u| {
			const pg = pp.PhysicalPageManager.get(u);
			if(pg) |pag| {
				const in = pag.is_in_addr_space(self);
				if(!in) {
					try pag.add_addr_space(self, virtaddr);
				}
			} else {
				std.log.err("Address {x} is not in Physical Page Manager", .{u});
				return error.NO_DEST;
			}
		}
	}

	pub fn remove_mapping_4k(self: *AddressSpace, virtaddr: usize) !void {
		self.lock.lock();
		defer self.lock.unlock();
		if(self.cr3 == null) return;
		try self.pageman.unmap(self.cr3.?, virtaddr);
	}

	pub fn get_physaddr(self: *AddressSpace, virtaddr: usize) ?usize {
		self.lock.lock();
		defer self.lock.unlock();
		if(!self.pageman.is_mapped(self.cr3, virtaddr)) return null;
		return self.pageman.get_physaddr(virtaddr) catch null;
	}

	fn send_pages_to_mmanager(self: *AddressSpace) !void {
		var it = try self.pageman.iterator(self.cr3.?);
		const Range = struct {
			vaddr: usize,
			npages: usize,
			granter_addr: usize,
		};

		var current: ?Range = null;

		const flush = struct {
			fn call(r: Range, adr: *AddressSpace) !void {
				const msg = ipcfn.ipc_msg.ipc_message_t{
					.dest = 0,
					.source = 0,
					.npages = r.npages,
					.page = r.vaddr,
					.flags = ipcfn.ipc_msg.IPC_FLAG_GRANT_PAGE,
					.value0 = ipcfn.ipc_msg.PAGE_MAP_USER | ipcfn.ipc_msg.PAGE_MAP_READ,
					.value1 = r.granter_addr,
					.value2 = 0,
					.value3 = 0,
					.value4 = 0,
				};

				if (syc.inner_transfer_page_range(&msg, adr) != 0)
					return error.TRANSFER_ERROR;
			}
		}.call;

		while (it.next()) |p| {
			const addr = switch (p) {
				.page => |v| try page.PageManager.get_addr_from_entry(v.*),
				.large_page => |v| try page.PageManager.get_addr_from_entry(v.*),
				.med_page => |v| try page.PageManager.get_addr_from_entry(v.*),
			};

			const entry = pp.PhysicalPageManager.get(addr) orelse unreachable;
			const nentries = entry.remove_addr_space(self);

			if (nentries) |red| {
				if (red.nentries != 1)
					continue;

				const vaddr = red.virtaddr;
				const gaddr = entry.grantor.vaddr;

				if (current) |*c| {
					const cont =
						c.vaddr + c.npages * page.PAGE_SIZE == vaddr and
						c.granter_addr + c.npages * page.PAGE_SIZE == gaddr;

					if (cont) {
						c.npages += 1;
						continue;
					}

					// Send to mmanager
					try flush(c.*, self);
					current = null;
				}
				current = .{
					.vaddr = vaddr,
					.npages = 1,
					.granter_addr = gaddr,
				};
			}
		}

		// trail
		if (current) |c| {
			try flush(c, self);
		}
	}

	pub fn open(self: *AddressSpace) void {
		_ = self.refcount.fetchAdd(1, .acq_rel);
	}

	pub fn close(self: *AddressSpace) !void {
		const val = self.refcount.fetchSub(1, .acq_rel);
		if(val == 1) {
			const mycpuid = main.mycpuid();
			
			try self.send_pages_to_mmanager();
			// Free page tables in use
			var itp = try self.pageman.table_iterator(self.cr3.?);
			while(itp.next()) |table| {
				std.log.info("{*}", .{table});
				const owner = pa.PageTableAllocator.get_cpu_owner(table);
				if(owner == mycpuid) {
					try pa.PageTableAllocator.free(table);
				} else if(owner) |ow| {
					const pay = ipi.IPIProtocolPayload.init_with_data(ipi.IPIProtocolMessageType.FREE_PAGE_TABLE, @intFromPtr(table), 0,0 );
					ipi.IPIProtocolHandler.send_ipi(@truncate(ow), pay);
				} else {
					return error.OUT_OF_RANGE;
				}
			}
		}
		std.log.info("Closed 1 entry of address space", .{});
	}
};