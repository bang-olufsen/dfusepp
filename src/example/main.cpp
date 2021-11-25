// Copyright 2021 - Bang & Olufsen a/s
#include <Dfusepp.h>

#include <vector>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
    if (argc == 2) {
        std::ifstream file(argv[1], std::ios::in | std::ios::binary);
        if (file.is_open()) {
            file.seekg(0, std::ios_base::end);
            size_t fileSize = file.tellg();
            file.seekg(0, std::ios_base::beg);

            std::vector<char> buffer(fileSize);
            file.read(buffer.data(), fileSize);
            file.close();

            Dfusepp::Dfusepp dfusepp;
            dfusepp.addData(reinterpret_cast<const uint8_t*>(buffer.data()), 0, buffer.size());
            std::cout << "File is " << (dfusepp.valid() ? "valid" : "not valid") << std::endl;
        }
    }

    return 0;
}
