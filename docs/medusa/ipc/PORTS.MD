# Port
A port is an endpoint for two separate but related functionalities:
1) Interfacing with the kernel.
2) Communication between processes.

A port is a *destination* for an operation. This operation can be applied to the port, the port's owner (a process), or an address space linked to the port.

# Working with ports

## How does a port work
A port allows to:
- Communicate with a process via synchronous bufferless IPC (Inter Process Communication).
- Interface with the kernel via pseudosyscalls.

### The bootstrap port
The bootstrap port is the first port of a new process when it is created, and has some notable characteristics:
- The parent process receives this port when the child process is created.
- The child process (who technically owns it) cannot do these operations in this port:
	- Change rights.
	- Destroy it.
- The port has all rights (except `IPC_RIGHT_MODIF_RIGHTS`) enabled.

## IPC
The most basic action a process can do with a port is IPC. A process can send or receive 1 message. The message struct is defined in `libsyscall/ipc.h`.


### Sending a message
To send a message the sending process needs to know the destination port. The task of sending this port is left as an exercise to the reader (Hint: it may involve the flag `IPC_FLAG_TRANSFER_PORT`). A process can only send messages on ports with `IPC_RIGHT_SEND` rights or if it's the owner.

```c
port_t destination_port = receive_port_from_who_knows_where();
ipc_message_t msg {
	.dest = destination_port,
	.value0 = 15
};
if(sys_ipc_send(&msg) != EOK) {
	// Handle error
} else {
	// Message was sent
}
```

### Receiving a message
To receive a message the receiving process needs to know the receiving port. The task of receiving this port is left as an exercise to the reader (Hint: it may involve the flag `IPC_FLAG_TRANSFER_PORT`). A process can only receive messages on ports with `IPC_RIGHT_RECV` rights or if it's the owner.

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.dest = destination_port
};
if(sys_ipc_send(&msg) != EOK) {
	// Handle error
} else {
	// Message was received correctly
	printf("%d\n", msg.value0);
}
```

## [Pseudosyscalls](../sysc/SYSCALL.MD)

### IPC_FLAG_NONBLOCKING
The flag `IPC_FLAG_NONBLOCKING` allows to try to send or receive a message. If the destination (if sending) or source (if receiving) is not prepared to send the message it returns immediately and sys_ipc_* will return `ENODEST`.

Example:
```c
port_t destination_port = my_port;
ipc_message_t msg {
	.dest = destination_port,
	.flags = IPC_FLAG_NONBLOCKING,
	.value0 = 0xDEADBEEF
};
int ecode = sys_ipc_send(&msg);
if(ecode != EOK) {
	if(ecode == ENODEST) {
		printf("The destination is not listening\n");
	} else {
		// Handle error
	}
} else {
	// Message was received correctly
	printf("%d\n", msg.value0);
}
```

### IPC_FLAG_GRANT_RIGHTS
The flag `IPC_FLAG_GRANT_RIGHTS` allows to grant rights to an owned port or a port with the right `IPC_RIGHT_MODIF_RIGHTS`.

This pseudosyscall doesn't send a message.

> [!WARNING]: A process cannot grant rights to its bootstrap port. 

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.dest = destination_port,
	.flags = IPC_FLAG_GRANT_RIGHTS,
	.value0 = IPC_RIGHT_SEND | IPC_RIGHT_KILL
};
int ecode = sys_ipc_send(&msg);
if(ecode != EOK) {
	// Handle error
} else {
	// Permissions changed successfully
}
```

### IPC_FLAG_REVOKE_RIGHTS
The flag `IPC_FLAG_REVOKE_RIGHTS` allows to revoke rights from an owned port or a port with the right `IPC_RIGHT_MODIF_RIGHTS`.

This pseudosyscall doesn't send a message.

> [!WARNING]: A process cannot revoke rights from its bootstrap port. 

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.dest = destination_port,
	.flags = IPC_FLAG_REVOKE_RIGHTS,
	.value0 = IPC_RIGHT_KILL
};
int ecode = sys_ipc_send(&msg);
if(ecode != EOK) {
	// Handle error
} else {
	// Permissions changed successfully
}
```

### IPC_FLAG_TRANSFER_PORT
The flag `IPC_FLAG_TRANSFER_PORT` allows to send a port along with a message to a destination.

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.dest = destination_port,
	.flags = IPC_FLAG_TRANSFER_PORT,
	.value0 = 0x1337B33F
};
int ecode = sys_ipc_send(&msg);
if(ecode != EOK) {
	// Handle error
} else {
	// Message + port sent
}
```

### IPC_FLAG_CREATE_PORT
The flag `IPC_FLAG_CREATE_PORT` allows to create a new port. The owner is the process that runs the CREATE_PORT operation.

This pseudosyscall doesn't send a message.

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.flags = IPC_FLAG_CREATE_PORT,
};
int ecode = sys_ipc_recv(&msg);
if(ecode != EOK) {
	// Handle error
} else {
	// Port received
	port_t mynewport = msg.value0;
}
```

### IPC_FLAG_DESTROY_PORT
The flag `IPC_FLAG_DESTROY_PORT` allows to destroy a port. If the process that destroys the port is the owner then the rest of processes that were listening will be awakened with an ENODEST error code.

This pseudosyscall doesn't send a message.

> [!WARNING]: A process cannot destroy its bootstrap port. 

```c
port_t destination_port = my_port;
ipc_message_t msg {
	.flags = IPC_FLAG_DESTROY_PORT,
};
int ecode = sys_ipc_recv(&msg);
if(ecode != EOK) {
	// Handle error
} else {
	// Port received
	port_t mynewport = msg.value0;
}
```