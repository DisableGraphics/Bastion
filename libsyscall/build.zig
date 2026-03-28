const std = @import("std");

const c_flags = &[_][]const u8{"-std=c99", "-ffreestanding", "-nostdlib"};

pub fn build_syscall(b: *std.Build, target: std.Build.ResolvedTarget, mode: std.builtin.OptimizeMode) void {
	const syscall_mod = b.addModule("syscall", .{
		.target = target,
		.optimize = mode,
		.root_source_file = null
	});
	const syscall = b.addLibrary(.{
		.root_module = syscall_mod,
		.name = "syscall",
		.linkage = .static,
	});

	switch(target.result.cpu.arch) {
		.x86_64 => {
			syscall_mod.addAssemblyFile(b.path("syscall.S"));
		},
		else => {}
	}
	b.installArtifact(syscall);
}

pub fn build_ipc(b: *std.Build, target: std.Build.ResolvedTarget, mode: std.builtin.OptimizeMode) void {
	const ipc_mod = b.addModule("ipc", .{
		.target = target,
		.optimize = mode,
		.root_source_file = null
	});

	ipc_mod.addCSourceFile(.{ 
		.file = b.path("ipc.c"),
		.flags = c_flags
	});

	const ipc = b.addLibrary(.{
		.root_module = ipc_mod,
		.name = "ipc",
		.linkage = .static,
	});
	b.installArtifact(ipc);
}

pub fn build_syswrap(b: *std.Build, target: std.Build.ResolvedTarget, mode: std.builtin.OptimizeMode) void {
	const syswrap_mod = b.addModule("syswrap", .{
		.target = target,
		.optimize = mode,
		.root_source_file = null
	});
	const syswrap = b.addLibrary(.{
		.root_module = syswrap_mod,
		.name = "syswrap",
		.linkage = .static,
	});

	syswrap.addCSourceFile(.{ 
		.file = b.path("syswrap.c"),
		.flags = c_flags
	});
	
	b.installArtifact(syswrap);
}

pub fn build_debug(b: *std.Build, target: std.Build.ResolvedTarget, mode: std.builtin.OptimizeMode) void {
	const debug_mod = b.addModule("debug", .{
		.target = target,
		.optimize = mode,
		.root_source_file = null
	});
	const debug = b.addLibrary(.{
		.root_module = debug_mod,
		.name = "debug",
		.linkage = .static,
	});

	debug.addCSourceFile(.{ 
		.file = b.path("debug.c"),
		.flags = c_flags
	});
	
	b.installArtifact(debug);
}

pub fn build(b: *std.Build) void {
	const target = b.standardTargetOptions(.{
		.default_target = .{
			.abi = .none,
			.os_tag = .freestanding
		}
	});
	const mode = b.standardOptimizeOption(.{});

	build_syscall(b, target, mode);
	build_ipc(b, target, mode);
	build_syswrap(b, target, mode);
	build_debug(b, target, mode);
}