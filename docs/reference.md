# fcal
This reference file is written for every public variable, function, and class in fcal v0.2.

### Public functions

```void fcal::open(unsigned int buffer_ms)``` - Opens the audio playback thread with a buffer resolution in milliseconds buffer_ms.

```void fcal::close()``` - Closes the audio playback thread.

```void fcal::play_test_sound(unsigned int ms)``` - Plays a sine wave at 400 Hz for duration ms (in milliseconds). Can be used to test the responsiveness of audio playback.

```void fcal::register_source(fcal::audio_source* source)``` - Adds an audio_source to the audio playback thread's listening list.

```void fcal::remove_source(fcal::audio_source* source)``` - Removes an audio_source from the audio playback thread's listening list.

```void fcal::enable_info_print()``` - Tells fcal to print extra information relating to audio_stream and audio device formats. Useful for debugging issues related to such.

```void fcal::disable_info_print()``` - Tells fcal not to print extra information relating to audio_stream and audio device formats. By default, this feature is already disabled.

### audio_task

```
struct fcal::audio_task
{
	float* data;
	bool stream_end;
	unsigned int length, offset, type;	
}
```

The ```audio_task``` struct contains the data of and information regarding an audio snippet to be written to the audio playback buffer. Developers should not need to worry about audio_task structs, as managing them is the job of the library itself. Regardless, it may be useful to know what each of these parameters mean.

```float* data``` - The raw audio data to be written to the audio playback buffer.

```bool stream_end``` - Indicates whether this is the final audio snippet in an audio_stream, which tells the playback thread to not request further snippets from said audio_stream.

```unsigned int length``` - length of the float array where the data is located.

```unsigned int offset``` - offset of the data in the audio_stream object.

```unsigned int type``` - the 'type' of the audio_task indicates whether it's source is an audio_stream or if it was written to directly.

### audio_stream

```class fcal::audio_stream```

```audio_stream``` objects are responsible for loading and storing information relating to external sources of audio data, such as .wav files.

```fcal::audio_stream::audio_stream(std::string filepath)``` - Attempts to initialize an audio stream with the data in the file located at filepath. As of v0.2, only .wav files are supported, so if the file does not have the .wav extension present fcal will assume the file is of an unsupported type. From there the header of the file is read and information such as the audio data's bit depth, sample rate, and number of channels are pulled.

```fcal::audio_stream::~audio_stream()``` - Empty destructor.

```float fcal::audio_stream::get_balance_left()``` - Returns the left balance value for the audio_stream. This value is set to 1 upon initialization.

```float fcal::audio_stream::get_balance_right()``` - Returns the right balance value for the audio_stream. This value is set to 1 upon initialization.

```float fcal::audio_stream::get_volume()``` - Returns the volume value for the audio_stream. This value is set to 1 upon initialization.

```bool fcal::audio_stream::is_valid()``` - Returns true if the initialization for this audio_stream object was successful, and false otherwise.

```float* fcal::audio_stream::pull(unsigned int& frame_offset, unsigned int frames, WAVEFORMATEX* native_format, bool* end)``` - Pulls data out of an audio_stream's source file, from ```frame_offset``` to ```frame_offset + frames```, and converts the data into format ```native_format```. If the data includes the end of the stream, the function modifies the value at ```end``` to true. The function also modifies the value at ```frame_offset``` to the new offset determined after sample rate conversion. An audio_source object routinely calls this function when playing an audio_stream object.

```void fcal::audio_stream::set_balance(float left, float right)``` - Sets the balance values for 2-channel stereo playback, where 0 corresponds to a muted sound and 1 corresponds to the sound's original volume.

```void fcal::audio_stream::set_volume(float value)``` - Sets the volume value for playback, where 0 corresponds to a muted sound and 1 corresponds to the sound's original volume.

### audio_source

```class fcal::audio_source```

```audio_source``` objects are utility objects that help manage the playback of audio_streams and performs the creation of audio_tasks.

```fcal::audio_source::audio_source()``` - Initializes an audio_source.

```fcal::audio_source::~audio_source()``` - Deinitializes an audio_source and frees associated memory.

```fcal::audio_task* fcal::audio_source::get_audio_task()``` - Returns the current task compiled by the audio_source. This function is routinely called by the audio playback thread when sources are playing.

```unsigned int fcal::audio_source::get_stream_list_size()``` - Returns the number of streams the audio source is currently playing.

```bool fcal::audio_source::is_playing()``` - Returns true if the number of audio_streams in the source's buffer exceeds 0.

```void fcal::audio_source::renew_task(unsigned int frame_length, WAVEFORMATEX* format)``` - Loads the audio_source's audio_task buffer with the audio_source's current data, pulled from the audio_stream objects in it's audio_stream list. This function is routinely called by the audio playback thread when sources are playing.

```void fcal::audio_source::play(fcal::audio_stream* stream)``` - Adds an audio_stream object to an audio_source's playback list.

```void fcal::audio_source_stop(fcal::audio_stream* stream)``` - Removes an audio_stream object from an audio_source's playback list, if it's present. Otherwise, an error message is printed.