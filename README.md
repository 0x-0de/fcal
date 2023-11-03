# fcal
Free C++ Audio Library

FCAL (Free C++ Audio Library) is an open-source audio library for C++ applications, currently just supporting Windows.

Current features:
  - WASAPI integration
  - Audio playback thread.
  - .WAV file streaming.
  - Volume (gain) and balance controls with streams.
  - Automatic channel, sample rate, and bit depth conversion.

Planned features:
  - Audio sources (to act as middlemen, such that things like volume controls and playback aren't inherently tied to audio_stream objects).
  - Linux (Ubuntu, at least) support.
  - Stream/source halting.
  - .OGG file reading.
