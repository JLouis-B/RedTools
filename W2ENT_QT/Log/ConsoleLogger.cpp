#include "ConsoleLogger.h"

#include <iostream>

ConsoleLogger::ConsoleLogger()
{

}

void ConsoleLogger::log(irr::core::stringc content)
{
    std::cout << content.c_str();
}

void ConsoleLogger::flush()
{
    std::cout.flush();
}
