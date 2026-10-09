#include <iostream>
#include "reader.hpp"

int main() {
	std::string filePath;
	std::cout << "Enter the path to the ELF file: ";
	std::cin >> filePath;

	try {
		Reader reader(filePath);
		if (reader.isValidELFFile()) {
			std::cout << "The file is a valid 64-bit ELF file." << std::endl;
		} else {
			std::cout << "The file is not a valid 64-bit ELF file." << std::endl;
		}
		reader.getElf64_Ehdr();
		reader.getProgramHeader();
		reader.getSectionHeader();
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
	


	return 0;
}
