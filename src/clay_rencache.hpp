#pragma once

extern "C" {
#include "clay.h"
#include "renderer.h"
}

namespace clay {

// birdge layer between Clay and rencache.h
class ClayRencache {
public:
    static void Initialize();
    
    // Render Clay commands through rencache
    static void RenderThroughRencache(RenWindow* window, const Clay_RenderCommandArray& commands);
    
    // Check if a Clay element should be rendered
    static bool ShouldRender(const Clay_BoundingBox& box, const Clay_BoundingBox& clipRect);
    
private:
    static bool initialized;
};

} // namespace clay
