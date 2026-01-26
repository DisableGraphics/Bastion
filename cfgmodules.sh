#!/bin/sh
if [ ! -f "modules.lst" ]; then
	cp default_modules.lst modules.lst
fi
cargo run --manifest-path=bs/Cargo.toml