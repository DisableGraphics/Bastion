```
sudo udisksctl loop-setup -f disk.img
gparted /dev/loop0
sudo udisksctl loop-delete -b /dev/loop0
```
