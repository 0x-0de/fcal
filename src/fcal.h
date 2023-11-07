#ifndef _FCAL_H_
#define _FCAL_H_

#include "windows.h"

#include <string>
#include <vector>

#define DLL_FEATURE __declspec(dllexport)

namespace fcal
{
    struct audio_task
    {
        float* data;
        bool stream_end;
        unsigned int length, offset, type;
    };

    class DLL_FEATURE audio_stream
    {
        public:
            audio_stream(std::string filepath);
            ~audio_stream();

            float get_balance_left();
            float get_balance_right();
            float get_volume();

            bool is_valid();

            float* pull(unsigned int& frame_offset, unsigned int frames, WAVEFORMATEX* native_format, bool* end);

            void set_balance(float left, float right);
            void set_volume(float val);
        private:
            std::string filepath;
            bool success_init;

            void apply_balance(float* data, unsigned int data_size);
            void apply_volume(float* data, unsigned int data_size);

            void read_wav_header();

            WAVEFORMATEX file_format;
            unsigned int length, file_data_offset;

            float volume, balance_left, balance_right;
    };

    class DLL_FEATURE audio_source
    {
        public:
            audio_source();
            ~audio_source();

            unsigned int get_stream_list_size();
            audio_task* get_task();

            bool is_playing();

            void renew_task(unsigned int frame_length, WAVEFORMATEX* format);

            void play(audio_stream* stream);
            void stop(audio_stream* stream);
        private:
            std::vector<audio_stream*> streams;
            std::vector<unsigned int> stream_offsets;

            std::vector<audio_stream*> stop_requests;

            audio_task* task;
    };

    DLL_FEATURE void open(unsigned int requested_buffer_time);
    DLL_FEATURE void close();

    DLL_FEATURE void play_test_sound(unsigned int ms);

    DLL_FEATURE void register_source(audio_source* source);
    DLL_FEATURE void remove_source(audio_source* source);

    DLL_FEATURE void disable_info_print();
    DLL_FEATURE void enable_info_print();
}

#endif