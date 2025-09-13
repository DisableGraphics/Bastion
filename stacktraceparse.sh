#!/bin/sh
found=0
while IFS= read -r line; do
  if [[ $found -eq 0 ]]; then
    # Check for stack trace
    if [[ $line == Error:* ]]; then
		result="${line#*at }"
		echo $(addr2line -e zig-out/bin/kernel.elf $result)
      	found=1
    fi
  else
	# Parse stack trace
    field=$(echo "$line" | awk '{print $3}')
	if [[ $field == 0xffffffff8* ]]; then
		echo $(addr2line -e zig-out/bin/kernel.elf $field)
	fi
  fi
done < bastion.serial
