#include <iostream>
#include "reader.hpp"
#include "report.hpp"

// Exit codes: 0 = report printed, 1 = error, 2 = unsupported file format.
enum ExitCode { kSuccess = 0, kError = 1, kUnsupported = 2 };

int main(int argc, char** argv)
{
    std::string filePath;

    if (argc >= 2)
        filePath = argv[1];
    else
    {
        std::cout << "Enter the path to the ELF file: ";
        if (!(std::cin >> filePath))
        {
            std::cerr << "Error: No file path provided" << std::endl;
            return kError;
        }
    }

    try
    {
        Reader reader(filePath);
        return printReport(reader) ? kSuccess : kUnsupported;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return kError;
    }
}
