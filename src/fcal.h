#ifndef _FCAL_H_
#define _FCAL_H_

#include "windows.h"

#include <string>

#define DLL_FEATURE __declspec(dllexport)

namespace fcal
{
    class DLL_FEATURE audio_stream
    {
        public:
            audio_stream(std::string filepath);
            ~audio_stream();

            float get_balance_left();
            float get_balance_right();
            float get_volume();

            float* pull(unsigned int frames, WAVEFORMATEX* native_format, bool* end);

            void reset();

            void set_balance(float left, float right);
            void set_volume(float val);
        private:
            std::string filepath;
            bool success_init;

            void apply_balance(float* data, unsigned int data_size);
            void apply_volume(float* data, unsigned int data_size);

            void read_wav_header();

            WAVEFORMATEX file_format;
            unsigned int length, offset, file_data_offset;

            float volume, balance_left, balance_right;
    };

    DLL_FEATURE void open(unsigned int requested_buffer_time);
    DLL_FEATURE void close();

    DLL_FEATURE void play_test_sound(unsigned int ms);
    DLL_FEATURE void play_stream(audio_stream* stream);

    DLL_FEATURE void disable_info_print();
    DLL_FEATURE void enable_info_print();
}

#endif