const schman = @import("manager.zig");
const cpuid = @import("../main.zig");
const spin = @import("../sync/spinlock.zig");

const LoadBalancer = struct {
	var lock = spin.SpinLock.init();
	fn find_next_victim() u32 {
		lock.lock();
		defer lock.unlock();
		const myid = cpuid.mycpuid();
		for(0..schman.SchedulerManager.schedulers.len) |i| {
			if(i != myid) {
				
			}
		}
	}
};