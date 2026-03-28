const std = @import("std");

// Although this function looks imperative, note that its job is to
// declaratively construct a build graph that will be executed by an external
// runner.
pub fn build(b: *std.Build) void {
	// Standard target options allows the person running `zig build` to choose
	// what target to build for. Here we do not override the defaults, which
	// means any target is allowed, and the default is native. Other options
	// for restricting supported target set are available.

	var disabled_features = std.Target.Cpu.Feature.Set.empty;
	var enabled_features = std.Target.Cpu.Feature.Set.empty;

	disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.mmx));
	disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.sse));
	disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.sse2));
	disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.avx));
	disabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.avx2));
	enabled_features.addFeature(@intFromEnum(std.Target.x86.Feature.soft_float));

	const target_query = std.Target.Query{
		.cpu_arch = std.Target.Cpu.Arch.x86_64,
		.os_tag = std.Target.Os.Tag.freestanding,
		.abi = std.Target.Abi.none,
		.cpu_features_sub = disabled_features,
		.cpu_features_add = enabled_features
	};
	const target = b.resolveTargetQuery(target_query);
	const host_target = b.graph.host;

	// Standard optimization options allow the person running `zig build` to select
	// between Debug, ReleaseSafe, ReleaseFast, and ReleaseSmall. Here we do not
	// set a preferred release mode, allowing the user to decide how to optimize.
	const optimize = b.standardOptimizeOption(.{.preferred_optimize_mode = .ReleaseSafe});

	// We will also create a module for our other entry point, 'main.zig'.
	const exe_mod = b.createModule(.{
		.root_source_file = b.path("src/main.zig"),
		.optimize = optimize,
		.target = target
	});
	exe_mod.error_tracing = false;
	exe_mod.addIncludePath(b.path("../../../libsyscall"));
	exe_mod.addIncludePath(b.path("../"));
	exe_mod.addLibraryPath(b.path("../../../libsyscall/zig-out/lib/"));
	exe_mod.linkSystemLibrary("syscall", .{.preferred_link_mode = .static});
	exe_mod.linkSystemLibrary("ipc", .{.preferred_link_mode = .static});
	exe_mod.linkSystemLibrary("syswrap", .{.preferred_link_mode = .static});
	exe_mod.linkSystemLibrary("debug", .{.preferred_link_mode = .static});

	const test_mod = b.createModule(.{
		.root_source_file = b.path("src/main.zig"),
		.optimize = optimize,
		.target = host_target
	});
	test_mod.addIncludePath(b.path("../../../libsyscall"));

	// This creates another `std.Build.Step.Compile`, but this one builds an executable
	// rather than a static library.
	const exe = b.addExecutable(.{
		.name = "bitmapmm",
		.root_module = exe_mod,
	});
	exe.setLinkerScript(b.path("linker.ld"));
	
	// This declares intent for the executable to be installed into the
	// standard location when the user invokes the "install" step (the default
	// step when running `zig build`).
	b.installArtifact(exe);

	const exe_unit_tests = b.addTest(.{
		.name = "test",
		.root_module = test_mod,
		.target = host_target,
		.link_libc = true
	});
	b.installArtifact(exe_unit_tests);
	const run_exe_unit_tests = b.addRunArtifact(exe_unit_tests);
	

	// Similar to creating the run step earlier, this exposes a `test` step to
	// the `zig build --help` menu, providing a way for the user to request
	// running the unit tests.
	const test_step = b.step("test", "Run unit tests");
	test_step.dependOn(&run_exe_unit_tests.step);
}
