#!/bin/sh
zig build -freference-trace=16 -- -mnored-zone -fno-stack-protector 