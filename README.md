n64rd
=====

Nintendo 64 Remote Debugger
---------------------------

Building
--------

To build a release version:

    $ gcc -Wall -o n64rd gspro.c n64rd.c

To build a debug version:

    $ gcc -DDEBUG -Wall -o n64rd gspro.c n64rd.c

Running
-------

    $ sudo n64rd -h
    n64rd v0.1
    By Parasyte (parasyte@kodewerx.org)
    Website: http://www.kodewerx.org/
    Build date: Feb 27 2012, 01:28:39

    Usage: n64rd [options]
    Options:
      -h            Print usage and quit.
      -p <port>     Specify port number (default 0x378).
      -d            Detect GS firmware version.
      -a <address>  Specify address (default 0x80000000).
      -l <length>   Specify length (default 0x00400000).
      -r[file]      Read memory;
                    Copy <length> bytes from memory <address> (to [file]).
      -w <file>     Write memory;
                    Copy from <file> to memory <address>.

Why Sudo?
---------

Because I'm lazy. ;) There's a better way to access the parallel port under
Linux, using the special /dev/parport0 character device and ioctl().

See: [User-level device drivers](http://people.redhat.com/twaugh/parport/html/ppdev.html).

