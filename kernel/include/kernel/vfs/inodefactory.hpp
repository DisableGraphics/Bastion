#pragma once
#include <kernel/vfs/inodes/inode.hpp>
#include <kernel/vfs/inodes/physicalinode.hpp>
#include <kernel/vfs/inodes/pipeinode.hpp>

enum INODE_CREAT {
	PHYSICAL,
	PIPE
};

class InodeFactory {
	public:
		template<typename... Args>
		static Inode* create(INODE_CREAT type, Args&&...);
};
template<typename... Args>
Inode* InodeFactory::create(INODE_CREAT type, Args&&... args) {
	switch(type) {
		case PHYSICAL:
			return new PhysicalInode(forward(args...));
		case PIPE:
			return new PipeInode(forward(args...));
	}
}