README EHWE
===========

ehwe (Embedded HW Emulator) is a soft work-bench for embedded software
development on a normal Linux desktop.


Get documentation below. Build-system is CMake-based. Please read
more about it in the [wiki](wiki/README.md).

Simplified build-instructions are available under [src/README.md](src/README.md).

Cloning with submodules in one go:
----------------------------------

To clone project including it's submodules from any server:

    git clone -b master --single-branch --recursive \
	   proto://url/path/etrace.git

To clone the public Github repository:

    git clone --recursive \
	    https://github.com/mambrus/etrace.git

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

