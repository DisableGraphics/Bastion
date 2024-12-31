#include <kernel/random/rand.hpp>
#include <stdint.h>
#include <cpuid.h>

#define RDSEED_POS 0
#define RDRAND_POS 1

bool available_rands[2]{false, false};
bool has_init = false;

void check_available();

void init_random() {
	has_init = true;
	check_available();
}

void check_available() {
	int ebx, unused;
    __cpuid(7, unused, ebx, unused, unused);
	available_rands[0] = ebx & bit_RDSEED;
	int ecx;
	__cpuid(1, unused, unused, ecx, unused);
	available_rands[1] = ecx & bit_RDRND;
}

int krand_internal() {
	int random_value = -1;
	int retries = 100;
	if(available_rands[0]) {
		__asm__ (
			"movl %1, %%ecx;"           // Set the number of retries to ecx
		"1: "                          // Retry label (local label 1)
			"rdseed %%eax;"             // Generate a random number and store it in eax
			"jc 2f;"                    // Jump to label 2 if carry flag is set (success)
			"loop 1b;"                  // Decrement ecx and repeat the loop if ecx > 0
		"3: "                          // Fail label (local label 3)
			"movl $0, %0;"              // Set random_value to 0 if failed
			"jmp 2f;"                   // Jump to done label 2
		"2: "                          // Done label (local label 2)
			"movl %%eax, %0;"           // Store the value of eax (random number) in random_value
			: "=r"(random_value)        // Output: random_value stores the result in eax
			: "r"(retries)              // Input: retries value passed to ecx
			: "%eax", "%ecx"            // Clobbered registers
		);
	} else if(available_rands[1]) {
		__asm__ (
			"movl %1, %%ecx;"           // Set the number of retries to ecx
		"1: "                          // Retry label (local label 1)
			"rdrand %%eax;"             // Generate a random number and store it in eax
			"jc 2f;"                    // Jump to label 2 if carry flag is set (success)
			"loop 1b;"                  // Decrement ecx and repeat the loop if ecx > 0
		"3: "                          // Fail label (local label 3)
			"movl $0, %0;"              // Set random_value to 0 if failed
			"jmp 2f;"                   // Jump to done label 2
		"2: "                          // Done label (local label 2)
			"movl %%eax, %0;"           // Store the value of eax (random number) in random_value
			: "=r"(random_value)        // Output: random_value stores the result in eax
			: "r"(retries)              // Input: retries value passed to ecx
			: "%eax", "%ecx"            // Clobbered registers
		);
	}
	return random_value;
}

int ktrand(void) {
	if(!has_init) init_random();
	return krand_internal();
}