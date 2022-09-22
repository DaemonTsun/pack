
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>

#include "sdlexception.hpp"

#include "pack/package_loader.hpp"
#include "gen/testpack.h"

package_loader loader;

float FPS = 60;

struct _window_resize
{
    float timeout = 0.3f;
    float t = 0.f;
    bool resizing = false;
    SDL_Event event;
} window_resize;

SDL_Window *window = nullptr;
SDL_Texture *texture = nullptr;
SDL_Renderer *renderer = nullptr;
int window_width = 640;
int window_height = 480;
float tex_x = 0;
float tex_y = 0;
float tex_vel_x = 3;
float tex_vel_y = 1;

void update(float);
void render(float);

void setup_texture()
{
    if (texture != nullptr)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    memory_stream imgmem;
    init(&imgmem);

    if (!load_entry(&loader, testpack_pack__res_image_png, &imgmem))
        throw std::runtime_error("could not load texture");
    
    SDL_RWops *rw = SDL_RWFromMem(imgmem.data, imgmem.size);
    SDL_Surface* surface = IMG_Load_RW(rw, 0);

    texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (texture == nullptr)
        throw sdlexception();

    SDL_FreeSurface(surface);
}

void setup()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        throw sdlexception();

    window = SDL_CreateWindow("pack loading demo",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              window_width, window_height,
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
        throw sdlexception();

    renderer = SDL_CreateRenderer(window,
                                  -1, // renderer driver, -1 = first one that works
                                  SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr)
        throw sdlexception();

    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);

    IMG_Init(IMG_INIT_PNG);

    load_package(&loader, testpack_pack);

    setup_texture();
}

void handle_window_event(SDL_Event *e)
{
    if (e->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        SDL_GetWindowSize(window, &window_width, &window_height);
        // etc
    }
}

void handle_current_events(SDL_Event *e, bool *quit)
{
    while (SDL_PollEvent(e) != 0 && !*quit)
    {
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
    tex_x = (int)(tex_x + tex_vel_x) % window_width;
    tex_y = (int)(tex_y + tex_vel_y) % window_height;
}

void render(float dt)
{
    SDL_Rect t1{tex_x               , tex_y                , window_width, window_height};
    SDL_Rect t2{tex_x - window_width, tex_y                , window_width, window_height};
    SDL_Rect t3{tex_x               , tex_y - window_height, window_width, window_height};
    SDL_Rect t4{tex_x - window_width, tex_y - window_height, window_width, window_height};

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &t1);
    SDL_RenderCopy(renderer, texture, NULL, &t2);
    SDL_RenderCopy(renderer, texture, NULL, &t3);
    SDL_RenderCopy(renderer, texture, NULL, &t4);
    SDL_RenderPresent(renderer);
}

int main(int argc, const char *argv[])
{
    setup();

    event_loop();

    cleanup();

    return 0;
}
