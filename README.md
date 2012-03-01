n64rd
=====

Nintendo 64 Remote Debugger
---------------------------

Building
--------

Requirements:

* [SCons](http://www.scons.org/) - Build system

To build a release version:

    $ scons

To build a debug version:

    $ scons debug=1

To clean:

    $ scons -c

Running
-------

    $ n64rd -h
    n64rd v0.1
    By Parasyte (parasyte@kodewerx.org)
    Website: http://www.kodewerx.org/
    Build date: Feb 27 2012, 01:28:39

    Usage: n64rd [options]
    Options:
      -h            Print usage and quit.
      -p <port>     Specify port number (default 0x378).
                    Linux systems with PPDev can use a path.
                    e.g. "/dev/parport0"
      -d            Detect GS firmware version.
      -a <address>  Specify address (default 0x80000000).
      -l <length>   Specify length (default 0x00400000).
      -r[file]      Read memory;
                    Copy <length> bytes from memory <address> (to [file]).
      -w <file>     Write memory;
                    Copy from <file> to memory <address>.
