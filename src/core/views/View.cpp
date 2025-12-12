#include "View.hpp"
#include "api/Config.hpp"
#include <algorithm>
#include <cmath>

namespace core
{

    namespace view
    {
        View::View()
        {
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
            return false;
        }

        void View::onMouseReleased(const std::string &button, float x, float y)
        {
        }

        bool View::onMouseMoved(float x, float y, float dx, float dy)
        {
            return false;
        }

        void View::onMouseLeft()
        {
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

        // TODO
        void View::clampScrollPosition()
        {
            float vScrollable = getScrollableSize();
            float max = vScrollable - size.y;

            if (max < 0)
                max = 0;

            scroll.to.y = std::max(scroll.to.y, 0.0f);
            if (std::isfinite(max))
            {
                scroll.to.y = std::min(scroll.to.y, max);
            }

            float hScrollable = getHScrollableSize();
            max = hScrollable - size.x;

            if (max < 0)
                max = 0;

            scroll.to.x = std::max(scroll.to.x, 0.0f);
            if (std::isfinite(max))
            {
                scroll.to.x = std::min(scroll.to.x, max);
            }
        }

        // TODO
        void View::moveTowards(float &value, float target, float rate,
                               const std::string &name)
        {
            float diff = std::abs(value - target);

            Config &config = ConfigManager::instance();
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

} // namespace core
