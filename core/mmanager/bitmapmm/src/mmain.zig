const std = @import("std");
const mmap = @cImport(@cInclude("memmap.h"));
const mman = @cImport(@cInclude("mmanager.h"));
const bmp = @import("bitmap.zig");
const builtin = @import("builtin");
const ipc = @import("ipc.zig");
const log = @import("log.zig");
const allc = @import("alloc.zig");
const debug = @cImport(
	@cInclude("debug.h")
);

const sys = @cImport(
	@cInclude("syswrap.h")
);

pub const std_options: std.Options = .{
	// Set the log level to debug
	.log_level = .debug,
	.page_size_max = 1*1024*1024*1024,
	// Define logFn to override the std implementation
	.logFn = log.logfn,
};

comptime {
    if (builtin.os.tag == .freestanding) {
        @export(&_start, .{ .name = "_start", .linkage = .strong });
    }
}

pub fn _start() callconv(.C) noreturn {
    main();
	hcf();
}

fn hcf() noreturn {
	while(true){}
}

fn alloc_loop(allocator: *allc.PageFrameAllocator, port: ipc.ipc.port_t, fb_entry: *mmap.mmap_fb_entry_t) void {
	while(true) {
		const command = ipc.receive_command(port);
		
		switch(command) {
			.cmd => |com| {
				defer _ = sys.sys_destroy_port(com.source);
				switch(com.cmd) {
					.ALLOC_REQUEST => {
						const dst_addr = com.arg0;
						const mapping_options = com.arg1;
						const allocation = allocator.alloc(1) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						ipc.respond_with_allocation(com.source, allocation, 1, dst_addr, mapping_options) catch continue;
					},
					.FREE_REQUEST => {
						const addr = com.arg0;
						const myaddr = ipc.revoke(com.source, addr, 1) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						allocator.dealloc(myaddr, 1) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						ipc.respond_ok(com.source) catch continue;
					},
					.ALLOC_RANGE_REQUEST => {
						const dst_addr = com.arg0;
						const mapping_options = com.arg1;
						const npages = com.arg2;
						const allocation = allocator.alloc(npages) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						ipc.respond_with_allocation(com.source, allocation, npages, dst_addr, mapping_options) catch continue;
					},
					.FREE_RANGE_REQUEST => {
						const addr = com.arg0;
						const npages = com.arg1;
						const myaddr = ipc.revoke(com.source, addr, npages) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						allocator.dealloc(myaddr, npages) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
							continue;
						};
						ipc.respond_ok(com.source) catch continue;
					},
					.DEV_REQUEST => {
						const devaddr = com.arg0;
						const destaddr = com.arg1;
						const mappings = com.arg2;
						const npages = com.arg3;

						ipc.respond_with_allocation(
							com.source,
							devaddr + mmap.OFFSET_MAP,
							npages,
							destaddr,
							mappings
						) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
						};
					},
					.FB_REQUEST => {
						const fb_addr = mmap.get_address(fb_entry.defs.addrflags);
						const npages = fb_entry.defs.npages;
						const destaddr = com.arg0;
						const mappings = com.arg1;
						
						ipc.respond_with_allocation(
							com.source,
							fb_addr,
							npages,
							destaddr,
							mappings
						) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source) catch continue;
						};
						ipc.respond_with_fb(com.source, fb_entry) catch continue;
					},
				}
			},
			.ecode => |err| {
				std.log.err("Error while processing command: {s}", .{err});
			}
		}
	}
}

fn in_region_with_characteristic(memmap: *mmap.mmap, addr: usize, flagmask: usize) bool {
	var entry: *mmap.mmap_basic_entry_t = mmap.get_next_entry(memmap, null);
	for(0..memmap.size) |_| {
		const start = mmap.get_address(entry.addrflags);
		const end = start + entry.npages * bmp.PAGE_SIZE;
		const flags = mmap.get_flags(entry.addrflags);
		if(start <= addr and end > addr) {
			if(flags & flagmask != 0) {
				return true;
			} else {
				return false;
			}
		}
		entry = mmap.get_next_entry(memmap, entry);
	}
	return false;
}

fn in_usable(memmap: *mmap.mmap, addr: usize) bool {
	return in_region_with_characteristic(memmap, addr, mmap.MEM_USABLE);
}

fn in_reserved(memmap: *mmap.mmap, addr: usize) bool {
	return in_region_with_characteristic(memmap, addr, mmap.MEM_DEVICE);
}

fn get_fb_entry(memmap: *mmap.mmap) ?*mmap.mmap_fb_entry_t {
	var entry: *mmap.mmap_basic_entry_t = mmap.get_next_entry(memmap, null);
	for(0..memmap.size) |_| {
		if(mmap.get_flags(entry.addrflags) & mmap.MEM_FB != 0) {
			return @ptrCast(entry);
		}
		entry = mmap.get_next_entry(memmap, entry);
	}
	return null;
}

fn receiver_loop() noreturn {
	const port = ipc.ipc.IPC_BOOTSTRAP_PORT;
	var msg = ipc.ipc.ipc_message_t {
		.dest = port
	};
	if(ipc.ipc.sys_ipc_recv(&msg) != 0) {
		@panic("No allocator sent, cannot function");
	}
	const allocator: *allc.PageFrameAllocator = @ptrFromInt(msg.value0);
	const memmap: *mmap.mmap_t = @ptrFromInt(msg.value1);
	while(true) {
		var recv = ipc.ipc.ipc_message_t {
			.dest = port,
		};
		// Basically the kernel returns pages from destroyed address spaces via an IPC message,
		// so I must be listening. 
		if(ipc.ipc.sys_ipc_recv(&recv) == 0) {
			const address = recv.value0;
			if(in_usable(memmap, address)) {
				const npages = recv.npages;
				allocator.dealloc(address, npages) catch continue;
			}
		}
	}
}

fn get_port() !i16 {
	var msg = ipc.ipc.ipc_message_t {
		.dest = 0,
		.flags = ipc.ipc.IPC_FLAG_TRANSFER_PORT
	};
	if(ipc.ipc.sys_ipc_recv(&msg) == ipc.ipc.EOK) {
		return msg.source;
	} else {
		return error.TRANSFER_ERROR;
	}
}

fn send_port(port: i16, dest: i16) !void {
	var msg = ipc.ipc.ipc_message_t {
		.dest = dest,
		.source = port,
		.flags = ipc.ipc.IPC_FLAG_TRANSFER_PORT
	};
	if(ipc.ipc.sys_ipc_send(&msg) != ipc.ipc.EOK) {
		return error.TRANSFER_ERROR;
	}
}

pub inline fn rdtsc() u64 {
	var low: u32 = undefined; var high: u32 = undefined;
    asm volatile("rdtsc":[low]"={eax}"(low),[high]"={edx}"(high));
    return (@as(u64, high) << 32) | low;
}

const N_ITERS = 1000;

fn test1() noreturn {
	_ = debug.debug_init(0);
	const otherport = get_port();
	if(otherport) |port| {
		const at_start = rdtsc();
		for(0..N_ITERS) |i| {
			const msg = ipc.ipc.ipc_message_t {
				.dest = port,
				.value0 = i+1
			};
			_ = ipc.ipc.sys_ipc_send(&msg);
		}
		const at_end = rdtsc();
		std.log.info("Cycles per IPC: {}", .{(at_end - at_start)/N_ITERS});
		std.log.info("Finished sending", .{});
		_ = sys.sys_kill_process(0);
		std.log.err("Process did not exit", .{});
		hcf();
	} else |err| {
		@panic(@errorName(err));
	}
}


fn test2() noreturn {
	_ = debug.debug_init(0);
	for(0..N_ITERS) |i| {
		var msg = ipc.ipc.ipc_message_t {
			.dest = 0,
			.value0 = 0
		};
		_ = ipc.ipc.sys_ipc_recv(&msg);
		std.debug.assert(msg.value0 == i+1);
	}
	std.log.info("Finished receiving", .{});
	_ = sys.sys_kill_process(0);
	std.log.err("Process did not exit", .{});
	hcf();
}

fn spawn_other(allocator: *allc.PageFrameAllocator) void {
	const firstport = sys.sys_create_process();
	if(firstport < 0) {
		@panic("Could not create process");
	}
	const N_PAGES_CHILD = 16;
	const stack = allocator.alloc(N_PAGES_CHILD) catch |err| {
		@panic(@errorName(err));
	};
	
	if(sys.sys_start_process(firstport, @constCast(&test1), @ptrFromInt(stack + N_PAGES_CHILD * bmp.PAGE_SIZE), sys.tls_reg_t {.fs = 0, .gs = 0}) != 0) {
		@panic("Could not start process");
	}

	const secondport = sys.sys_create_process();
	if(secondport < 0) {
		@panic("Could not create process");
	}
	const stack2 = allocator.alloc(N_PAGES_CHILD) catch |err| {
		@panic(@errorName(err));
	};
	
	if(sys.sys_start_process(secondport, @constCast(&test2), @ptrFromInt(stack2 + N_PAGES_CHILD * bmp.PAGE_SIZE), sys.tls_reg_t {.fs = 0, .gs = 0}) != 0) {
		@panic("Could not start process");
	}

	send_port(secondport, firstport) catch |err| {
		@panic(@errorName(err));
	};

	std.log.info("Spawned the other processes", .{});
}

pub fn main() void {
	if(debug.debug_init(0) != 0) {
		@panic("Initialization failed");
	}
	const memmap: *mmap.mmap_t = @ptrFromInt(mmap.MMAP_VIRTADDR);
	var entry: ?*mmap.mmap_basic_entry_t = null;
	for(0..memmap.size) |_| {
		entry = mmap.get_next_entry(memmap, entry);
		const addr = mmap.get_address(entry.?.addrflags);
		const flags = mmap.get_flags(entry.?.addrflags);
		const pages = entry.?.npages * bmp.PAGE_SIZE;
		const regmemtype = switch(flags) {
			2 => "Device",
			4 => "Framebuffer",
			8 => "Usable",
			else => "no"
		};
		std.log.info("(BITMAPMM) Bitmap region: {x}-{x} ({s})", .{addr, addr + pages, regmemtype});
	}
	const port = sys.sys_create_port();
	if(port < 0) {
		@panic("No port allocated");
	}
	const bitmap = bmp.bitmap.init_from_memmap(memmap) catch |err| {
		@panic(@errorName(err));
	};
	var allocator = allc.PageFrameAllocator.init(bitmap.bitmap, bitmap.start_addr, bitmap.n_pages) catch |err| {
		@panic(@errorName(err));
	};

	// Spawn the receiver process that gets the freed pages from recently killed processes
	const otherport = sys.sys_create_process();
	if(otherport < 0) {
		@panic("Could not create process");
	}
	const N_PAGES_CHILD = 16;
	const stack = allocator.alloc(N_PAGES_CHILD) catch |err| {
		@panic(@errorName(err));
	};
	
	if(sys.sys_start_process(otherport, @constCast(&receiver_loop), @ptrFromInt(stack + N_PAGES_CHILD * bmp.PAGE_SIZE), sys.tls_reg_t {.fs = 0, .gs = 0}) != 0) {
		@panic("Could not start process");
	}
	const alloc_msg = ipc.ipc.ipc_message_t {
		.dest = otherport,
		.value0 = @intFromPtr(&allocator),
		.value1 = @intFromPtr(memmap)
	};
	if(ipc.ipc.sys_ipc_send(&alloc_msg) != 0) {
		@panic("Can't send allocator to child, cannot function");
	}

	// Spawn the other processes
	spawn_other(&allocator);
	
	// And finally listen to allocations
	const fb_entry = get_fb_entry(memmap) orelse {
		@panic("No framebuffer entry found");
	};
	alloc_loop(&allocator, port, fb_entry);
}

test "empty" {
	const alloc = std.testing.allocator;
	const size = 8;
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 0;
	_ = bmp.bitmap.init_from_memmap(memmap) catch {
		return;
	};
	std.debug.assert(false);
}

test "one_region" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);
}

test "one_region_alloc" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);

	var allocator = try allc.PageFrameAllocator.init(bitm.bitmap, bitm.start_addr, 64);
	const alloc1 = try allocator.alloc(1);
	try allocator.dealloc(alloc1, 1);

}

test "one_region_alloc_one" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);

	var allocator = try allc.PageFrameAllocator.init(bitm.bitmap, bitm.start_addr, 64);
	for (1..63) |i| {
		const alloc1 = try allocator.alloc(i);
		try allocator.dealloc(alloc1, i);
	}
}

test "one_region_alloc_multiple" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);

	var allocator = try allc.PageFrameAllocator.init(bitm.bitmap, bitm.start_addr, 64);
	var alloc1 = try allocator.alloc(1);
	const initial = alloc1;
	defer allocator.dealloc(initial, 63) catch |err| {
		std.debug.print("{}", .{err});
		@panic(@errorName(err));
	};
	for(1..63) |_| {
		const alloc2 = try allocator.alloc(1);
		try std.testing.expectEqual(alloc2 - bmp.PAGE_SIZE, alloc1);

		alloc1 = alloc2;
	}

}

test "one_region_alloc_free_is_ok" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);

	var allocator = try allc.PageFrameAllocator.init(bitm.bitmap, bitm.start_addr, 64);
	const alloc1 = try allocator.alloc(63);
	try allocator.dealloc(alloc1, 63);
	const alloc2 = try allocator.alloc(63);
	try allocator.dealloc(alloc2, 63);
}

test "one_region_alloc_free_can_use" {
	const alloc = std.heap.page_allocator;
	const size = 24;
	const npages = 64;
	const region_size = bmp.PAGE_SIZE * npages;
	const raw_region = alloc.rawAlloc(region_size, .@"64", 0).?;
	defer alloc.rawFree(raw_region[0..region_size], .@"64", 0);
	const raw_memmap = alloc.rawAlloc(size, .@"8", 0).?;
	defer alloc.rawFree(raw_memmap[0..size], .@"8", 0);
	const memmap: *mmap.mmap_t = @ptrCast(@alignCast(raw_memmap));
	memmap.size = 1;
	const entry: *mmap.mmap_basic_entry_t = @ptrCast(&memmap.entries()[0]);
	entry.addrflags = @intFromPtr(raw_region) | mmap.MEM_USABLE;
	entry.npages = npages;

	const bitm = try bmp.bitmap.init_from_memmap(memmap);
	try std.testing.expectEqual((npages + @typeInfo(usize).int.bits - 1)/@typeInfo(usize).int.bits, bitm.bitmap.len);

	var allocator = try allc.PageFrameAllocator.init(bitm.bitmap, bitm.start_addr, 64);
	const alloc1 = try allocator.alloc(63);
	defer allocator.dealloc(alloc1, 63) catch {};
	if(allocator.alloc(1)) |_| {
		try std.testing.expect(false);
	} else |_| {
		try std.testing.expect(true); // No-op just to keep the branch happy xd
	}
	const bytes = 63*bmp.PAGE_SIZE;
	const buffer: []u8 = @as([*]u8, @ptrFromInt(alloc1))[0..bytes];
	var fba = std.heap.FixedBufferAllocator.init(buffer);
	const all = fba.allocator();

	var arrl = std.ArrayList(u64).init(all);
	defer arrl.deinit();
	// Not as much, just to test that I can use it. (Resizings are a bit of a pain ya know).
	const KB32 = 48*bmp.PAGE_SIZE/8;
	for(0..KB32) |_| {
		try arrl.append(7);
	}
	// if it did not die then it's alright
}