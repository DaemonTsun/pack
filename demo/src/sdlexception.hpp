
#pragma once

#include <stdexcept>
#include "SDL_error.h"

class sdlexception : public std::runtime_error
{
public:
    sdlexception()
      : std::runtime_error(SDL_GetError())
    {}

    sdlexception(const char *msg)
      : std::runtime_error(msg)
    {}

    void clear()
    {
        SDL_ClearError();
    }
};

