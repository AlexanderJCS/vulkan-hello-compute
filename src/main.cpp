#include <iostream>
#include "Raymarcher.h"


int main() {
    try {
        Raymarcher raymarcher{};
        raymarcher.renderLoop();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
