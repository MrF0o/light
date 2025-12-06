#include "Scrollbar.hpp"
#include <algorithm>
#include <cmath>

extern "C"
{
#include "lua_compat.h"
}

namespace view
{

    static constexpr float SCROLLBAR_WIDTH = 4.0f;
    static constexpr float SCROLLBAR_MARGIN = 2.0f;
    static constexpr float SCROLLBAR_HOVER_WIDTH = 12.0f;
    static constexpr float MIN_THUMB_SIZE = 20.0f;

    Scrollbar::Scrollbar(ScrollbarDirection dir, ScrollbarAlignment align)
        : direction(dir), alignment(align)
    {
        size.thumb = SCROLLBAR_WIDTH;
        size.track = SCROLLBAR_WIDTH;
        size_to.thumb = SCROLLBAR_WIDTH;
        size_to.track = SCROLLBAR_WIDTH;
    }

    void Scrollbar::setSize(float x, float y, float width, float height,
                            float scrollable)
    {
        bounds.x = x;
        bounds.y = y;
        bounds.w = width;
        bounds.h = height;
        scrollable_size = scrollable;
        updateThumbRect();
    }

    void Scrollbar::setPercent(float percent)
    {
        percent_value = std::clamp(percent, 0.0f, 1.0f);
        updateThumbRect();
    }

    void Scrollbar::updateThumbRect()
    {
        if (direction == ScrollbarDirection::Vertical)
        {
            float x_pos = bounds.x + bounds.w - SCROLLBAR_MARGIN - size.track;
            if (alignment == ScrollbarAlignment::Start)
            {
                x_pos = bounds.x + SCROLLBAR_MARGIN;
            }

            track.x = x_pos;
            track.y = bounds.y;
            track.w = size.track;
            track.h = bounds.h;

            // thumb size and position
            float visible_ratio = bounds.h / scrollable_size;
            float thumb_height = std::max(MIN_THUMB_SIZE, bounds.h * visible_ratio);
            float scroll_track_height = bounds.h - thumb_height;

            thumb.x = x_pos + (size.track - size.thumb) / 2.0f;
            thumb.y = bounds.y + scroll_track_height * percent_value;
            thumb.w = size.thumb;
            thumb.h = thumb_height;
        }
        else
        {
            float y_pos = bounds.y + bounds.h - SCROLLBAR_MARGIN - size.track;
            if (alignment == ScrollbarAlignment::Start)
            {
                y_pos = bounds.y + SCROLLBAR_MARGIN;
            }

            track.x = bounds.x;
            track.y = y_pos;
            track.w = bounds.w;
            track.h = size.track;

            // thumb size and position
            float visible_ratio = bounds.w / scrollable_size;
            float thumb_width = std::max(MIN_THUMB_SIZE, bounds.w * visible_ratio);
            float scroll_track_width = bounds.w - thumb_width;

            thumb.x = bounds.x + scroll_track_width * percent_value;
            thumb.y = y_pos + (size.track - size.thumb) / 2.0f;
            thumb.w = thumb_width;
            thumb.h = size.thumb;
        }
    }

    bool Scrollbar::pointInRect(float px, float py, const Rect &rect) const
    {
        return px >= rect.x && px < rect.x + rect.w && py >= rect.y &&
               py < rect.y + rect.h;
    }

    bool Scrollbar::overlaps(float x, float y) const
    {
        return pointInRect(x, y, track);
    }

    float Scrollbar::onMousePressed(const std::string &button, float x, float y,
                                    int clicks)
    {
        if (button != "left")
            return 0.0f;

        if (pointInRect(x, y, thumb))
        {
            dragging = true;
            if (direction == ScrollbarDirection::Vertical)
            {
                drag_start_offset = y - thumb.y;
            }
            else
            {
                drag_start_offset = x - thumb.x;
            }
            return 1.0f; // handled
        }

        if (pointInRect(x, y, track))
        {
            // Click on track - calculate new position
            if (direction == ScrollbarDirection::Vertical)
            {
                float scroll_track_height = bounds.h - thumb.h;
                if (scroll_track_height > 0)
                {
                    float click_pos = y - bounds.y - thumb.h / 2.0f;
                    return std::clamp(click_pos / scroll_track_height, 0.0f, 1.0f);
                }
            }
            else
            {
                float scroll_track_width = bounds.w - thumb.w;
                if (scroll_track_width > 0)
                {
                    float click_pos = x - bounds.x - thumb.w / 2.0f;
                    return std::clamp(click_pos / scroll_track_width, 0.0f, 1.0f);
                }
            }
        }

        return 0.0f;
    }

    void Scrollbar::onMouseReleased(const std::string &button, float x, float y)
    {
        if (button == "left")
        {
            dragging = false;
        }
    }

    float Scrollbar::onMouseMoved(float x, float y, float dx, float dy)
    {
        hovering.thumb = pointInRect(x, y, thumb);
        hovering.track = pointInRect(x, y, track);

        if (dragging)
        {
            if (direction == ScrollbarDirection::Vertical)
            {
                float scroll_track_height = bounds.h - thumb.h;
                if (scroll_track_height > 0)
                {
                    float new_pos = y - bounds.y - drag_start_offset;
                    return std::clamp(new_pos / scroll_track_height, 0.0f, 1.0f);
                }
            }
            else
            {
                float scroll_track_width = bounds.w - thumb.w;
                if (scroll_track_width > 0)
                {
                    float new_pos = x - bounds.x - drag_start_offset;
                    return std::clamp(new_pos / scroll_track_width, 0.0f, 1.0f);
                }
            }
        }

        return 0.0f;
    }

    void Scrollbar::onMouseLeft()
    {
        hovering.thumb = false;
        hovering.track = false;
    }

    void Scrollbar::update()
    {
        // Animate scrollbar width based on hover state
        float target_width =
            (hovering.track || dragging) ? SCROLLBAR_HOVER_WIDTH : SCROLLBAR_WIDTH;

        // TODO: integrate with proper animation system
        float rate = 0.3f;
        size_to.thumb = target_width;
        size_to.track = target_width;

        size.thumb += (size_to.thumb - size.thumb) * rate;
        size.track += (size_to.track - size.track) * rate;

        updateThumbRect();
    }

    void Scrollbar::draw(RenSurface *surface)
    {
        if (!surface)
        {
            return;
        }

        if (scrollable_size <= (direction == ScrollbarDirection::Vertical ? bounds.h
                                                                          : bounds.w))
        {
            return; // Nothing to scroll, don't draw
        }

        // track
        RenRect track_rect = {static_cast<int>(track.x), static_cast<int>(track.y),
                              static_cast<int>(track.w), static_cast<int>(track.h)};
        RenColor track_color = {40, 40, 40, 128};
        ren_draw_rect(surface, track_rect, track_color);

        // thumb
        RenRect thumb_rect = {static_cast<int>(thumb.x), static_cast<int>(thumb.y),
                              static_cast<int>(thumb.w), static_cast<int>(thumb.h)};
        RenColor thumb_color = hovering.thumb ? RenColor{180, 180, 180, 255}
                                              : RenColor{120, 120, 120, 200};
        ren_draw_rect(surface, thumb_rect, thumb_color);
    }

} // namespace view
