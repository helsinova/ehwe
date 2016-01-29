# Build

``etrace`` build-system is based on CMake, with all it's blessings and flaws. 

* For a local build (recommended method):
```bash
cd src
mkdir BUILD
cd BUILD
cmake ../
ccmake .         #Adjust settings. Optional
make -j$(cat /proc/cpuinfo | grep processor | wc -l)
```
*The name of the build-directory is not important, just make sure it
differs from other buld-directories*

More detailed build-instructions in the [wiki](http://localhost:5011). If
you haven't set up your wiki locally, you can read more about how in the [wiki subdirectory](../wiki/README.md)

