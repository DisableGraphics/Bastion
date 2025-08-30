const page = @import("../../../memory/pagemanager.zig");
const kmm = @import("../../../memory/kmm.zig");
const std = @import("std");

const RSDP = packed struct {
	signature: u64,
	checksum: u8,
	oemid: u48,
	revision: u8,
	rsdtaddress: u32
};

const XSDP = packed struct {
	rsdp: RSDP,
	length: u32,
	xsdt_address: u64,
	extended_checksum: u8,
	reserved: u24
};

const ACPISDTHeader = packed struct {
	signature: u32,
	length: u32,
	revision: u8,
	checksum: u8,
	oemid: u48,
	oem_table_id: u64,
	oem_revision: u32,
	creator_id: u32,
	creator_revision: u32,
	pub fn format(
		self: ACPISDTHeader,
		comptime fmt: []const u8,
		options: std.fmt.FormatOptions,
		writer: anytype,
	) !void {
		_ = fmt;
		_ = options;
		var bf: [4]u8 = undefined;
		std.mem.writeInt(u32, &bf, self.signature, .little);
		try writer.print("ACPISDTHeader{{ .signature = \"{s}\", .length = {}, .revision = {}, checksum = {x}, oemid = {x}, oem_table_id = {x}, creator_id = {x}, creator_revision = {x} }}", .{
			bf,
			self.length,
			self.revision,
			self.checksum,
			self.oemid,
			self.oem_table_id,
			self.creator_id,
			self.creator_revision
		});
	}
};


const RSDT = packed struct {
	header: ACPISDTHeader,
	pointers_to_other_sdts: u32,
	// TODO: change this
	pub fn get_pointers(s:* align(1) RSDT) []align(1) u32 {
		const ptr = @intFromPtr(&s.pointers_to_other_sdts);
		const acpi_header_len = @bitSizeOf(ACPISDTHeader)/8;
		std.log.debug("{} - {}", .{s.header.length, acpi_header_len});
		const len = (s.header.length - acpi_header_len)/4;
		return (@as([*]align(1) u32, @ptrFromInt(ptr)))[0..len];
	}
};

const XSDT = packed struct {
	header: ACPISDTHeader,
	pointers_to_other_sdts: u64,
	pub fn get_pointers(s:* align(1) XSDT) []align(1) u64 {
		const ptr = @intFromPtr(&s.pointers_to_other_sdts);
		const len = (s.header.length - @sizeOf(ACPISDTHeader))/8;
		return (@as([*]align(1) u64, @ptrFromInt(ptr)))[0..len];
	}
};

pub const MADTLocalAPIC = packed struct {
	header: MADTEntryHeader,
	acpi_processor_id: u8,
	apic_id: u8,
	flags: u32
};
const MADTIOAPIC = packed struct {
	header: MADTEntryHeader,
	io_apic_id: u8,
	reserved: u8,
	io_apic_addr: u32,
	global_system_interrupt_base: u32
};
const MADTIOAPICSourceOverride = packed struct {
	header: MADTEntryHeader,
	bus_source: u8,
	irq_source: u8,
	global_system_interrupt: u32,
	flags: u16
};
const MADTIOAPICNMISource = packed struct {
	header: MADTEntryHeader,
	nmi_source: u8,
	reserved: u8,
	flags: u16,
	global_system_interrupt: u32,
};
const MADTLocalAPICNMI = packed struct {
	header: MADTEntryHeader,
	acpi_processor_id: u8,
	flags: u16,
	lint: u8,
};
const MADTLocalAPICAddressOverride = packed struct {
	header: MADTEntryHeader,
	reserved: u16,
	local_apic_physaddr: u64
};
const MADTProcessorLocalx2APIC = packed struct {
	header: MADTEntryHeader,
	reserved: u16,
	processor_local_x2_apic_id: u32,
	flags: u32,
	acpi_id: u32
};

const MADTEntryTag = enum(u8){
	local_apic=0,
	io_apic=1,
	io_apic_source_override=2,
	io_apic_nmi_source=3,
	local_apic_nmi=4,
	local_apic_address_override=5,
	processor_local_x2_apic=9
};

const MADTEntryHeader = packed struct {
	tag: MADTEntryTag,
	length: u8,
};

const MADTEntryUnion = union(MADTEntryTag) {
	local_apic: *align(1) MADTLocalAPIC,
	io_apic: *align(1) MADTIOAPIC,
	io_apic_source_override: *align(1) MADTIOAPICSourceOverride,
	io_apic_nmi_source: *align(1) MADTIOAPICNMISource,
	local_apic_nmi: *align(1) MADTLocalAPICNMI,
	local_apic_address_override: *align(1) MADTLocalAPICAddressOverride,
	processor_local_x2_apic: *align(1) MADTProcessorLocalx2APIC
};

const MADT = packed struct {
	header: ACPISDTHeader,
	local_apic_addr: u32,
	flags: u32,
	_entries: u16,
	fn get_entries(self: *align(1) MADT, out_elements: []MADTEntryUnion) !usize {
		var index: usize = 0;
		var count: usize = 0;
		const length = self.header.length - (@bitSizeOf(ACPISDTHeader)/8) - 8;
		const tbl: []align(1) u8 = @as([*]align(1) u8, @ptrCast(@alignCast(&self._entries)))[0..length];

		while (index < tbl.len) {
			if (count >= out_elements.len) return error.TooManyElements;

			const header: *align(1) const MADTEntryHeader = @ptrCast(&tbl[index]);
			if (index + header.length > tbl.len) return error.MalformedTable;

			switch(header.tag) {
				.local_apic => {
					out_elements[count] = MADTEntryUnion{.local_apic = @ptrCast(&tbl[index])};
				},
				.io_apic=> {
					out_elements[count] = MADTEntryUnion{.io_apic = @ptrCast(&tbl[index])};
				},
				.io_apic_source_override=> {
					out_elements[count] = MADTEntryUnion{.io_apic_source_override = @ptrCast(&tbl[index])};
				},
				.io_apic_nmi_source=> {
					out_elements[count] = MADTEntryUnion{.io_apic_nmi_source = @ptrCast(&tbl[index])};
				},
				.local_apic_nmi=> {
					out_elements[count] = MADTEntryUnion{.local_apic_nmi = @ptrCast(&tbl[index])};
				},
				.local_apic_address_override=> {
					out_elements[count] = MADTEntryUnion{.local_apic_address_override = @ptrCast(&tbl[index])};
				},
				.processor_local_x2_apic => {
					out_elements[count] = MADTEntryUnion{.processor_local_x2_apic = @ptrCast(&tbl[index])};
				},
			}

			index += header.length;
			count += 1;
		}

		return count;
	}
};

pub const tabletype = enum{
	madt
};
pub const table = union(tabletype){
	madt: *align(1) MADT,
};

pub const ACPIManager = struct {
	revision: u64,
	address: u64,
	hhdm_offset: u64,
	pub fn init(rev: u64, virtaddr: u64, hhdm_offset: u64, mapper: *kmm.KernelMemoryManager) !ACPIManager {
		const self = ACPIManager{
			.revision = rev,
			.address = virtaddr,
			.hhdm_offset = hhdm_offset,
		};
		if(rev != 0) {
			const xsdt: *align(1) XSDT = self.get_xsdt().?;
			try mapper.pm.map_4k_alloc(mapper.pm.root_table.?, 
				(@intFromPtr(xsdt) - hhdm_offset) & ~@as(u64, 0xfff), 
				(@intFromPtr(xsdt)) & ~@as(u64, 0xfff),
				mapper.pfa);
		} else {
			const rsdt: *align(1) RSDT = self.get_rsdt().?;
			try mapper.pm.map_4k_alloc(mapper.pm.root_table.?, 
				(@intFromPtr(rsdt) - hhdm_offset) & ~@as(u64, 0xfff), 
				(@intFromPtr(rsdt)) & ~@as(u64, 0xfff),
				mapper.pfa);
		}
		return self;
	}

	fn get_xsdt(self: *const ACPIManager) ?*align(1)XSDT {
		if(self.revision == 0) return null;
		const r: *align(1)XSDP = @ptrFromInt(self.address);
		return @ptrFromInt(@as(u64, r.xsdt_address) + self.hhdm_offset);
	}

	fn get_rsdt(self: *const ACPIManager) ?*align(1)RSDT {
		if(self.revision != 0) return null;
		const r: *align(1)RSDP = @ptrFromInt(self.address);
		return @ptrFromInt(@as(u64, r.rsdtaddress) + self.hhdm_offset);
	}

	pub fn scan(self: *const ACPIManager, signature: *const [4]u8, index: usize, km: *kmm.KernelMemoryManager) !?table {
		const sig = std.mem.readInt(u32, signature, .little);
		var i = index;
		std.log.info("Signature to search: {s}", .{signature});
		if(self.get_xsdt()) |xsdt| {
			std.log.info("{any}", .{xsdt.*});
			const pointers = xsdt.get_pointers();
			std.log.info("{any}", .{pointers});
			for(pointers) |ptr| {
				const result = try self.find(ptr, km, signature, sig, &i);
				if(result) |res| {
					return res;
				}
			}
		} else if(self.get_rsdt()) |rsdt| {
			std.log.info("{any}", .{rsdt.*});
			const pointers = rsdt.get_pointers();
			std.log.info("{any}", .{pointers});
			for(pointers) |ptr| {
				const result = try self.find(ptr, km, signature, sig, &i);
				if(result) |res| {
					return res;
				}
			}
		}
		return null;
	}

	fn find(self: *const ACPIManager, ptr: u64, km: *kmm.KernelMemoryManager, signature: *const[4] u8, sig: u32, i: *usize) !?table {
		const virtptr: u64 = self.hhdm_offset + ptr;
		const header: *align(1) ACPISDTHeader = @ptrFromInt(virtptr);
		try km.pm.map_4k_alloc(
			km.pm.root_table.?, 
			ptr & ~@as(u64, 0xfff),
			virtptr & ~@as(u64, 0xfff), 
			km.pfa);
		
		std.log.info("{any}", .{header.*});
		std.log.info("{x} vs {x}", .{header.signature, sig});
		if(header.signature == sig) {
			if(i.* > 0) {
				i.* -= 1;
			} else {
				const enu = enum{APIC};
				const val = std.meta.stringToEnum(enu, signature) orelse return error.INVALID_CHOICE;
				switch (val) {
					.APIC => {
						return table{.madt = @ptrFromInt(virtptr)};
					}
				}
			}
		}
		return null;
	}

	pub fn parse_madt(self: *const ACPIManager, madt: *align(1) MADT) !void {
		_ = self;
		var entries: [128]MADTEntryUnion = undefined;
		const nentries = try madt.get_entries((&entries)[0..]);
		for(0..nentries) |i| {
			std.log.info("{x}: {any}", .{i, entries[i]});
		}
		
	}

	pub fn get_local_apic(self: *const ACPIManager, madt: *align(1) MADT, cpuid: usize) !?*align(1) MADTLocalAPIC {
		_ = self;
		var entries: [128]MADTEntryUnion = undefined;
		const nentries = try madt.get_entries((&entries)[0..]);
		for(0..nentries) |i| {
			switch(entries[i]) {
				.local_apic => |lapic| {
					if(lapic.acpi_processor_id == cpuid) {
						return lapic;
					}
				},
				else => continue
			}
			std.log.info("{x}: {any}", .{i, entries[i]});
		}
		return null;
	}
};

