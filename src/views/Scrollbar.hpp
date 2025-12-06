#ifndef SCROLLBAR_HPP
#define SCROLLBAR_HPP

#include <string>

extern "C"
{
#include "renderer.h"
}

namespace view
{

    enum class ScrollbarDirection
    {
        Horizontal,
        Vertical
    };

    enum class ScrollbarAlignment
    {
        Start,
        End
    };

    struct ScrollbarHovering
    {
        bool thumb = false;
        bool track = false;
    };

    class Scrollbar
    {
    public:
        Scrollbar(ScrollbarDirection dir, ScrollbarAlignment align);

        void setSize(float x, float y, float width, float height, float scrollable);

        // Set the scroll percentage (0.0 to 1.0)
        void setPercent(float percent);

        void update();

        // Check if point overlaps the scrollbar
        bool overlaps(float x, float y) const;

        // Mouse event handlers - return true if handled, or scroll ratio if dragging
        // Returns: true (handled without drag), false (not handled), or a float
        // between 0-1 for drag position
        float onMousePressed(const std::string &button, float x, float y, int clicks);
        void onMouseReleased(const std::string &button, float x, float y);
        float onMouseMoved(float x, float y, float dx, float dy);
        void onMouseLeft();

        void draw(RenSurface *surface);

        bool dragging = false;
        ScrollbarHovering hovering;

    private:
        struct Rect
        {
            float x = 0, y = 0, w = 0, h = 0;
        };

        struct ThumbTrack
        {
            float thumb = 0;
            float track = 0;
        };

        void updateThumbRect();
        bool pointInRect(float px, float py, const Rect &rect) const;

        ScrollbarDirection direction;
        ScrollbarAlignment alignment;

        Rect bounds;
        Rect thumb;
        Rect track;

        float scrollable_size = 0;
        float percent_value = 0;
        float drag_start_offset = 0;

        ThumbTrack size;
        ThumbTrack size_to;
    };

} // namespace view

#endif // SCROLLBAR_HPP
