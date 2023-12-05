#include "../fcal.h"

#include <iostream>
#include <string>

int main()
{
    fcal::open(20);

    std::string input = "";
    while(input != "exit")
    {
        std::cout << "> ";
        std::getline(std::cin, input);

        if(input == "1")
        {
            fcal::play_test_sound(50);
        }
        else if(input == "2")
        {
            fcal::play_test_sound(100);
        }
        else if(input == "3")
        {
            fcal::play_test_sound(500);
        }
    }

    fcal::close();

    return 0;
}