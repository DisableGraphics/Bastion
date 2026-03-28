#ifndef _BASTION_PMM
#define _BASTION_PMM
#ifdef __cplusplus
extern "C" {
#endif

/**
	Client-Side API that the Physical Memory Manager server must implement
	WARNING: ANY AND ALL MESSAGES SENT TO THE PMM MUST BE SENT WITH THE IPC_PORT_TRANSFER flag
*/

/// Constants (Memory Allocation)
//// Commands
// Request to allocate 1 page
#define CMD_ALLOC_REQUEST 1
// Request to free 1 page
#define CMD_FREE_REQUEST 2
// Request to allocate a range of n pages
#define CMD_ALLOC_RANGE_REQUEST 3
// Request to free a range of n pages
#define CMD_FREE_RANGE_REQUEST 4
//// Responses
// Ok
#define RES_OK 0
// There has been an error processing the request
#define RES_ERR 1
// Operation in not supported
#define RES_NOT_SUPPORTED 2
// Out of memory
#define RES_OOM 3

/// Constants (Memory Petition)
//// Commands
// Request the memory address of the framebuffer (i.e. VESA/GOP in x86_64)
#define CMD_FB_REQUEST 1024
// Request to map device memory
#define CMD_DEV_REQUEST 1025
//// Responses (Same as the constants)

#ifdef __cplusplus
}
#endif
#endif