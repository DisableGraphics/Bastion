# Physical Memory Management
This folder contains all the available physical memory manager servers that can be used in the core.
Of course, the core only supports one memory manager.
However, the algorithm for this memory manager may be useful for particular workloads, therefore all physical memory manager servers that may be implemented in this core must support the same protocol.
Language of implementation of the physical memory manager server is irrelevant.

## Protocol
This protocol is useful for both client-side applications and the implementation of the memory manager servers.

This section will only be used to describe potential workloads.