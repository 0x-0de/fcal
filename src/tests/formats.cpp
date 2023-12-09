#include "../fcal.h"

#include <iostream>

int main()
{
    fcal::open(50);

    fcal::audio_stream stereo16("resources/jingle 16bit stereo.wav");
    fcal::audio_stream stereo24("resources/jingle 24bit stereo.wav");
    fcal::audio_stream stereo32("resources/jingle 32bit stereo.wav");

    fcal::audio_stream mono16("resources/jingle 16bit mono.wav");
    fcal::audio_stream mono24("resources/jingle 24bit mono.wav");
    fcal::audio_stream mono32("resources/jingle 32bit mono.wav");

    fcal::audio_source source;

    fcal::register_source(&source);

    std::string input = "";
    while(input != "exit")
    {
        std::cout << "Testing .wav playback with different formats..." << std::endl;

        std::cout << "Playing: 16-bit stereo." << std::endl;
        source.play(&stereo16);
        Sleep(1000);

        std::cout << "Playing: 24-bit stereo." << std::endl;
        source.play(&stereo24);
        Sleep(1000);

        std::cout << "Playing: 32-bit stereo." << std::endl;
        source.play(&stereo32);
        Sleep(1000);

        std::cout << "Playing: 16-bit mono." << std::endl;
        source.play(&mono16);
        Sleep(1000);

        std::cout << "Playing: 24-bit mono." << std::endl;
        source.play(&mono24);
        Sleep(1000);

        std::cout << "Playing: 32-bit mono." << std::endl;
        source.play(&mono32);
        Sleep(1000);
    }

    std::cout << "Test concluded." << std::endl;

    fcal::remove_source(&source);
    fcal::close();

    return 0;
}