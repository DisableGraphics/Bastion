const elf = @import("std").elf;
const limine = @import("limine");
const std = @import("std");
const builtin = @import("builtin");
const km = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");
const asp = @import("../memory/addrspace.zig");
const aspa = @import("../memory/addrspacealloc.zig");
const main = @import("../main.zig");

pub const MManagerData = struct {
	addrsp: *asp.AddressSpace,
	start_vaddr: usize
};

pub const LoadModule = struct {
	pub fn load_mmanager(mod: *limine.ModuleResponse) !MManagerData {
		return load_elf_mmanager(mod.getModules()[0]);
	}

	// Checks if memory manager is supported and panics otherwise
	// It panics because I don't want to run just the kernel without any program loaded
	fn check_supported_x86_64(header: *const elf.Header) !void {
		if(!header.is_64) return error.NOT_64_BITS_EXEC;
		if(header.endian != .little) return error.INCORRECT_ENDIANESS;
		if(header.machine != .X86_64) return error.NON_SUPPORTED_MACHINE;
		if(header.type != .EXEC) return error.NOT_A_NON_RELOCATABLE_EXECUTABLE; // Must be a non-relocatable executable
	}

	// Gets mapping options from the ELF segment flags field
	fn get_opts_from_flags(flags: usize) struct {
		r: u1,
		w: u1,
		exec: u1,
	} {
		return .{
			.exec = @intFromBool(flags & 1 != 0),
			.w = @intFromBool(flags & 2 != 0),
			.r = @intFromBool(flags & 4 != 0)
		};
	}

	fn load_elf_mmanager(file: *limine.File) !MManagerData {	
		const file_slice: []u8 = @as([*]u8, @ptrCast(file.address))[0..file.size];
		var reader = std.io.fixedBufferStream(file_slice);
		const header = try elf.Header.read(&reader);

		try check_supported_x86_64(&header);

		const destaddrspace = aspa.AddressSpaceAllocator.alloc() orelse return error.NO_SPACE_LEFT;
		destaddrspace.* = asp.AddressSpace.init(main.km.pm);
		
		var it = header.program_header_iterator(&reader);
		while(it.next() catch |err| { return err; }) |v| {
			if(v.p_type == 1) { // pt_load which means: load into memory
				const dst1 = (v.p_vaddr & ~@as(usize, (page.PAGE_SIZE - 1)));
				const dst2 = ((v.p_vaddr + v.p_memsz + page.PAGE_SIZE - 1) & ~@as(usize, (page.PAGE_SIZE - 1)));
				const npages = (dst2 - dst1) / page.PAGE_SIZE;
				for(0..npages) |i| {
					const physaddr = ((try main.km.alloc_virt(1)) orelse return error.NO_PAGES_FREE) - main.km.hhdm_offset;
					const opts = get_opts_from_flags(v.p_flags);
					const page_vaddr = dst1 + i * page.PAGE_SIZE;
					try destaddrspace.add_mapping_4k(physaddr, page_vaddr,
						.{.p = 1, .us = 1, .rw = opts.w, .xd = @intFromBool(opts.exec == 0)},
						.{.p = 1, .us = 1, .rw = 1},
						.{.p = 1, .us = 1, .rw = 1},
						.{.p = 1, .us = 1, .rw = 1});
					const dstmem = @as([*]u8, @ptrFromInt(physaddr + main.km.hhdm_offset))[0..page.PAGE_SIZE];

					@memset(dstmem, 0);

					const page_start = page_vaddr;
					const page_end = page_vaddr + page.PAGE_SIZE;

					const seg_file_start = v.p_vaddr;
					const seg_file_end = v.p_vaddr + v.p_filesz;

					const copy_start = @max(page_start, seg_file_start);
					const copy_end = @min(page_end, seg_file_end);

					if (copy_start < copy_end) {
						const dst_off = copy_start - page_start;
						const src_off = (copy_start - v.p_vaddr) + v.p_offset;
						const len = copy_end - copy_start;

						const srcmem = file_slice[src_off .. src_off + len];
						@memcpy(dstmem[dst_off..dst_off + len], srcmem);
					}
					try main.km.pm.unmap(main.km.pm.root_table.?, physaddr + main.km.hhdm_offset);
				}
				std.log.info("0x{x} 0x{x} 0x{x}, 0x{x} {}", .{dst1, dst2, v.p_vaddr, v.p_vaddr + v.p_memsz, npages});
			}
		}
		return .{
			.addrsp = destaddrspace,
			.start_vaddr = header.entry
		};
	}
};