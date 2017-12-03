README EHWE
===========

ehwe (Embedded HW Emulator) is a soft work-bench for embedded software
development on a normal Linux desktop.


Get documentation below. Build-system is CMake-based. Please read
more about it in the [wiki](wiki/README.md).

Simplified build-instructions are available under [src/README.md](src/README.md).

Cloning with submodules in one go:
----------------------------------

## Prerequisite:

To avoid "hard-coding" remote-names of submodules, a *symbolic remote name* is
used instead: ``ssh://siterepo/``

For `Github`: Remap the *symbolic remote name* as follows in `~/.gitconfig`: 

```
[url "https://github.com/helsinova/"]
    insteadOf = ssh://siterepo/

```

*(For more advanced setup, see the wiki: [Good to know: Git remotes](wiki/Good-to-know:-Git-remotes))*

## Cloning

To clone the project including it's submodules:

```bash
git clone -b master --single-branch --recursive \
   ssh://siterepo/ehwe.git
```

Embedded workbench (src/embedded)
---------------------------------
Your embedded code resides in a git submodule under ``src/embedded``.
Path for the workbench is hard-coded and the project attached per default
is a simple test/template which you can use for guidance. To replace with
your own project just follow these steps (please read carefully):

### Replace submodule

```bash
git submodule deinit src/embedded
git submodule add $REMOTE_REPO src/embedded
git submodule init
git submodule update
```

Where ``$REMOTE_REPO`` is your URI (i.e. URL, local path e.t.c.)

### Requirements

#### Code
Your code's ``main``-function has to be renamed to ``embedded_main``. One
elegant way to do this is to put the following ``#define`` in a header-file
of your own choice in your own project:

```C
#define main embedded_main
```

*See also template/example ``src/embedded/spiram.h``*

### Buildsystem
This project assumes you use *CMake* as your build-system. I.e. you need
a file ``embedded/CMakeLists.txt`` defining the sources in a module called
``libembedded``.

*See also template/example ``src/embedded/CMakeLists.txt``*


### Runtime arguments
``EHWE`` has it's own arguments which is documented in man-pages and
else-where. But the workbench can have it's own arguments. As ``EHWE`` is
workbench agnostic it can't know which these are and can't parse them.
However, everything after the special option '-' is sent to the workbench
as is.

Note that not all, probably most, embedded targets will provide anything
meaningful in argc & argv (because it requires an environment, which implies a
process).

Example:

```bash
./ehwe -v debug  \
	-d "spi:1:bp:master:/dev/ttyUSB0"  \
	-d "i2c:1:bp:slave:/dev/ttyUSB8" \
	-d "spi:2:bp:master:/dev/ttyUSB9" \
	-- \
	-x 100 "XoXoXo"
```

The last line contains the ``argv`` handed to workbench's ``main()``, which
for the template project SPI-RAM means: *"Write string \"XoXoXo\" 100 times
in memory, then dump the contents of what was written"*
