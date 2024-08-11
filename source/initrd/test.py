import os 
import tarfile

with tarfile.open("ustar.tar", "w", format=tarfile.USTAR_FORMAT) as ramdisk:
    ramdisk.add("rootdir", arcname="/")
