const elf = @import("std").elf;
const limine = @import("limine");
const std = @import("std");
const builtin = @import("builtin");
const km = @import("../memory/kmm.zig");
const page = @import("../memory/pagemanager.zig");

pub const LoadModule = struct {
	pub fn load_mmanager(mod: *limine.ModuleResponse) void {
		load_elf_mmanager(mod.getModules()[0]);
	}

	// Checks if memory manager is supported and panics otherwise
	// It panics because I don't want to run just the kernel without anything else
	fn check_supported(header: *elf.Ehdr) void {
		std.debug.assert(std.mem.eql(u8, header.e_ident[0..4], &[_]u8{0x7f, 'E', 'L', 'F'}));

		std.debug.assert(header.e_ident[elf.EI_CLASS] == 2); // 64 bits
		std.debug.assert(header.e_ident[elf.EI_DATA] == 1); // Little endian (on x86_64)
		std.debug.assert(header.e_ident[elf.EI_VERSION] == 1); // I only support ELF v1
		const header_eident = switch(builtin.cpu.arch) {
			.x86_64 => 0x3E,
			else => unreachable
		};
		std.debug.assert(header.e_machine == header_eident);
		std.debug.assert(header.e_type == 2); // Must be a non-relocatable executable
	}

	fn load_executable() void {

	}

	fn sheader(header: *elf.Ehdr) [*]elf.Shdr {
		return @ptrFromInt(@intFromPtr(header) + header.e_shoff);
	}

	fn section(header: *elf.Ehdr, idx: usize) *elf.Shdr {
		return &sheader(header)[idx];
	}

	fn str_table(header: *elf.Ehdr) ?[*]u8 {
		if(header.e_shstrndx == elf.SHN_UNDEF) return null;
		return @ptrCast(header + section(header, header.e_shstrndx).sh_offset);
	}

	fn lookup_string(header: *elf.Ehdr, offset: usize) ?[*:0]u8 {
		const strtab = str_table(header);
		if(strtab) |tab| {
			return @ptrCast(tab + offset);
		} else {
			return null;
		}
	}

	fn get_symval(header: *elf.Ehdr, table: usize, idx: usize) !usize {
		if(table == elf.SHN_UNDEF or idx == elf.SHN_UNDEF) return 0;
		const shdr = section(header, table);

		const n_entries = shdr.sh_size/shdr.sh_entsize;
		if(idx >= n_entries) return error.OUT_OF_RANGE;

		const sym: *elf.Sym = @as([*]*elf.Sym, @ptrFromInt(@intFromPtr(header) + shdr.sh_offset))[idx];
		if(sym.st_shndx == elf.SHN_UNDEF) {
			//const strtab = section(header, shdr.sh_link);
			//const name: [*:0]u8 = @ptrFromInt(@intFromPtr(header) + strtab.sh_offset + sym.st_name);
			//const target = null;
			//if(target) |tgt| {
				//if(sym.st_info & elf.STB_WEAK) {
					//return 0;
				//} else {
					//return error.RELOC_ERROR;
				//}
			//}
			//return @intFromPtr(target);
			return 0;
		} else if(sym.st_shndx == elf.SHN_ABS) {
			return sym.st_value;
		} else {
			const target = section(header, sym.st_shndx);
			return @intFromPtr(header) + sym.st_value + target.sh_offset;
		}
	}

	fn load_stage_1(header: *elf.Ehdr, alloc: *km.KernelMemoryManager) !usize {
		const shdr = sheader(header);
		for(0..header.e_shnum) |i| {
			const sect = shdr[i];
			if(sect.sh_type == elf.SHT_NOBITS) {
				if(sect.sh_size == 0) continue;
				if(sect.sh_flags & elf.SHF_ALLOC) {
					const mem = (try alloc.alloc_virt((sect.sh_size + page.PAGE_SIZE - 1) / page.PAGE_SIZE)) orelse return error.NO_MEMORY;
					const memory: []u8 = @as([*]u8, @ptrFromInt(mem))[0..sect.sh_size];
					@memset(memory, 0);
					sect.sh_offset = mem - @intFromPtr(header);
				}
			}
		}
	}

	fn load_elf_mmanager(file: *limine.File) void {
		const header: *elf.Ehdr = @ptrCast(file.address);
		// In addr should be the ELF header
		std.log.info("{}", .{header});
		
		check_supported(header);
		
	}
};