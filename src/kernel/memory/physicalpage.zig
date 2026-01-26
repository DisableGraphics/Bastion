const addrspac = @import("addrspace.zig");
const std = @import("std");
const limine = @import("limine");
const km = @import("kmm.zig");
const page = @import("pagemanager.zig");
const ma = @import("multialloc.zig");
const ipi = @import("../interrupts/ipi_protocol.zig");
const main = @import("../main.zig");

pub const MappingNodeAllocator = ma.MultiAlloc(MappingNode, false, 1024); 

pub const MappingNode = struct {
	as: *addrspac.AddressSpace,
	vpn: usize, 
	next: ?*MappingNode,
	parent: ?*MappingNode, 
	cpu_owner: u64,
};
const OwnerData = struct {
	owner: ?*addrspac.AddressSpace,
	vaddr: usize,
};

pub const PhysicalPage = struct {
	mapping_data: union {
		single: OwnerData,
		shared_list: ?*MappingNode,
	},
	grantor: OwnerData,
	refcount: std.atomic.Value(u32),

	pub fn init() PhysicalPage {
		return .{
			.grantor = .{.owner = null, .vaddr = 0},
			.mapping_data = .{.single = .{.owner = null, .vaddr = 0}},
			.refcount = std.atomic.Value(u32).init(0)
		};
	}

	pub fn is_in_addr_space(self: *PhysicalPage, ad: *addrspac.AddressSpace) bool {
		const s = self.refcount.load(.acquire);
		if(s == 1) {
			return ad == self.mapping_data.single.owner;
		} else if(s > 1) {
			var head = self.mapping_data.shared_list;
			while(head != null) {
				if(head.?.as == ad) return true;
				head = head.?.next;
			}
			return false;
		} else {
			return false;
		}
	}

	pub fn add_addr_space(self: *PhysicalPage, ad: *addrspac.AddressSpace, vpn: usize) !void {
		const s = self.refcount.fetchAdd(1, .acq_rel);
		if(s == 0) {
			self.mapping_data.single.owner = ad;
			self.mapping_data.single.vaddr = vpn;
		} else if(s == 1) {
			const curr_on_err = self.mapping_data.single;
			const current = MappingNode {
				.as = curr_on_err.owner.?,
				.vpn = curr_on_err.vaddr,
				.parent = null,
				.next = null,
				.cpu_owner = main.mycpuid()
			};

			self.mapping_data.shared_list = MappingNodeAllocator.alloc() orelse {
				_ = self.refcount.fetchSub(1, .acq_rel);
				return error.NO_SPACE_AVAILABLE;
			};
			self.mapping_data.shared_list.?.next = MappingNodeAllocator.alloc() orelse {
				MappingNodeAllocator.free(self.mapping_data.shared_list.?) catch unreachable;
				self.mapping_data.single = curr_on_err;
				_ = self.refcount.fetchSub(1, .acq_rel);
				return error.NO_SPACE_AVAILABLE;
			};
			self.mapping_data.shared_list.?.* = current;
			self.mapping_data.shared_list.?.next.?.* = MappingNode{
				.as = ad,
				.vpn = vpn,
				.next = null,
				.parent = null,
				.cpu_owner = main.mycpuid()
			};
		} else {
			var head = self.mapping_data.shared_list;
			var h1 = head;
			while(h1 != null) {
				head = h1;
				h1 = h1.?.next;
			}

			head.?.next = MappingNodeAllocator.alloc() orelse {
				_ = self.refcount.fetchSub(1, .acq_rel);
				return error.NO_SPACE_AVAILABLE;
			};
			head.?.next.?.* = MappingNode{
				.as = ad,
				.vpn = vpn,
				.next = null,
				.parent = null,
				.cpu_owner = main.mycpuid()
			};
		}
	}

	pub fn remove_addr_space(self: *PhysicalPage, ad: *addrspac.AddressSpace) ?struct {
		nentries: usize, virtaddr: usize} {
		const r = self.refcount.load(.acquire);
		if(r == 1) {
			if(self.mapping_data.single.owner == ad) {
				const ref = self.refcount.fetchSub(1, .acq_rel);
				self.mapping_data.single.owner = null;
				return .{ .nentries = ref, .virtaddr = self.mapping_data.single.vaddr};
			}
		} else if(r == 2) {
			var head = self.mapping_data.shared_list;
			var h1 = head;
			while(h1 != null and h1.?.as != ad) {
				head = h1;
				h1 = h1.?.next;
			}
			if(h1 != null) {
				const ref = self.refcount.fetchSub(1, .acq_rel);
				const n1 = self.mapping_data.shared_list;
				const n2 = self.mapping_data.shared_list.?.next;

				const result = if(n1.?.as == ad) n2.? else n1.?;
				const opp = if(n1.?.as == ad) n1.? else n2.?;
				const virtaddr_ret = opp.vpn;

				self.mapping_data.single = .{.owner = result.as, .vaddr = result.vpn};

				const v = [_]?*MappingNode{n1, n2};
				for(v) |node| {
					if(node.?.cpu_owner == main.mycpuid()) {
						MappingNodeAllocator.free(node.?) catch unreachable;
					} else {
						ipi.IPIProtocolHandler.send_ipi(@truncate(node.?.cpu_owner), 
							ipi.IPIProtocolPayload.init_with_data(
								ipi.IPIProtocolMessageType.FREE_PAGE_NODE, 
								@intFromPtr(node.?), 0,0)
						);
					}
				}
				return .{ .nentries = ref, .virtaddr = virtaddr_ret};
			}
		} else if(r > 2) {
			var head = self.mapping_data.shared_list;
			var h1 = head;
			while(h1 != null and h1.?.as != ad) {
				head = h1;
				h1 = h1.?.next;
			}
			if(h1 != null) {
				const ref = self.refcount.fetchSub(1, .acq_rel);
				const node = head.?.next;
				const virtaddr_ret = node.?.vpn;
				if(node.?.cpu_owner == main.mycpuid()) {
					MappingNodeAllocator.free(node.?) catch unreachable;
				} else {
					ipi.IPIProtocolHandler.send_ipi(@truncate(node.?.cpu_owner), 
						ipi.IPIProtocolPayload.init_with_data(
							ipi.IPIProtocolMessageType.FREE_PAGE_NODE, 
							@intFromPtr(node.?), 0,0)
					);
				}
				head.?.next = head.?.next.?.next;
				return .{ .nentries = ref, .virtaddr = virtaddr_ret};
			}
		}
		return null;
	}

	pub fn set_owner(self: *PhysicalPage, ad: *addrspac.AddressSpace, vdn: usize) void {
		const v = self.refcount.load(.acquire);
		std.debug.assert(v == 0 or v == 1);
		self.mapping_data.single.owner = ad;
		self.mapping_data.single.vaddr = vdn;
	}

	pub fn set_grantor(self: *PhysicalPage, ad: *addrspac.AddressSpace, vdn: usize) void {
		self.grantor = OwnerData{.owner = ad, .vaddr = vdn};
	}
};

const PhysicalPageRegion = struct {
	start: usize,
	end: usize,
	physical_pages: []PhysicalPage
};

const region_t = []PhysicalPageRegion;

pub const PhysicalPageManager = struct {
	var regions: region_t = undefined;
	fn truncate_to_page(addr: usize) usize {
		return addr & ~@as(usize, 0xFFF);
	}
	pub fn ginit(mmap: *limine.MemoryMapResponse, kmm: *km.KernelMemoryManager) !void {
		const buf_size = 2048;
		var buffer: [buf_size]u8 = undefined;
		var tmp_alloc = std.heap.FixedBufferAllocator.init(&buffer);
		// Should have enough headroom.
		var tmp_regions = try std.ArrayList(PhysicalPageRegion)
			.initCapacity(tmp_alloc.allocator(), buf_size/@sizeOf(PhysicalPageRegion));
		// Don't really need to defer

		const entries = mmap.getEntries();
		var start_addr = entries[0].base;
		var prev_addr = truncate_to_page(entries[0].base + entries[0].length);
		var n_entries: usize = 0;
		for(1..entries.len) |entry| {
			const start = truncate_to_page(entries[entry].base);
			if(start != prev_addr) {
				try tmp_regions.append(.{
					.start = start_addr,
					.end = prev_addr,
					.physical_pages = undefined
				});
				start_addr = start;
				n_entries += 1;
			}

			prev_addr = truncate_to_page(entries[entry].base + entries[entry].length);
		}

		// Allocate for regions
		const regions_pages = ((n_entries * @sizeOf(PhysicalPageRegion)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		regions = @as([*]PhysicalPageRegion, @ptrFromInt((try kmm.alloc_virt(regions_pages)).?))[0..n_entries];

		for(0..n_entries) |i| {
			regions[i] = tmp_regions.items[i];
			const size_pages = ((regions[i].end - regions[i].start) + page.PAGE_SIZE - 1)/page.PAGE_SIZE;
			const page_regions_pages = ((size_pages * @sizeOf(PhysicalPage)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			regions[i].physical_pages = @as([*]PhysicalPage, @ptrFromInt((try kmm.alloc_virt(page_regions_pages)).?))[0..size_pages];
			std.log.info("Pages in region: {}", .{size_pages});
			for(0..size_pages) |p| {
				regions[i].physical_pages[p] = PhysicalPage.init();
			}
		}
	}

	pub fn get(physaddr: usize) ?*PhysicalPage {
		for(regions) |region| {
			if(physaddr >= region.start and physaddr < region.end) {
				const offset = (physaddr - region.start) / page.PAGE_SIZE;
				return &region.physical_pages[offset];
			}
		}
		return null;
	}
};