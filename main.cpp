#include <iostream>
#include "Renderer.h"
int main() {
    Renderer* renderer = new Renderer(512, 512);
    renderer->initRenderer();
    renderer->run();
    delete renderer;
    return 0;
}