#include "clay_rencache.hpp"
#include <print>
#include <algorithm>

extern "C" {
#include "rencache.h"
}

namespace clay {

bool ClayRencache::initialized = false;

void ClayRencache::Initialize() {
    if (!initialized) {
        initialized = true;
    }
}

// this function will check if bounding box intersects with clip rectangle
bool ClayRencache::ShouldRender(const Clay_BoundingBox& box, const Clay_BoundingBox& clipRect) {
    float x1 = std::max(box.x, clipRect.x);
    float y1 = std::max(box.y, clipRect.y);
    float x2 = std::min(box.x + box.width, clipRect.x + clipRect.width);
    float y2 = std::min(box.y + box.height, clipRect.y + clipRect.height);
    
    return x2 > x1 && y2 > y1;
}

void ClayRencache::RenderThroughRencache(RenWindow* window, const Clay_RenderCommandArray& commands) {
    if (!window) {
        std::println("Error: No window provided to Clay rencache rendering");
        return;
    }
    
    // Current clip rectangle
    Clay_BoundingBox currentClip = {0, 0, 10000, 10000}; // Will be set properly by clip commands
    for (int i = 0; i < commands.length; i++) {
        const Clay_RenderCommand* cmd = Clay_RenderCommandArray_Get(
            const_cast<Clay_RenderCommandArray*>(&commands), i);
        
        if (!cmd) continue;
        
        // Skip the command if not visible within current clip
        if (cmd->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_START &&
            cmd->commandType != CLAY_RENDER_COMMAND_TYPE_SCISSOR_END &&
            !ShouldRender(cmd->boundingBox, currentClip)) {
            continue;
        }
        
        switch (cmd->commandType) {
            case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
                const auto& data = cmd->renderData.rectangle;
                RenRect rect = {
                    static_cast<int>(cmd->boundingBox.x),
                    static_cast<int>(cmd->boundingBox.y),
                    static_cast<int>(cmd->boundingBox.width),
                    static_cast<int>(cmd->boundingBox.height)
                };
                RenColor color = {
                    static_cast<uint8_t>(data.backgroundColor.b),
                    static_cast<uint8_t>(data.backgroundColor.g),
                    static_cast<uint8_t>(data.backgroundColor.r),
                    static_cast<uint8_t>(data.backgroundColor.a)
                };
                rencache_draw_rect(window, rect, color);
                break;
            }
            
            case CLAY_RENDER_COMMAND_TYPE_BORDER: {
                const auto& data = cmd->renderData.border;
                RenColor color = {
                    static_cast<uint8_t>(data.color.b),
                    static_cast<uint8_t>(data.color.g),
                    static_cast<uint8_t>(data.color.r),
                    static_cast<uint8_t>(data.color.a)
                };
                
                const auto& box = cmd->boundingBox;
                
                if (data.width.top > 0) {
                    RenRect rect = {
                        static_cast<int>(box.x),
                        static_cast<int>(box.y),
                        static_cast<int>(box.width),
                        static_cast<int>(data.width.top)
                    };
                    rencache_draw_rect(window, rect, color);
                }
                
                if (data.width.bottom > 0) {
                    RenRect rect = {
                        static_cast<int>(box.x),
                        static_cast<int>(box.y + box.height - data.width.bottom),
                        static_cast<int>(box.width),
                        static_cast<int>(data.width.bottom)
                    };
                    rencache_draw_rect(window, rect, color);
                }
                
                if (data.width.left > 0) {
                    RenRect rect = {
                        static_cast<int>(box.x),
                        static_cast<int>(box.y),
                        static_cast<int>(data.width.left),
                        static_cast<int>(box.height)
                    };
                    rencache_draw_rect(window, rect, color);
                }
                
                if (data.width.right > 0) {
                    RenRect rect = {
                        static_cast<int>(box.x + box.width - data.width.right),
                        static_cast<int>(box.y),
                        static_cast<int>(data.width.right),
                        static_cast<int>(box.height)
                    };
                    rencache_draw_rect(window, rect, color);
                }
                break;
            }
            
            case CLAY_RENDER_COMMAND_TYPE_TEXT: {
                // TBD: implement text rendering through rencache
                const auto& data = cmd->renderData.text;
                
                RenRect rect = {
                    static_cast<int>(cmd->boundingBox.x),
                    static_cast<int>(cmd->boundingBox.y),
                    static_cast<int>(cmd->boundingBox.width),
                    static_cast<int>(cmd->boundingBox.height)
                };
                RenColor color = {
                    static_cast<uint8_t>(data.textColor.b),
                    static_cast<uint8_t>(data.textColor.g),
                    static_cast<uint8_t>(data.textColor.r),
                    static_cast<uint8_t>(data.textColor.a * 0.2)
                };
                rencache_draw_rect(window, rect, color);
                break;
            }
            
            case CLAY_RENDER_COMMAND_TYPE_IMAGE:
                break;
            
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
                currentClip = cmd->boundingBox;
                RenRect rect = {
                    static_cast<int>(cmd->boundingBox.x),
                    static_cast<int>(cmd->boundingBox.y),
                    static_cast<int>(cmd->boundingBox.width),
                    static_cast<int>(cmd->boundingBox.height)
                };
                rencache_set_clip_rect(window, rect);
                break;
            }
            
            case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
                // Reset to full screen clip
                currentClip = {0, 0, 10000, 10000};
                RenRect rect = {0, 0, 10000, 10000};
                rencache_set_clip_rect(window, rect);
                break;
            }
            
            case CLAY_RENDER_COMMAND_TYPE_CUSTOM:
            case CLAY_RENDER_COMMAND_TYPE_NONE:
            default:
                break;
        }
    }
}

} // namespace clay
