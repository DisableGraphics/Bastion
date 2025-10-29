const slab = @import("../memory/slaballoc.zig");
const main = @import("../main.zig");
const kmm = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const std = @import("std");

pub const PageTableAllocator = struct {
	var allocators: []slab.SlabAllocator(page.page_table_type) = undefined;
	pub fn init(expected_tasks: u64, mp_cores: u64, km: *kmm.KernelMemoryManager) !void {
		const allocators_pages = ((mp_cores * @sizeOf(slab.SlabAllocator(page.page_table_type))) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
		allocators = @as([*]slab.SlabAllocator(page.page_table_type), @ptrFromInt((try km.alloc_virt(allocators_pages)).?))[0..mp_cores];

		// Force at least 64 page tables per core
		const expected_tasks_per_core = @max(64, (expected_tasks + mp_cores - 1) / mp_cores);
		for(0..mp_cores) |i| {
			const expected_tasks_pages = ((expected_tasks_per_core * @sizeOf(page.page_table_type)) + page.PAGE_SIZE - 1) / page.PAGE_SIZE;
			const ntasks_expected = expected_tasks_pages * page.PAGE_SIZE;
			const buffer = @as([*]u8, @ptrFromInt((try km.alloc_virt(expected_tasks_pages)).?))[0..ntasks_expected];
			allocators[i] = try slab.SlabAllocator(page.page_table_type).init(buffer);
			try allocators[i].init_region();
		}

	}

	pub fn alloc() ?*page.page_table_type {
		const myid = main.mycpuid();
		const buf = allocators[myid].alloc();
		if(buf == null) return null;
		@memset(buf.?, 0);
		return buf;
	}
	pub fn alloc_pml5_t() ?*page.pml5_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_pml4_t() ?*page.pml4_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_pml3_t() ?*page.pml3_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_pml2_t() ?*page.pml2_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_huge_page_t() ?*page.huge_page_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_large_page_t() ?*page.large_page_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn alloc_page_t() ?*page.page_t {
		return @ptrCast(PageTableAllocator.alloc());
	}

	pub fn free(ts: *page.page_table_type) !void {
		const myid = main.mycpuid();
		return allocators[myid].free(ts);
	}

	pub fn free_pml5_t(ts: *page.pml5_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_pml4_t(ts: *page.pml4_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_pml3_t(ts: *page.pml3_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_pml2_t(ts: *page.pml2_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_huge_page_t(ts: *page.huge_page_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_large_page_t(ts: *page.large_page_t) !void {
		return free(@ptrCast(ts));
	}

	pub fn free_page_t(ts: *page.page_t) !void {
		return free(@ptrCast(ts));
	}

};