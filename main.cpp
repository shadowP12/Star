#include <iostream>
#include "Renderer.h"
int main() {
    Renderer* renderer = new Renderer(800, 600);
    renderer->initRenderer();
    renderer->run();
    delete renderer;
    return 0;
}