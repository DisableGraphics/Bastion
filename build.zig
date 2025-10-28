const std = @import("std");

pub fn build(b: *std.Build) void {
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
        .cpu_features_add = enabled_features,
    };
    const optimize = b.standardOptimizeOption(.{.preferred_optimize_mode = .ReleaseSafe});

    const kernel = b.addExecutable(.{
        .name = "kernel.elf",
        .root_module = b.createModule(
		.{
			.root_source_file = b.path("src/kernel/main.zig"),
        	.target = b.resolveTargetQuery(target_query),
        	.optimize = optimize,
        	.code_model = .kernel
		})
    });
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/boot/boot.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/gdt/reloadSegments.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/gdt/setCr3.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/scheduler/switch_to_task.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/vector/enable_sse.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/vector/disable_sse.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/vector/fxsave.S"));
	kernel.addAssemblyFile(b.path("src/kernel/arch/x86_64/vector/fxrstor.S"));

	const limine_zig = b.dependency("limine_zig", .{
		// The API revision of the Limine Boot Protocol to use, if not provided
		// it defaults to 0. Newer revisions may change the behavior of the bootloader.
		.api_revision = 3,
		// Whether to allow using deprecated features of the Limine Boot Protocol.
		// If set to false, the build will fail if deprecated features are used.
		.allow_deprecated = false,
		// Whether to expose pointers in the API. When set to true, any field
		// that is a pointer will be exposed as a raw address instead.
		.no_pointers = false,
	});

	// Get the Limine module
	const limine_module = limine_zig.module("limine");

	// Import the Limine module into the kernel
	kernel.root_module.addImport("limine", limine_module);

    kernel.setLinkerScript(b.path("src/kernel/arch/x86_64/linker.ld"));
    b.installArtifact(kernel);


    const kernel_step = b.step("kernel", "Build the kernel");
    kernel_step.dependOn(&kernel.step);
}
