#pragma once

#include <array>
#include <vector>

extern "C" {
#include "clay.h"
#include "renderer.h"
}

namespace clay {

struct FontGroup {
    std::array<RenFont*, FONT_FALLBACK_MAX> fonts;
};

// birdge layer between Clay and rencache.h
class ClayRencache {
public:
    static void Initialize();
    
    // Render Clay commands through rencache
    static void RenderThroughRencache(RenWindow* window, const Clay_RenderCommandArray& commands);
    
    // Check if a Clay element should be rendered
    static bool ShouldRender(const Clay_BoundingBox& box, const Clay_BoundingBox& clipRect);

    static uint16_t AddFont(const FontGroup& fontGroup);
    static FontGroup* GetFont(uint16_t id);
    static void ResetFonts();
    
private:
    static bool initialized;
};

} // namespace clay
