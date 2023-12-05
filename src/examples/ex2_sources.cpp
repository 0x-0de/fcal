#include "../fcal.h"

#include <iostream>

int main()
{
    fcal::open(50);

    fcal::audio_stream piano("resources/piano.wav");
    fcal::audio_stream ambient("resources/ambient.wav");
    fcal::audio_stream dance("resources/dance.wav");

    fcal::audio_source source;
    fcal::register_source(&source);

    std::string input = "";
    while(input != "exit")
    {
        std::cout << "> ";
        std::getline(std::cin, input);

        if(input == "1")
        {
            source.play(&piano);
        }
        else if(input == "2")
        {
            source.play(&ambient);
        }
        else if(input == "3")
        {
            source.play(&dance);
        }
    }

    fcal::remove_source(&source);
    fcal::close();

    return 0;
}