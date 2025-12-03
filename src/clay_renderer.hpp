#pragma once

extern "C" {
#include "clay.h"
}

#include <memory>

struct SDL_Renderer;
struct RenWindow;

namespace clay {

struct ClayRendererImpl;

class ClayRenderer {
public:
    ClayRenderer();
    ~ClayRenderer();
    
    void Initialize(RenWindow* window);
    
    // Render all commands
    void RenderCommands(const Clay_RenderCommandArray& commands);
    
    // Clear the renderer with a color
    void Clear(Clay_Color color);
    
    // Draw the current frame
    void Present();
    
    ClayRenderer(const ClayRenderer&) = delete;
    ClayRenderer& operator=(const ClayRenderer&) = delete;
    ClayRenderer(ClayRenderer&&) = delete;
    ClayRenderer& operator=(ClayRenderer&&) = delete;

private:
    std::unique_ptr<ClayRendererImpl> impl;
};

} // namespace clay
