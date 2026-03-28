const std = @import("std");
const mmap = @cImport(@cInclude("memmap.h"));
const mman = @cImport(@cInclude("mmanager.h"));
const bmp = @import("bitmap.zig");
const builtin = @import("builtin");
const ipc = @import("ipc.zig");
const log = @import("log.zig");
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

fn alloc_loop(bitmap: *bmp.bitmap, port: ipc.ipc.port_t) void {
	while(true) {
		const command = ipc.receive_command(port);

		switch(command) {
			.cmd => |com| {
				switch(com.cmd) {
					.ALLOC_REQUEST => {
						const allocation = bitmap.alloc(1) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source);
							sys.sys_destroy_port(com.source);
							continue;
						};
						ipc.respond_with_allocation(com.source, allocation, 1);
						sys.sys_destroy_port(com.source);
					},
					.FREE_REQUEST => {
						const addr = com.arg0;
						bitmap.free(addr, 1) catch |err| {
							std.log.err("{}", .{err});
							ipc.respond_with_error(com.source);
							sys.sys_destroy_port(com.source);
						};
						ipc.respond_ok(com.source);
						sys.sys_destroy_port(com.source);
					},
					else => {
						std.log.err("Command not recognized: {}", .{com.cmd});
						ipc.respond_with_error(com.source);
						sys.sys_destroy_port(com.source);
					}
				}
			},
			.ecode => |err| {
				std.log.err("Error while processing command: {s}", .{err});
			}
		}
	}
}

pub fn main() void {
	if(debug.debug_init(0) != 0) {
		@panic("Initialization failed");
	}
	const memmap: *mmap.mmap_t = @ptrFromInt(mmap.MMAP_VIRTADDR);
	for(0..memmap.size) |x| {
		const addr = mmap.get_address(memmap.entries()[x].addrflags);
		const pages = memmap.entries()[x].npages * 4096;
		const regmemtype = switch(mmap.get_flags(memmap.entries()[x].addrflags)) {
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
	var bitmap = bmp.bitmap.init_from_memmap(memmap);
	alloc_loop(&bitmap, port);
}