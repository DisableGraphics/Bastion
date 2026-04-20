# Physical Memory Management
This folder contains all the available physical memory manager servers that can be used in the core.
Of course, the core only supports one memory manager.
However, the algorithm for this memory manager may be useful for particular workloads, therefore all physical memory manager servers that may be implemented in this core must support the same protocol.
Language of implementation of the physical memory manager server is irrelevant.

## Protocol
This protocol is useful for both client-side applications and the implementation of the memory manager servers.

This section describes the communication diagrams between the client and the server. 

* ALLOC_REQUEST: Allocates one page.

Server					Client
  |		ALLOC_REQUEST	  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: `value0`: CMD_ALLOC_REQUEST, `value1`: Destination address (page aligned), `value2`: mapping options (as per libsyscall/ipc.h)
  |		ERROR_RESPONSE	  | Type: ipc message without flags
  |- - - - - - - - - - - >| Params: `value2`: Error code
  |		ALLOC_RESPONSE	  | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0
  |						  |

* ALLOC_RANGE_REQUEST: Allocates N contiguous pages.

Server					Client
  |		ALLOC_REQUEST	  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: `value0`: CMD_ALLOC_RANGE_REQUEST, `value1`: Destination address (page aligned), 
  |						  | `value2`: mapping options (as per libsyscall/ipc.h), `value3`: number of pages
  |		ERROR_RESPONSE	  | Type: ipc message without flags
  |- - - - - - - - - - - >| Params: `value2`: Error code
  |		ALLOC_RESPONSE	  | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0
  |						  |

* FREE_REQUEST: Frees one page

Server					Client
  |		 FREE_REQUEST	  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: `value0`: CMD_FREE_REQUEST, `value1`: Address to free (page aligned)
  |		PAGE_REVOKE		  | Type: ipc message with IPC_FLAG_REVOKE_PAGE (Invisible from the client perspective)
  |- - - - - - - - - - - -| Params: None
  |		FREE_RESPONSE	  | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0 if ok, error code if not ok
  |						  |

* FREE_RANGE_REQUEST: Frees N contiguous pages

Server					Client
  |		 FREE_REQUEST	  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: `value0`: CMD_FREE_RANGE_REQUEST, `value1`: Address to free (page aligned), `value2`: number of pages
  |		PAGE_REVOKE		  | Type: ipc message with IPC_FLAG_REVOKE_PAGE (Invisible from the client perspective)
  |- - - - - - - - - - - -| Params: None
  |		FREE_RESPONSE	  | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0 if ok, error code if not ok
  |						  |

* FB_REQUEST: Asks for the framebuffer.
> [!WARNING] First-come first-serve and pre-userspace will make sure to only give this opportunity to the FB driver. This is of NO use whatsoever to 99% of server developers. The remaining 1% is those who make a FB driver.

Server					Client
  |		 FB_REQUEST		  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: value0: CMD_FB_REQUEST, `value1`: Destination address (page aligned), `value2`: mapping options (as per libsyscall/ipc.h), 
  |		ERROR_RESPONSE	  | Type: ipc message without flags
  |- - - - - - - - - - - >| Params: `value2`: Error code
  |	   ALLOC_RESPONSE_1   | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0, 
  |	   ALLOC_RESPONSE_2   | Type ipc message without flags
  |---------------------->| Params: 
  |						  | `value0`: `width << 32 | height`, `value1`: `pitch << 32 | bpp`
  |						  | `value2`: `red_mask_size << 32 | green_mask_size`, 
  |						  | `value3`: `blue_mask_size << 32 | red_mask_disp << 24 | green_mask_disp << 16 | blue_mask_disp << 8`

What each field of ALLOC_RESPONSE_2 means:
- `width` (`u32`): Width in pixels of the framebuffer.
- `height` (`u32`): Height in pixels of the framebuffer.
- `pitch` (`u32`): Width in bytes of one row of the framebuffer. It may be different from `width*bpp`.
- `bpp` (`u32`): Bits per pixel of one pixel in the framebuffer.
- `red_mask_size` (`u32`): Size in bits of the red color mask.
- `green_mask_size` (`u32`): Size in bits of the green color mask.
- `blue_mask_size` (`u32`): Size in bits of the blue color mask.
- `red_mask_disp` (`u8`): Displacement (for a shift operation) to obtain the red mask.
- `green_mask_disp` (`u8`): Displacement (for a shift operation) to obtain the red mask.
- `blue_mask_disp` (`u8`): Displacement (for a shift operation) to obtain the red mask.

The idea of the masks and displacements is that you can draw a color into the framebuffer like this:
```c
pixels[idx] = (red & red_mask_size) << red_mask_disp | (green & green_mask_size) << green_mask_disp | (blue & blue_mask_size) << blue_mask_disp;
```

* DEV_REQUEST: Obtains device memory.
Device memory is not general purpose, therefore a separate command. To get device memory you need the physical address. This can be 
obtained from different methods such as BARs in PCI space. This operation is only useful for memory-mapped device drivers.

Server					Client
  |		 DEV_REQUEST	  | Type: ipc message with IPC_FLAG_TRANSFER_PORT
  |<----------------------| Params: `value0`: CMD_ALLOC_RANGE_REQUEST, `value1`: Physical address of device
  |						  |	`value2`: Destination address (page aligned), 
  |						  | `value3`: mapping options (as per libsyscall/ipc.h), `value4`: number of pages
  |		ERROR_RESPONSE	  | Type: ipc message without flags
  |- - - - - - - - - - - >| Params: `value2`: Error code
  |		ALLOC_RESPONSE	  | Type ipc message with IPC_FLAG_GRANT_PAGE
  |---------------------->| Params: `value2`: 0
  |						  |