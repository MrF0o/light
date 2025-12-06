#include "View.hpp"
#include "api/Config.hpp"
#include <algorithm>
#include <cmath>

namespace view
{

    View::View()
    {
        vScrollbar =
            std::make_unique<Scrollbar>(ScrollbarDirection::Vertical,
                                        ScrollbarAlignment::End);
        hScrollbar = std::make_unique<Scrollbar>(ScrollbarDirection::Horizontal,
                                                 ScrollbarAlignment::End);
    }

    View::~View()
    {
        if (L && luaRef != LUA_NOREF)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, luaRef);
        }
    }

    float View::getScrollableSize() const
    {
        return std::numeric_limits<float>::infinity();
    }

    void View::tryClose(std::function<void()> doClose)
    {
        if (doClose)
        {
            doClose();
        }
    }

    bool View::onMousePressed(const std::string &button, float x, float y,
                              int clicks)
    {
        if (!scrollable)
            return false;

        float result = vScrollbar->onMousePressed(button, x, y, clicks);
        if (result != 0.0f)
        {
            if (result != 1.0f)
            {
                scroll.to.y = result * (getScrollableSize() - size.y);
            }
            return true;
        }

        result = hScrollbar->onMousePressed(button, x, y, clicks);
        if (result != 0.0f)
        {
            if (result != 1.0f)
            {
                scroll.to.x = result * (getHScrollableSize() - size.x);
            }
            return true;
        }

        return false;
    }

    void View::onMouseReleased(const std::string &button, float x, float y)
    {
        if (!scrollable)
            return;
        vScrollbar->onMouseReleased(button, x, y);
        hScrollbar->onMouseReleased(button, x, y);
    }

    bool View::onMouseMoved(float x, float y, float dx, float dy)
    {
        if (!scrollable)
            return false;

        float result = 0.0f;

        // Skip vertical scrollbar if horizontal is dragging
        if (!hScrollbar->dragging)
        {
            result = vScrollbar->onMouseMoved(x, y, dx, dy);
            if (result != 0.0f)
            {
                if (result != 1.0f)
                {
                    scroll.to.y = result * (getScrollableSize() - size.y);
                    if (!ConfigManager::instance().animate_drag_scroll) {
                      clampScrollPosition();
                      scroll.y = scroll.to.y;
                    }
                }
                hScrollbar->onMouseLeft();
                return true;
            }
        }

        result = hScrollbar->onMouseMoved(x, y, dx, dy);
        if (result != 0.0f)
        {
            if (result != 1.0f)
            {
                scroll.to.x = result * (getHScrollableSize() - size.x);
                if (!ConfigManager::instance().animate_drag_scroll) {
                    clampScrollPosition();
                    scroll.x = scroll.to.x;
                }
            }
            return true;
        }

        return false;
    }

    void View::onMouseLeft()
    {
        if (!scrollable)
            return;
        vScrollbar->onMouseLeft();
        hScrollbar->onMouseLeft();
    }

    bool View::onFileDropped(const std::string &filename, float x, float y)
    {
        return false;
    }

    void View::onTextInput(const std::string &text)
    {
        // No-op
    }

    void View::onImeTextEditing(const std::string &text, int start, int length)
    {
        // No-op
    }

    bool View::onMouseWheel(float y, float x)
    {
        // No-op
        return false;
    }

    void View::onTouchMoved(float x, float y, float dx, float dy, int i)
    {
        if (!scrollable)
            return;

        // TODO: Implement touch scrollbar dragging
        // if (dragging_scrollbar) {
        //   float delta = getScrollableSize() / size.y * dy;
        //   scroll.to.y += delta;
        // }

        scroll.to.y += -dy;
        scroll.to.x += -dx;
    }

    void View::onScaleChange(float newScale, float prevScale)
    {
        // No-op
    }

    void View::getContentBounds(float &x1, float &y1, float &x2, float &y2) const
    {
        x1 = scroll.x;
        y1 = scroll.y;
        x2 = scroll.x + size.x;
        y2 = scroll.y + size.y;
    }

    void View::getContentOffset(float &offsetX, float &offsetY) const
    {
        offsetX = std::round(position.x - scroll.x);
        offsetY = std::round(position.y - scroll.y);
    }

    void View::clampScrollPosition()
    {
        float vScrollable = getScrollableSize();
        if (std::isfinite(vScrollable))
        {
            float max = vScrollable - size.y;
            if (max > 0)
            {
                scroll.to.y = std::clamp(scroll.to.y, 0.0f, max);
            }
            else
            {
                scroll.to.y = 0.0f;
            }
        }
        else
        {
            scroll.to.y = 0.0f;
        }

        float hScrollable = getHScrollableSize();
        if (std::isfinite(hScrollable))
        {
            float max = hScrollable - size.x;
            if (max > 0)
            {
                scroll.to.x = std::clamp(scroll.to.x, 0.0f, max);
            }
            else
            {
                scroll.to.x = 0.0f;
            }
        }
        else
        {
            scroll.to.x = 0.0f;
        }
    }

    void View::updateScrollbar()
    {
        float vScrollable = getScrollableSize();
        vScrollbar->setSize(position.x, position.y, size.x, size.y, vScrollable);
        float vPercent = scroll.y / (vScrollable - size.y);
        // Avoid setting NaN percent
        vScrollbar->setPercent((vPercent == vPercent) ? vPercent : 0.0f);
        vScrollbar->update();

        float hScrollable = getHScrollableSize();
        hScrollbar->setSize(position.x, position.y, size.x, size.y, hScrollable);
        float hPercent = scroll.x / (hScrollable - size.x);
        // Avoid setting NaN percent
        hScrollbar->setPercent((hPercent == hPercent) ? hPercent : 0.0f);
        hScrollbar->update();
    }

    void View::moveTowards(float &value, float target, float rate,
                           const std::string &name)
    {
        float diff = std::abs(value - target);

        NativeConfig& config = ConfigManager::instance();
        bool transitions = config.transitions;
        bool disabled = ConfigManager::isTransitionDisabled(name);
        float fps = config.fps;
        float animation_rate = config.animation_rate;

        if (!transitions || diff < 0.5f || disabled)
        {
            value = target;
        }
        else
        {
            // Apply frame rate and animation rate adjustments
            if (fps != 60.0f || animation_rate != 1.0f)
            {
                float dt = 60.0f / fps;
                rate = 1.0f - std::pow(std::clamp(1.0f - rate, 1e-8f, 1.0f - 1e-8f),
                                       animation_rate * dt);
            }
            // Lerp
            value = value + (target - value) * rate;
        }

        if (diff > 1e-8f)
        {
            needsRedraw = true;
        }
    }

    bool View::scrollbarOverlapsPoint(float x, float y) const
    {
        return vScrollbar->overlaps(x, y) || hScrollbar->overlaps(x, y);
    }

    bool View::scrollbarDragging() const
    {
        return vScrollbar->dragging || hScrollbar->dragging;
    }

    bool View::scrollbarHovering() const
    {
        return vScrollbar->hovering.track || hScrollbar->hovering.track;
    }

    void View::setScrollable(bool value) { scrollable = value; }

    void View::update()
    {
        float newScale = ConfigManager::instance().scale;
        if (currentScale != newScale)
        {
            onScaleChange(newScale, currentScale);
            currentScale = newScale;
        }

        clampScrollPosition();
        moveTowards(scroll.x, scroll.to.x, 0.3f, "scroll");
        moveTowards(scroll.y, scroll.to.y, 0.3f, "scroll");

        if (!scrollable)
            return;
        updateScrollbar();
    }

    void View::drawBackground(RenSurface *surface, const RenColor &color)
    {
        if (surface)
        {
            RenRect rect = {static_cast<int>(position.x), static_cast<int>(position.y),
                            static_cast<int>(size.x), static_cast<int>(size.y)};
            ren_draw_rect(surface, rect, color);
        }
    }

    void View::drawScrollbar(RenSurface *surface)
    {
        // No-op
    }

    void View::draw(RenSurface *surface)
    {
        // No-op
    }

#ifdef LITE_USE_SDL_RENDERER
    void View::draw(SDL_Renderer *renderer)
    {
        // No-op
    }
#endif

} // namespace view
