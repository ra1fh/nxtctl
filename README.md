nxtctl
======

### About

nxtctl is a simple libusb based program to control Mindstorms NXT via
USB. The following operations are implemented:

- list files
- upload/download/delete files
- start/stop programs
- get firmware and battery info


### Building

You need libusb-1.0 to build nxtctl. After extracting the tarball,
build nxtctl with:

        $ make
        $ make install

nxtctl should build and work on at least OpenBSD/amd64,
OpenBSD/sparc64, Debian 7.0 (amd64).

### Usage

        nxtctl [-BbdfghilpsSv] [filename/pattern]
         -B             boot (disabled by default)
         -b             print battery level
         -d [filename]  delete file
         -f             print firmware version
         -g [filename]  get file
         -p [filename]  put file
         -i             print device info
         -l [pattern]   list files
         -s [filename]  start program
         -S             stop running program
         -v             verbose debug output
