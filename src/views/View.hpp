#ifndef VIEW_HPP
#define VIEW_HPP

#include "Position.hpp"
#include "Scrollbar.hpp"
#include <memory>
#include <string>
#include <typeinfo>
#include <functional>
#include <limits>

extern "C"
{
#include "lua_compat.h"
#include "renderer.h"
}

namespace view
{

  class ViewRenderer;

  enum class ViewContext
  {
    Application,
    Session
  };

  class View
  {
  public:
    View();
    virtual ~View();

    virtual std::string toString() const { return "View"; }
    virtual std::string getName() const { return "---"; }
    virtual float getScrollableSize() const;
    virtual float getHScrollableSize() const { return 0.0f; }
    virtual bool supportsTextInput() const { return false; }

    virtual void tryClose(std::function<void()> doClose);

    virtual bool onMousePressed(const std::string &button, float x, float y,
                                int clicks);
    virtual void onMouseReleased(const std::string &button, float x, float y);
    virtual bool onMouseMoved(float x, float y, float dx, float dy);
    virtual void onMouseLeft();
    virtual bool onFileDropped(const std::string &filename, float x, float y);
    virtual void onTextInput(const std::string &text);
    virtual void onImeTextEditing(const std::string &text, int start, int length);
    virtual bool onMouseWheel(float y, float x);
    virtual void onTouchMoved(float x, float y, float dx, float dy, int i);
    virtual void onScaleChange(float newScale, float prevScale);

    virtual void *onContextMenu(float x, float y) { return nullptr; }

    virtual void update();
    virtual void draw(RenSurface *surface);
#ifdef LITE_USE_SDL_RENDERER
    virtual void draw(SDL_Renderer *renderer);
#endif
    virtual void drawBackground(RenSurface *surface, const RenColor &color);
    virtual void drawScrollbar(RenSurface *surface);

    void getContentBounds(float &x1, float &y1, float &x2, float &y2) const;
    void getContentOffset(float &offsetX, float &offsetY) const;
    void clampScrollPosition();
    void updateScrollbar();

    void moveTowards(float &value, float target, float rate,
                     const std::string &name);
    template <typename T>
    void moveTowardsField(T &obj, float T::*field, float target, float rate,
                          const std::string &name)
    {
      float &value = obj.*field;
      moveTowards(value, target, rate, name);
    }

    bool scrollbarOverlapsPoint(float x, float y) const;
    bool scrollbarDragging() const;
    bool scrollbarHovering() const;

    const std::type_info &getTypeInfo() const { return typeid(*this); }
    std::string getTypeName() const { return typeid(*this).name(); }

    ViewContext context = ViewContext::Application;
    Position position;
    Size size;
    Scroll scroll;
    std::string cursor = "arrow";
    bool scrollable = false;
    float currentScale = 1.0f;

    std::unique_ptr<Scrollbar> vScrollbar;
    std::unique_ptr<Scrollbar> hScrollbar;

    lua_State *L = nullptr;
    int luaRef = LUA_NOREF;

  protected:
    void setScrollable(bool value);

  private:
    bool needsRedraw = false;
  };

} // namespace view

#endif // VIEW_HPP
