#include <iostream>
#include "reader.hpp"
#include "report.hpp"

int main()
{
    std::string filePath;
    std::cout << "Enter the path to the ELF file: ";
    std::cin >> filePath;

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
