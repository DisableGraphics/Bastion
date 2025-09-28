const sch = @import("scheduler.zig");
const page = @import("../memory/pagemanager.zig");
const frame = @import("../memory/kmm.zig");
const ts = @import("../memory/tss.zig");
const main = @import("../main.zig");

pub const SchedulerManager = struct {
	var schedulers: []sch.Scheduler = undefined;
	pub fn ginit(cpus: u64, allocator: *frame.KernelMemoryManager) !void {
		const pages = ((cpus * @sizeOf(sch.Scheduler)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		schedulers = @as([*]sch.Scheduler, @ptrFromInt((try allocator.alloc_virt(pages)).?))[0..cpus];
		for(0..cpus) |i| {
			schedulers[i] = sch.Scheduler.init(ts.get_tss(@truncate(i)));
		}
	}

	pub fn get_scheduler_for_cpu(cpuid: u64) *sch.Scheduler {
		return &schedulers[cpuid];
	}

	pub fn on_irq(arg: ?*anyopaque) void {
		_ = arg;
		const cpuid = main.mycpuid();
		schedulers[cpuid].schedule();
	}
};

pub export fn unlock_scheduler_from_context_switch() callconv(.C) void {
	SchedulerManager.get_scheduler_for_cpu(main.mycpuid()).unlock();
}