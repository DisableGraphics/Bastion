#!/usr/bin/env python3
import sys
import subprocess

kernel_elf = sys.argv[1]
output_file = sys.argv[2]

nm_output = subprocess.check_output(['nm', '-n', kernel_elf]).decode()

symbols = []
for line in nm_output.splitlines():
	if ' T ' in line:
		addr, _type, name = line.split()
		symbols.append((int(addr, 16), name))

with open(output_file, 'w') as out:
	out.write('#include "../kernel/include/kernel/kernel/stable.h"\n\n')
	out.write('#include <stddef.h>\n\n')
	out.write('symbol_t kernel_symbols[] = {\n')
	for addr, name in symbols:
		out.write(f'    {{0x{addr:08x}, "{name}"}},\n')
	out.write('};\n\n')

	out.write(f'const size_t kernel_symbol_count = {len(symbols)};\n')
	out.write('''const char* find_function_name(size_t ip) {
	if(ip < kernel_symbols[0].address) return "<unknown>";
	for (size_t i = 0; i < kernel_symbol_count - 1; i++) {
		if (ip >= kernel_symbols[i].address &&
			ip < kernel_symbols[i + 1].address) {
			return kernel_symbols[i].name;
		}
	}
	return kernel_symbols[kernel_symbol_count - 1].name;
}
''')
