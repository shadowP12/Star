#ifndef STAR_RENDERER_H
#define STAR_RENDERER_H
#include <Application/Application.h>

class Renderer : public Application
{
public:
    Renderer(uint32_t width, uint32_t height);
    ~Renderer();
    virtual void prepare();
    virtual void run();
    virtual void finish();
protected:

};


#endif
