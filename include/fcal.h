#ifndef _FCAL_H_
#define _FCAL_H_

#include "windows.h"

#include <string>

namespace fcal
{
    class audio_stream
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

    void open(unsigned int requested_buffer_time);
    void close();

    void play_test_sound(unsigned int ms);
    void play_stream(audio_stream* stream);

    void disable_info_print();
    void enable_info_print();
}

#endif