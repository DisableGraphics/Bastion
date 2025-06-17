#include <stdio.h>
#include <distorm/print_disassemble.h>
#include <distorm/distorm.h>
#include <kernel/kernel/log.hpp>

void print_disassemble(void *addr, size_t n, void* current_eip) {
	_DecodeResult res;
	_OffsetType offset = 0;
	_DecodedInst *decodedInstructions = new _DecodedInst[n];
	unsigned int decodedInstructionsCount = 0;
	_DecodeType dt = Decode32Bits;
	const unsigned char* buf = reinterpret_cast<unsigned char*>(addr);
	res = distorm_decode(offset, buf, n << 2, dt, decodedInstructions, n, &decodedInstructionsCount); 
	if (res == DECRES_INPUTERR) {
		printf("Array was too small: %d\n", n);
	}

	for (size_t i = 0; i < decodedInstructionsCount; i++) {
		size_t offset = decodedInstructions[i].offset + (size_t)addr;
		size_t size = decodedInstructions[i].size;
		char* mnemonic = (char*)decodedInstructions[i].mnemonic.p;
		size_t operands_length = decodedInstructions[i].operands.length;
		char* operands = (char*)decodedInstructions[i].operands.p;
		log(INFO, "%p (%d bytes) %s%s%s%s",
			offset, 
			size, 
			mnemonic, 
			operands_length != 0 ? " " : "", 
			operands,
			(void*)offset == current_eip ? "                <-------- EIP":""); 
	}


	delete[] decodedInstructions;
}