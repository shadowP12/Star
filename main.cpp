#include "Renderer.h"

int main() {
    Renderer renderer(800, 600);
    renderer.prepare();
    renderer.runMainLoop();
    renderer.finish();
    return 0;
}