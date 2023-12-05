# fcal
## Free C++ Audio Library

FCAL (Free C++ Audio Library) is an open-source audio library for C++ applications, currently just supporting Windows, allowing developers to play audio in a C++ environment.

**Current features**:
  - WASAPI integration
  - Audio playback thread.
  - .WAV file streaming.
  - Volume (gain) and balance controls with streams.
  - Automatic channel, sample rate, and bit depth conversion.

**Planned features**:
  - Linux (Ubuntu, at least) support.
  - .OGG file reading.

As of v0.2, the only files necessary for the features of this library are fcal.h and fcal.dll if linking dynamically, or fcal.cpp if linking statically. Examples will be provided
in the "examples" folder, and documentation will be provided in the "docs" folder.

### Compiling

Since fcal uses the C++ standard implementation for threads, fcal needs to be compiled with C++11 or later.

#### Dynamically linking

When linking dynamically, include -lfcal in your list of libraries to compile with. For example:

        g++ -std=c++11 ../src/test.cpp -L./ [...] -lfcal -o test.exe

#### Statically linking

When linking statically, compile fcal.cpp into the build and include -lole32 and -lpthread in your libraries. For example:

        g++ -std=c++11 ../lib/fcal.cpp ../src/test.cpp -L./ [...] -lpthread -lole32 -o test.exe

### License

fcal uses the zlib license. For more information, check LICENSE.md.
