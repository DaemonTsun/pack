
#include <chrono>
#include <SDL.h>

#include "sdlexception.hpp"

#include "pack/package_reader.hpp"
#include "gen/testpack.h"

float FPS = 60;

struct _window_resize
{
    float timeout = 0.3f;
    float t = 0.f;
    bool resizing = false;
    SDL_Event event;
} window_resize;

SDL_Window *window = nullptr;

void update(float);
void render(float);

void setup()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        throw sdlexception();

    window = SDL_CreateWindow("title",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480,
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
        throw sdlexception();
}

void handle_window_event(SDL_Event *e)
{
    if (e->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
    }
}

void handle_current_events(SDL_Event *e, bool *quit)
{
    while (SDL_PollEvent(e) != 0 && !*quit)
    {
        printf("%d\n", e->type);
        switch (e->type)
        {
        case SDL_QUIT:
            *quit = true;
            return;

        case SDL_WINDOWEVENT:
        {
            if (window_resize.timeout > 0
             && e->window.event == SDL_WINDOWEVENT_RESIZED)
            {
                window_resize.resizing = true;
                window_resize.t = window_resize.timeout;
                window_resize.event = *e;
            }
            else
                handle_window_event(e);
            break;
        }

        case SDL_KEYDOWN:
        case SDL_KEYUP:

        case SDL_MOUSEMOTION:
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:

        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            break;
        case SDL_CONTROLLERAXISMOTION:
            break;
        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
            continue;
        default:
            continue;
        }
    }
}

void update_window_resizing_timeout(float dt)
{
    if (!window_resize.resizing)
        return;

    window_resize.t -= dt;
    
    if (window_resize.t < 0)
    {
        window_resize.resizing = false;
        handle_window_event(&window_resize.event);
    }
}

void event_loop()
{
    bool quit = false;
    SDL_Event e;

    auto now = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::high_resolution_clock::now();
    float dt = 0;

    while (true)
    {
        const float fpss = 1 / FPS;
        
        now = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float>(now - start).count();

        handle_current_events(&e, &quit);
        
        if (quit)
            break;

        if (dt >= fpss)
        {
            start = now;
            update_window_resizing_timeout(dt);
            
            if (!window_resize.resizing)
            {
                update(dt);
                render(dt);
            }
        }
        else
        {
            const float time_left_until_frame = fpss - dt;
            
            if (time_left_until_frame >= 0.01)
                SDL_Delay(time_left_until_frame * 1000u);
            else
                SDL_Delay(0);
        }
    }
}

void cleanup()
{
    SDL_Quit();
}

void update(float dt)
{
}

void render(float dt)
{
}

int main(int argc, const char *argv[])
{
    setup();

    event_loop();

    cleanup();

    return 0;
}
