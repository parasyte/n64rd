n64rd
=====

Nintendo 64 Remote Debugger
---------------------------

Building
--------

### Requirements ###

* [Python](http://www.python.org/) - For SCons
* [SCons](http://www.scons.org/) - Build system

### To build a release version ###

    $ scons

### To build a debug version ###

    $ scons debug=1

### To clean ###

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
      -v            Detect GS firmware version.
      -a <address>  Specify address (default 0x80000000).
      -l <length>   Specify length (default 0x00400000).
      -d[file]      Dump memory 32-bits at a time;
                    Copy <length> bytes from memory <address> (to [file]).
      -r[file]      Read memory;
                    Copy <length> bytes from memory <address> (to [file]).
      -w <file>     Write memory;
                    Copy from <file> to memory <address>.

Points of Interest
------------------

### Differences between `-r` and `-d` ###

The main differences between the `-r` and `-d` options are:

* `-r` reads one byte at a time. `-d` reads one word at a time.
  This makes `-d` suitable for reading from ROM space, while `-r` is not.
* `-r` is restricted to reading valid addresses only. The "invalid" address
  ranges are listed below. `-d` is completely unrestricted; they didn't even
  try!
* `-r` keeps the game paused when it is finished. `-d` unpauses immediately when
  finished.
* `-r` can only be used in-game. `-d` can be used in the menu or in-game.

#### Invalid read ranges ####

      0x80780000 - 0x807FFFFF
      0xBDFFFFFF - 0xFFFFFFFF

Note that the mirrored address range 0xA0780000 - 0xA07FFFFF is VALID, so it can
be read easily. It also does not take into account the length of the data being
read. Therefore it's possible to read these invalid ranges by starting at a
lower address and reading more data. Reading 0x80000000 - 0x807FFFFF is
perfectly acceptable. (Something, something horrible programming.)

If it sounds like the `-d` option is more useful, you're probably right. The
only exception is when you want to read memory but leave the game paused. And
that is easy to patch into the `-d` options, anyway. (Patches are forthcoming!)

### Dumping N64 ROMs ###

Dump the cartridge ROM with:

    $ ./n64rd -dgame.n64 -a 0xB0000000 -l 0x0E000000

You will then have game.n64 file that is 224MB in size. This should be large
enough for ANY N64 ROM. But it will be way too much for the majority of games.
The ROM data will repeat in well-defined intervals. You can adjust `-l` to save
a lot of time, if you know the exact ROM size.

#### Dumping the GS ROM ####

Dump the GS ROM with:

    $ ./n64rd -dgs.n64 -a 0xBEC00000 -l 0x00040000
