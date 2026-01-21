# How does the IPC system work
This documentation is for the Medusa microkernel application developers.

## Before you start
This documentation only applies to applications that need to interface directly with the Medusa microkernel. These include, but are not limited to, memory managers, syscall managers and other low-level basic functionality given by some servers.
If you're trying to develop an application for Bastion, please look at the Bastion documentation.

## Foreword
The IPC system is not just an IPC system. It's the interface for application developers to communicate with the kernel.

In fact, in there are only two different system calls:
- ipc_recv
- ipc_send

## Using this API
For C/C++/Zig: include the ipc.h file found in the include/ directory.

## How this API works
If you examine the API ipc.h header file, you can find a list of flags which can be used in the message to invoke a particular behaviour from the kernel.

First and foremost: flags usually cannot be combined. If more than one flag is used at the same time, the kernel does not guarantee that all the requested functionalities will be executed, and, if more than just one are executed, in which order. ALWAYS use only one flag at a time, unless it has been explicitly written in the docs that it can be used in tandem with other flags.

## ipc_recv vs ipc_send
ipc_recv allows modification of the message, whereas ipc_send does not. That is why process creation, port creation and waiting for a process is done via ipc_recv.
ipc_send is the one that handles all the other functionalities.