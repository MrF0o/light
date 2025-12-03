#include "clay_renderer.hpp"
#include "clay_rencache.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>

extern "C" {
#include "clay.h"
#include "renwindow.h"
}

namespace clay {

static constexpr int NUM_CIRCLE_SEGMENTS = 16;

struct ClayRendererImpl {
    RenWindow* window;
    SDL_Renderer* renderer;
    
    ClayRendererImpl() : window(nullptr), renderer(nullptr) {}
};

ClayRenderer::ClayRenderer() : impl(std::make_unique<ClayRendererImpl>()) {}

ClayRenderer::~ClayRenderer() = default;

void ClayRenderer::Initialize(RenWindow* window) {
    if (impl->window == window) return;
    impl->window = window;
    impl->renderer = renwin_get_renderer(window);
    ClayRencache::Initialize();
}

void ClayRenderer::Clear(Clay_Color color) {
    if (!impl->renderer) return;
    
    SDL_SetRenderDrawColor(impl->renderer, color.r, color.g, color.b, color.a);
    SDL_RenderClear(impl->renderer);
}

void ClayRenderer::Present() {
    if (!impl->renderer) return;
    SDL_RenderPresent(impl->renderer);
}

void ClayRenderer::RenderCommands(const Clay_RenderCommandArray& commands) {
    if (!impl->window) {
        fprintf(stderr, "Error: Window not initialized\n");
        return;
    }
    
    ClayRencache::RenderThroughRencache(impl->window, commands);
}

} // namespace clay

