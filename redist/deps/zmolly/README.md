zmolly
======

zmolly is a generic data compressor with high compression ratio. it is based on LZP/PPM algorithm.

Simple benchmark with **enwik8**(100,000,000 bytes):

Tool              | Compressed Size |
------------------|-----------------|
gzip              | 36518 KB        |
gzip -9           | 36445 KB        |
bzip2             | 29009 KB        |
xz                | 26376 KB        |
xz --extreme      | 26366 KB        |
uharc -mx md32768 | 23919 KB        |
zmolly -b99       | 23516 KB        |
