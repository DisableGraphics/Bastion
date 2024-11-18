#include <multiboot/multiboot.h>

class MemoryManager {
	public:
		static MemoryManager &get();
		void init(multiboot_info_t* mbd, unsigned int magic);
	private:
		MemoryManager(){};
};