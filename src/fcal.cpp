#include "fcal.h"

#include <cmath>
#include <iostream>
#include <thread>

#include "comdef.h"

#include "mmdeviceapi.h"
#include "audioclient.h"

#define TASKTYPE_SINGLE 0
#define TASKTYPE_SOURCE 1

#define VERIFY(hr) if(!check_result(hr)) return hr

static bool print_info = false;

/*
The audio_task structure contains basic information about a clip of audio that needs to be played, including the clip itself, as well as it's
length, source (if it's a stream), and type.
*/

//Linear interpolation - used for sample rate conversions.
float util_lerp(float a, float b, float x)
{
    return (b - a) * x + a;
}

//Converting bytes to floats. To be used for .WAV files, specifically, as this function will convert 8/16/24 bit integers to floats, or simply
//copy 32-bit floats over.
void conv_bytes_to_floats(float* data, unsigned char* bytes, int data_size, int bytes_per_float)
{
    for(int i = 0; i < data_size / bytes_per_float; i++)
    {
        int bi = i * bytes_per_float;
        int se = 0;
        switch(bytes_per_float)
        {
            case 1:
                data[i] = (float) ((signed char) bytes[bi]) / 128 - 1;
                break;
            case 2:
                data[i] = (float) ((signed short) (bytes[bi] + bytes[bi + 1] * 256)) / 32768;
                break;
            case 3:
                se = (bytes[bi] + bytes[bi + 1] * 256 + bytes[bi + 2] * 65536);
                if(se >= 8388608) se -= 16777216;
                data[i] = (float) se / 8388608;
                break;
            case 4:
                memcpy(&data[i], bytes + bi, 4);
                break;
            default:
                std::cerr << "Cannot convert " << bytes_per_float << " to a float." << std::endl;
                break;
        }
    }
}

//Converting floats to bytes. This function will likely only be used for final output. Will convert to 8/16/24 bit integers or 32-bit floats
//depending on the 'bytes_per_float' value.
void conv_floats_to_bytes(unsigned char* data, float* floats, int float_array_size, int bytes_per_float)
{
    for(int i = 0; i < float_array_size; i++)
    {
        int bi = i * bytes_per_float;

        int si;
        unsigned char* p;
        
        switch(bytes_per_float)
        {
            case 1:
                si = floats[i] * 128;
                data[bi] = si;
                break;
            case 2:
                si = floats[i] * 32768;
                data[bi] = si;
                data[bi + 1] = si >> 8;
                break;
            case 3:
                si = floats[i] * 8388608;
                data[bi] = si;
                data[bi + 1] = si >> 8;
                data[bi + 2] = si >> 16;
                break;
            case 4:
                p = reinterpret_cast<unsigned char*>(&floats[i]);
                for(int j = 0; j < 4; j++)
                    data[bi + j] = p[j];
                break;
            default:
                std::cerr << "Unsupported float to byte conversion: " << bytes_per_float << std::endl;
                return;
        }
    }
}

//Converts a stream with current->nChannels to a stream with result->nChannels. This function currently simply copies the first channel of the current
//data stream however many times it needs, or omits other channels in the stream.
void conv_channels(WAVEFORMATEX* current, WAVEFORMATEX* result, float** data, unsigned int* data_size)
{
    int current_channels = current->nChannels;
    int result_channels = result->nChannels;
    
    int frames = *data_size / (current_channels);
    float* new_data = new float[frames * result_channels];

    for(int i = 0; i < frames; i++)
    {
        int current_offset = i * current_channels;
        int result_offset = i * result_channels;

        for(int k = 0; k < result_channels; k++)
        {
            new_data[result_offset + k] = (*data)[current_offset];
        }
    }

    delete[] data[0];
    *data = new_data;
    *data_size = frames * result_channels;
}

//Currently unused. Converts a stream with current->wBitsPerSample to a stream with result->wBitsPerSample. Since the audio pipeline works with 32-bit floats,
//and does one conversion at the end, this function is currently unnessecary.
void conv_bit_depth(WAVEFORMATEX* current, WAVEFORMATEX* result, unsigned char** data, unsigned int* data_size)
{
    int current_bit_depth = current->wBitsPerSample;
    int result_bit_depth = result->wBitsPerSample;

    int channel_frames = *data_size / (current_bit_depth / 8);
    unsigned char* new_data = new unsigned char[channel_frames * (result_bit_depth / 8)];

    for(int i = 0; i < channel_frames; i++)
    {
        float s;
        int se;
        switch(current_bit_depth)
        {
            case 8:
                s = (float) (signed char) (*data[i]) / 128 - 1;
                break;
            case 16:
                s = (float) ((signed short) ((*data)[i * 2] + (*data)[i * 2 + 1] * 256)) / 32768;
                break;
            case 24:
                se = ((*data)[i * 3] + (*data)[i * 3 + 1] * 256 + (*data)[i * 3 + 2] * 65536);
                if(se >= 8388608) se -= 16777216;
                s = (float) se / 8388608;
                break;
            case 32:
                memcpy(&s, *data + i * 4, 4);
                break;
            default:
                std::cerr << "Unsupported audio bit depth: " << current_bit_depth << std::endl;
                return;
        }

        int si;
        unsigned char* p;

        switch(result_bit_depth)
        {
            case 8:
                si = s * 128;
                new_data[i] = si;
                break;
            case 16:
                si = s * 32768;
                new_data[i * 2] = si;
                new_data[i * 2 + 1] = si >> 8;
                break;
            case 24:
                si = s * 8388608;
                new_data[i * 3] = si;
                new_data[i * 3 + 1] = si >> 8;
                new_data[i * 3 + 2] = si >> 16;
                break;
            case 32:
                p = reinterpret_cast<unsigned char*>(&s);
                for(int j = 0; j < 4; j++)
                    new_data[i * 4 + j] = p[j];
                break;
            default:
                std::cerr << "Unsupported audio bit depth: " << result_bit_depth << std::endl;
                return;
        }
    }

    delete[] data[0];
    *data = new_data;
    *data_size = channel_frames * (result_bit_depth / 8);
}

//Converts a stream of current->nSamplesPerSec to a stream of result->nSamplesPerSec. Uses linear interpolation to sample at a lower/higher
//resolution.
void conv_sample_rate(WAVEFORMATEX* current, WAVEFORMATEX* result, float* data, unsigned int data_size)
{
    int current_rate = current->nSamplesPerSec;
    int result_rate = result->nSamplesPerSec;

    float* current_data = new float[data_size];
    for(unsigned int i = 0; i < data_size; i++)
    {
        current_data[i] = data[i];
    }

    double ratio = (double) current_rate / (double) result_rate;
    double point = 0;

    unsigned int channels = result->nChannels;

    int size_of_point = channels;

    for(unsigned int i = 0; i < data_size; i += size_of_point)
    {
        for(unsigned int c = 0; c < channels; c++)
        {
            int si = (int) point * size_of_point + c;
            int ei = (int) point * size_of_point + c + size_of_point;

            float start, end;
            start = current_data[si];
            end = current_data[ei];

            float offset = point - (int) point;

            float p = util_lerp(start, end, offset);
            data[i + c] = p;
        }

        point += ratio;
    }

    delete[] current_data;
}

fcal::audio_stream::audio_stream(std::string filepath) : filepath(filepath)
{
    success_init = false;

    volume = 1;
    balance_left = 1;
    balance_right = 1;

    if(filepath.substr(filepath.length() - 4) == ".wav")
    {
        read_wav_header();
    }
    else
    {
        std::cerr << "File type unsupported: " << filepath << std::endl;
    }
}

fcal::audio_stream::~audio_stream()
{

}

//Applies the volume modifier to the float stream.
void fcal::audio_stream::apply_balance(float* data, unsigned int data_size)
{
    for(unsigned int i = 0; i < data_size; i += 2)
    {
        data[i] *= balance_left;
        data[i + 1] *= balance_right;
    }
}

//Applies the balance modifier to the float stream.
void fcal::audio_stream::apply_volume(float* data, unsigned int data_size)
{
    for(unsigned int i = 0; i < data_size; i++)
    {
        data[i] *= volume;
    }
}

float fcal::audio_stream::get_balance_left()
{
    return balance_left;
}

float fcal::audio_stream::get_balance_right()
{
    return balance_right;
}

float fcal::audio_stream::get_volume()
{
    return volume;
}

bool fcal::audio_stream::is_valid()
{
    return success_init;
}

//Pulls a subset of data out of a .WAV file and converts to a float stream compliant with the native_format format and modifiers such as gain/balance. This function
//is accessed by the audio buffer loop to play an audio stream.
float* fcal::audio_stream::pull(unsigned int& frame_offset, unsigned int frames, WAVEFORMATEX* native_format, bool* end)
{
    FILE* file = fopen(filepath.c_str(), "rb");

    unsigned int frame_size = file_format.nChannels * file_format.wBitsPerSample / 8;
    unsigned int data_offset = file_data_offset + frame_offset * frame_size;
    fseek(file, data_offset, SEEK_CUR); //Go to data begin.

    unsigned int remaining = length - (data_offset - file_data_offset);
    unsigned int data_size = frames * frame_size;
    unsigned int size = 0;
    
    if(remaining < data_size)
    {
        size = remaining;
        *end = true;
    }
    else
    {
        size = data_size;
        *end = false;
    }

    unsigned char* raw_data = new unsigned char[data_size];
    fread(raw_data, 1, size, file);
    fclose(file);

    if(remaining < data_size)
    {
        for(unsigned int i = remaining; i < data_size; i++)
        {
            raw_data[i] = 0;
        }
    }

    int bytes_per_float = file_format.wBitsPerSample / 8;
    float* data = new float[data_size / bytes_per_float];

    conv_bytes_to_floats(data, raw_data, data_size, bytes_per_float);
    size /= bytes_per_float;

    if(file_format.nChannels != native_format->nChannels)
    {
        //Convert to (most likely) stereo channel.
        conv_channels(&file_format, native_format, &data, &size);
    }

    if(file_format.nSamplesPerSec != native_format->nSamplesPerSec)
    {
        //Convert to native sample rate.
        conv_sample_rate(&file_format, native_format, data, size);
        float s = (float) size * ((float) file_format.nSamplesPerSec / (float) native_format->nSamplesPerSec);
        size = (int) s;
    }

    apply_volume(data, data_size / bytes_per_float);
    apply_balance(data, data_size / bytes_per_float);

    delete[] raw_data;

    float sr_ratio = ((float) file_format.nSamplesPerSec / (float) native_format->nSamplesPerSec);

    size = data_size * sr_ratio;
    size -= size % frame_size;

    frame_offset += size / frame_size;

    //If you don't make sure the offset moves by the right interval, the audio will stop working altogether.
    return data;
}

void skip_if_junk(FILE* file, std::vector<unsigned int>& offsets, std::vector<unsigned int>& sizes)
{
    if(offsets.size() != 0 && offsets[0] == (unsigned int) ftell(file))
    {
        fseek(file, sizes[0], SEEK_CUR);

        offsets.erase(offsets.begin());
        sizes.erase(sizes.begin());
    }
}

//Obtains necessary information from the .WAV file header.
void fcal::audio_stream::read_wav_header()
{
    FILE* file = fopen(filepath.c_str(), "rb");
    if(!file)
    {
        std::cerr << "Invalid file filepath! " << filepath << std::endl;
        return;
    }

    //Checking for JUNK chunks in the .WAV header.

    std::vector<unsigned int> ignore_offsets, ignore_sizes;

    char junk_check[256];
    fread(junk_check, sizeof(char), 256, file);

    unsigned int loop_size = 256 - 4;
    for(unsigned int i = 0; i < loop_size; i++)
    {
        char name[5];
        name[4] = '\0';
        for(int j = 0; j < 4; j++)
            name[j] = junk_check[i + j];
        
        if(strcmp(name, "JUNK") == 0)
        {
            unsigned int ig_size;

            ig_size += junk_check[i + 4];
            ig_size += junk_check[i + 5] * 256;
            ig_size += junk_check[i + 6] * 256 * 256;
            ig_size += junk_check[i + 7] * 256 * 256 * 256;

            ignore_offsets.push_back(i);
            ignore_sizes.push_back(ig_size + 8);

            loop_size += ig_size + 8;
            i += ig_size + 7;
            continue;
        }

        if(strcmp(name, "data") == 0)
        {
            break;
        }
    }

    fseek(file, 0, SEEK_SET);

    /*
    WAV files are formatted as follows:
    Bytes 1-4: "RIFF" tag (char[4])
    Bytes 5-8: Size (32-bit integer (DWORD/unsigned int))
    Bytes 9-12: "WAVE" tag (char[4])
    Bytes 13-16: "fmt " tag (char[4])
    Bytes 17-20: Chunk size (32-bit integer)
    Bytes 21-22: Format type (16-bit integer (short))
    Bytes 23-24: # of channels (16-bit integer)
    Bytes 25-28: Sample rate (32-bit integer)
    Bytes 29-32: Average bytes/sec (32-bit integer)
    Bytes 33-34: Bytes per sample (16-bit integer)
    Bytes 35-36: Bits per sample (16-bit integer)
    Bytes 37-40: "data" tag (char[4])
    Bytes 41-44: Data Size (32-bit integer)
    Bytes 45+: Data
    */

    char wav_type[4];

    fread(wav_type, sizeof(char), 4, file);

    if(strcmp(wav_type, "RIFF") < 0)
    {
        std::cerr << "Invalid .WAV type: " << filepath << std::endl;
        std::cerr << "Reads: " << wav_type << std::endl;
        return;
    }

    fseek(file, sizeof(DWORD), SEEK_CUR); //skip the file length.
    fread(wav_type, sizeof(char), 4, file);

    if(strcmp(wav_type, "WAVE") < 0)
    {
        std::cerr << "WAVE tag not present: " << filepath << std::endl;
        return;
    }

    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(wav_type, sizeof(char), 4, file);

    if(strcmp(wav_type, "fmt ") < 0)
    {
        std::cerr << "Invalid .WAV format: " << filepath << std::endl;
        return;
    }

    unsigned int chunk_size;
    unsigned short format_type;

    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&chunk_size, sizeof(DWORD), 1, file);
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&format_type, sizeof(short), 1, file);
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&file_format.nChannels, sizeof(WORD), 1, file);
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&file_format.nSamplesPerSec, sizeof(DWORD), 1, file);
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&file_format.nAvgBytesPerSec, sizeof(DWORD), 1, file);
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fseek(file, 2, SEEK_CUR); //skip the file length.
    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(&file_format.wBitsPerSample, sizeof(WORD), 1, file);

    skip_if_junk(file, ignore_offsets, ignore_sizes);
    fread(wav_type, sizeof(char), 4, file);

    if(print_info)
    {
        std::cout << filepath << " loaded." << std::endl;
        std::cout << "   Sample rate: " << file_format.nSamplesPerSec << std::endl;
        std::cout << "   Bit depth:   " << file_format.wBitsPerSample << std::endl;
        std::cout << "   Channels:    " << file_format.nChannels << std::endl;
    }

    if(strcmp(wav_type, "data") < 0)
    {
        std::cerr << "Missing data: " << filepath << std::endl;
        return;
    }

    fread(&length, sizeof(DWORD), 1, file);
    file_data_offset = ftell(file);

    success_init = true;
    
    fclose(file);
}

//Sets the balance (gain in both the 'left' and 'right' speakers) of the audio stream.
void fcal::audio_stream::set_balance(float left, float right)
{
    balance_left = left;
    balance_right = right;
}

//Sets the volume (gain) of the audio stream.
void fcal::audio_stream::set_volume(float val)
{
    volume = val;
}

fcal::audio_source::audio_source()
{
    task = new audio_task();
    task->data = new float[1];
}

fcal::audio_source::~audio_source()
{
    delete[] task->data;
    delete task;
}

fcal::audio_task* fcal::audio_source::get_task()
{
    return task;
}

bool fcal::audio_source::is_playing()
{
    return streams.size() != 0;
}

void fcal::audio_source::renew_task(unsigned int frame_length, WAVEFORMATEX* format)
{
    if(task->offset < task->length) return;

    //Maybe I should find a better way to do this.
    for(unsigned int i = 0; i < stop_requests.size(); i++)
    {
        for(unsigned int j = 0; j < streams.size(); j++)
        {
            if(streams[j] == stop_requests[i])
            {
                streams.erase(streams.begin() + j);
                stream_offsets.erase(stream_offsets.begin() + j);
                i--;
                break;
            }
        }
    }

    unsigned int size = frame_length * format->nChannels;

    float* sum_data = new float[size];
    for(unsigned int i = 0; i < size; i++)
        sum_data[i] = 0;

    for(unsigned int i = 0; i < streams.size(); i++)
    {
        bool end = false;
        float* data = streams[i]->pull(stream_offsets[i], frame_length, format, &end);

        for(unsigned int j = 0; j < size; j++)
        {
            sum_data[j] += data[j];
        }

        delete[] data;

        if(end)
        {
            streams.erase(streams.begin() + i);
            stream_offsets.erase(stream_offsets.begin() + i);
            i--;
        }
    }

    delete[] task->data;
    task->data = sum_data;
    task->length = size;
    task->offset = 0;
    task->stream_end = false;
    task->type = TASKTYPE_SOURCE;
}

void fcal::audio_source::play(audio_stream* stream)
{
    streams.push_back(stream);
    stream_offsets.push_back(0);
}

void fcal::audio_source::stop(audio_stream* stream)
{
    for(unsigned int i = 0; i < streams.size(); i++)
    {
        if(streams[i] == stream)
        {
            stop_requests.push_back(stream);
            break;
        }
        if(i == streams.size() - 1)
        {
            std::cerr << "Could not locate stream to stop: " << stream << std::endl;
        }
    }
}

static std::thread* audio_thread;

static bool active;

static int req_buffer_ms, buffer_duration_ms, sin_offset;
static double frame_per_msec;
static WAVEFORMATEX* format;

static std::vector<fcal::audio_task> tasks;
static std::vector<fcal::audio_source*> sources;

//Generates a sine wave at 400 hz for buffer_frame_length frames. This is used as a test sound.
float* generate_sin_wave(unsigned int buffer_frame_length)
{
    DWORD sample_rate = format->nSamplesPerSec;
    WORD channels = format->nChannels;

    int floats_per_frame = channels;

    float freq = 400;
    float factor = (float) sample_rate / freq;

    float* data = new float[buffer_frame_length * floats_per_frame];

    for(UINT32 i = 0; i < buffer_frame_length; i++)
    {
        float s = std::sin((float) sin_offset * 3.1415926f * 2 / factor);
        for(int c = 0; c < floats_per_frame; c++)
        {
            int b = i * floats_per_frame + c;
            data[b] = s;
        }
        sin_offset++;
    }

    return data;
}

//Plays a test sound (sine wave at 400 Hz) for 'ms' milliseconds.
void fcal::play_test_sound(unsigned int ms)
{
    audio_task task;
    task.data = generate_sin_wave(frame_per_msec * ms);
    task.length = frame_per_msec * ms * format->nChannels;
    task.offset = 0;
    task.type = TASKTYPE_SINGLE;
    tasks.push_back(task);
}

//Adds an audio_source object to the sources list. This means that the playback thread will be listening for streams playing on this source.
void fcal::register_source(fcal::audio_source* source)
{
    sources.push_back(source);
}

//Removes an audio_source object from the sources list. The audio playback thread will no longer listen to streams on this source.
void fcal::remove_source(fcal::audio_source* source)
{
    for(unsigned int i = 0; i < sources.size(); i++)
    {
        if(sources[i] == source)
        {
            sources.erase(sources.begin() + i);
            break;
        }
        if(i == sources.size() - 1)
        {
            std::cerr << "Couldn't locate source to remove: " << source << std::endl;
        }
    }
}

//Error checker utility function for the WASAPI.
bool check_result(HRESULT result)
{
    if(FAILED(result))
    {
        _com_error err(result);
        std::cerr << err.ErrorMessage() << std::endl;
        return false;
    }

    return true;
}

//Writes the audio output (rendering) buffer.
void write_buffer(unsigned char* data, unsigned int buffer_frame_length)
{
    WORD bit_depth = format->wBitsPerSample;
    WORD channels = format->nChannels;

    int bytes_per_sample = bit_depth / 8;
    unsigned int float_array_length = buffer_frame_length * channels;

    float* f_data = new float[float_array_length];

    for(unsigned int i = 0; i < float_array_length; i++)
    {
        f_data[i] = 0;
    }

    for(unsigned int i = 0; i < sources.size(); i++)
    {
        
    }

    for(unsigned int i = 0; i < float_array_length; i++)
    {
        for(unsigned int t = 0; t < tasks.size(); t++)
        {
            f_data[i] += tasks[t].data[tasks[t].offset];
            tasks[t].offset++;
            if(tasks[t].offset >= tasks[t].length)
            {
                delete[] tasks[t].data;
                tasks.erase(tasks.begin() + t);
            }
        }
        for(unsigned int s = 0; s < sources.size(); s++)
        {
            if(sources[s]->is_playing())
            {
                sources[s]->renew_task(buffer_frame_length, format);
                fcal::audio_task* t = sources[s]->get_task();
                f_data[i] += t->data[t->offset];
                t->offset++;
            }
        }
    }

    conv_floats_to_bytes(data, f_data, float_array_length, bytes_per_sample);
    delete[] f_data;
}

static IMMDeviceEnumerator* device_enumerator;
static IMMDevice* audio_device;
static IAudioClient* audio_client;
static IAudioRenderClient* audio_render_client;

static unsigned int buffer_frame_size;

HRESULT wasapi_init()
{
    //Initialize Windows COM library.
    HRESULT hr = CoInitialize(NULL);
    VERIFY(hr);

    //Create a Windows multimedia device enumerator instance. As of 2023, this is only used for getting audio endpoint devices.
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**) &device_enumerator);
    VERIFY(hr);

    //Get the actual audio device (headphones/speakers) to be used for playback (rendering).
    hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audio_device);
    VERIFY(hr);

    //The AudioClient interface is what we use to create an audio stream between the application and engine layer.
    
    hr = audio_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**) &audio_client);
    VERIFY(hr);

    //Getting the format of the endpoint device (sample rate, bit depth, etc).
    hr = audio_client->GetMixFormat(&format);
    VERIFY(hr);

    //Initializing the audio stream.
    hr = audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, 0, (REFERENCE_TIME) (req_buffer_ms * 10000), 0, format, NULL);
    VERIFY(hr);
    hr = audio_client->GetBufferSize(&buffer_frame_size);
    VERIFY(hr);

    //Getting a render client. This will let us actually write to the rendering buffer, which will then get sent down to the audio engine.
    hr = audio_client->GetService(__uuidof(IAudioRenderClient), (void**) &audio_render_client);
    VERIFY(hr);

    //Should be the duration in ms of the actual buffer - assuming everything goes right.
    buffer_duration_ms = (buffer_frame_size * 1000) / format->nSamplesPerSec;
    frame_per_msec = (double) format->nSamplesPerSec / (1000 * format->nChannels * format->wBitsPerSample / 8);

    if(print_info)
    {
        std::cout << "Audio client initialized." << std::endl;
        std::cout << "   Sample rate: " << format->nSamplesPerSec << std::endl;
        std::cout << "   Bit depth:   " << format->wBitsPerSample << std::endl;
        std::cout << "   Channels:    " << format->nChannels << ((format->nChannels == 1) ? " (mono)" : " (stereo)") << std::endl;
        std::cout << "   Buffer size: " << buffer_frame_size << std::endl;
        std::cout << "     Duration:  " << buffer_duration_ms << "ms" << std::endl;
        std::cout << "     Frames/ms: " << frame_per_msec << std::endl;
    }

    return hr;
}

//Opens and maintains the audio rendering thread, used by WASAPI.
HRESULT thread_open()
{
    //Starting the audio client, this will begin playback.
    HRESULT hr = audio_client->Start();
    VERIFY(hr);

    unsigned char* data;
    unsigned int used_buffer_size;

    while(active)
    {
        //Get size of remaining buffer space.
        hr = audio_client->GetCurrentPadding(&used_buffer_size);
        VERIFY(hr);
        unsigned int remaining_buffer_size = buffer_frame_size - used_buffer_size;

        //Getting a pointer to the data, so that we can write to it.
        hr = audio_render_client->GetBuffer(remaining_buffer_size, &data);
        VERIFY(hr);
        
        //Continue writing to that space.
        write_buffer(data, remaining_buffer_size);

        //Release it to the audio client.
        hr = audio_render_client->ReleaseBuffer(remaining_buffer_size, 0);
        VERIFY(hr);

        Sleep(buffer_duration_ms / 2);
    }

    //Stop the audio stream.
    hr = audio_client->Stop();
    VERIFY(hr);

    //Release all COM resources.
    audio_client->Release();
    audio_render_client->Release();
    audio_device->Release();
    device_enumerator->Release();

    return hr;
}

//Opens the audio rendering (playback) thread.
void fcal::open(unsigned int requested_buffer_time)
{
    sin_offset = 0;
    std::vector<audio_task>().swap(tasks);
    active = true;
    req_buffer_ms = requested_buffer_time;

    HRESULT hr = wasapi_init();
    
    if(check_result(hr)) 
        audio_thread = new std::thread(thread_open);
    else
        std::cerr << "Failed to start audio playback thread." << std::endl;
}

//Closes the audio rendering (playback) thread.
void fcal::close()
{
    active = false;

    audio_thread->join();
    delete audio_thread;
}

//Enable info printing. This will print information to the standard output relating to audio_stream objects and the audio playback thread, such
//as sample rates, bit depths, and channels.
void fcal::disable_info_print()
{
    print_info = false;
}

//Enable info printing. This will print information to the standard output relating to audio_stream objects and the audio playback thread, such
//as sample rates, bit depths, and channels.
void fcal::enable_info_print()
{
    print_info = true;
}