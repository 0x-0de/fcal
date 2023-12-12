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

    fcal::audio_stream khz16("resources/jingle 16khz.wav");
    fcal::audio_stream khz96("resources/jingle 96khz.wav");

    fcal::audio_source source;

    fcal::register_source(&source);

    std::cout << "Testing .wav playback with different formats...\n" << std::endl;
    std::cout << "If the audio appears to glitch at the end of playback after 32-bit stereo,\nit's likely a problem with the exported .wav files I'm using.\n" << std::endl;
    std::cout << "All 44.1 khz.\n" << std::endl;

    std::cout << "Playing: 16-bit stereo." << std::endl;
    source.play(&stereo16);
    Sleep(1500);

    std::cout << "Playing: 24-bit stereo." << std::endl;
    source.play(&stereo24);
    Sleep(1500);

    std::cout << "Playing: 32-bit stereo." << std::endl;
    source.play(&stereo32);
    Sleep(1500);

    std::cout << "Playing: 16-bit mono." << std::endl;
    source.play(&mono16);
    Sleep(1500);

    std::cout << "Playing: 24-bit mono." << std::endl;
    source.play(&mono24);
    Sleep(1500);

    std::cout << "Playing: 32-bit mono." << std::endl;
    source.play(&mono32);
    Sleep(1500);

    std::cout << "Playing: 16khz. (32-bit stereo)" << std::endl;
    source.play(&khz16);
    Sleep(1500);

    std::cout << "Playing: 96khz. (32-bit stereo)" << std::endl;
    source.play(&khz96);
    Sleep(1500);

    std::cout << "Test concluded." << std::endl;

    fcal::remove_source(&source);
    fcal::close();

    return 0;
}