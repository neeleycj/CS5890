#include <iostream>
#include "reader.hpp"
#include "report.hpp"

int main(int argc, char** argv)
{
    std::string filePath;

    if (argc >= 2)
        filePath = argv[1];
    else
    {
        std::cout << "Enter the path to the ELF file: ";
        std::cin >> filePath;
    }

    try
    {
        Reader reader(filePath);
        printReport(reader);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
