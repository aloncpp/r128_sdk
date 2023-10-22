$ sudo apt install gperf

$ git clone https://bitbucket.org/nuttx/tools.git
$ cd tools/kconfig-frontends
$ # on MacOS do the following:
$ patch < ../kconfig-macos.diff -p 1
$ ./configure --enable-mconf --disable-shared --enable-static --disable-gconf --disable-qconf --disable-nconf
$ # on Linux do the following:
$ ./configure --enable-mconf --disable-nconf --disable-gconf --disable-qconf
$ make
$ make install