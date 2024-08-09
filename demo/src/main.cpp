
#include <chrono>
#include <SDL.h>
#include <SDL_image.h>

#include "pack/pack_loader.hpp"
#include "shl/error.hpp"

#include "gen/testpack.h"

pack_loader loader{};

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

static void _update(float);
static void _render(float);

static bool _setup_texture(error *err)
{
    if (texture != nullptr)
    {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    pack_entry img_entry{};

    if (!pack_loader_load_entry(&loader, testpack_pack__res_image_png, &img_entry, err))
        return false;
    
    SDL_RWops *rw = SDL_RWFromMem((void*)img_entry.data, img_entry.size);
    SDL_Surface* surface = IMG_Load_RW(rw, 0);

    texture = SDL_CreateTextureFromSurface(renderer, surface);

    if (texture == nullptr)
    {
        set_error(err, 1, SDL_GetError());
        return false;
    }

    SDL_FreeSurface(surface);
    SDL_RWclose(rw);

    return true;
}

static bool _setup(error *err)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        set_error(err, 1, SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("pack loading demo",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              window_width, window_height,
                              SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window == nullptr)
    {
        set_error(err, 1, SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window,
                                  -1, // renderer driver, -1 = first one that works
                                  SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr)
    {
        set_error(err, 1, SDL_GetError());
        return false;
    }

    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);

    IMG_Init(IMG_INIT_PNG);

    init(&loader);

#ifndef NDEBUG
    if (!pack_loader_load_package_file(&loader, testpack_pack, err))
        return false;
#else
    pack_loader_load_files(&loader, testpack_pack_files, testpack_pack_file_count);
#endif

    return _setup_texture(err);
}

static void _handle_window_event(SDL_Event *e)
{
    if (e->window.event == SDL_WINDOWEVENT_RESIZED)
    {
        SDL_GetWindowSize(window, &window_width, &window_height);
        // etc
    }
}

static void _handle_current_events(SDL_Event *e, bool *quit)
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
            _handle_window_event(e);
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

static void _event_loop()
{
    bool quit = false;
    SDL_Event e{};

    auto now = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::high_resolution_clock::now();
    float dt = 0;

    while (true)
    {
        const float fpss = 1 / FPS;
        
        now = std::chrono::high_resolution_clock::now();
        dt = std::chrono::duration<float>(now - start).count();

        _handle_current_events(&e, &quit);
        
        if (quit)
            break;

        if (dt >= fpss)
        {
            start = now;
            _update(dt);
            _render(dt);
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

static void _cleanup()
{
    SDL_DestroyRenderer(renderer);
    SDL_Quit();

    free(&loader);
}

static void _update(float dt)
{
    tex_x = (int)(tex_x + tex_vel_x) % window_width;
    tex_y = (int)(tex_y + tex_vel_y) % window_height;
}

static void _render(float dt)
{
    SDL_Rect t1{(int)tex_x               , (int)tex_y                , window_width, window_height};
    SDL_Rect t2{(int)tex_x - window_width, (int)tex_y                , window_width, window_height};
    SDL_Rect t3{(int)tex_x               , (int)tex_y - window_height, window_width, window_height};
    SDL_Rect t4{(int)tex_x - window_width, (int)tex_y - window_height, window_width, window_height};

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &t1);
    SDL_RenderCopy(renderer, texture, NULL, &t2);
    SDL_RenderCopy(renderer, texture, NULL, &t3);
    SDL_RenderCopy(renderer, texture, NULL, &t4);
    SDL_RenderPresent(renderer);
}

int main(int argc, const char *argv[])
{
    error err{};

    if (!_setup(&err))
    {
        printf("error: %s\n", err.what);
        return err.error_code;
    }

    _event_loop();

    _cleanup();

    return 0;
}
