#include "Struct.h"
#include <SDL3/SDL.h>

int Renderer::Render(const SDL_Texture* texture, const SDL_FRect* srect, const SDL_FRect* dsrect) const
{
    return(SDL_RenderTexture(SDLrenderer, std::_Const_cast(texture), srect, dsrect));
}