#ifndef VIEW_RENDERER_HPP
#define VIEW_RENDERER_HPP

#include "views/View.hpp"
#include <typeinfo>

extern "C"
{
#include "renderer.h"
}

namespace view
{

  /**
   * Base class for rendering views.
   * ViewRenderer identifies view types and dispatch to appropriate rendering logic.
   *
   * Views have their own render() method for self-rendering, but
   * ViewRenderer provides a centralized rendering pipeline that can:
   * - Set up common rendering state
   * - Handle view hierarchies
   * - Apply effects or transformations
   * - Implement debugging/profiling
   */
  class ViewRenderer
  {
  public:
    ViewRenderer() = default;
    virtual ~ViewRenderer() = default;

    /**
     * Render a view to the given surface.
     * This is the main entry point for rendering.
     *
     * @param view The view to render
     * @param surface The surface to render to
     */
    virtual void render(View *view, RenSurface *surface);
#ifdef LITE_USE_SDL_RENDERER
    virtual void render(View *view, SDL_Renderer *renderer);
#endif

    /**
     * Pre-render hook called before view->draw().
     * Can be used to set up clipping, transformations, etc.
     *
     * @param view The view about to be rendered
     * @param surface The surface to render to
     * @return true if rendering should continue, false to skip
     */
    virtual bool preRender(View *view, RenSurface *surface);
#ifdef LITE_USE_SDL_RENDERER
    virtual bool preRender(View *view, SDL_Renderer *renderer);
#endif

    /**
     * Post-render hook called after view->draw().
     * Can be used to draw overlays, restore state, etc.
     *
     * @param view The view that was just rendered
     * @param surface The surface to render to
     */
    virtual void postRender(View *view, RenSurface *surface);
#ifdef LITE_USE_SDL_RENDERER
    virtual void postRender(View *view, SDL_Renderer *renderer);
#endif

    /**
     * Get type information about a view.
     * This can be used for debugging or type-specific behavior.
     *
     * @param view The view to inspect
     * @return Type name string
     */
    std::string getViewTypeName(const View *view) const;

    /**
     * Check if a view is of a specific type.
     * Use this sparingly - prefer virtual functions for behavior.
     *
     * Example: if (isViewType<DocView>(view)) { ... }
     */
    template <typename T>
    bool isViewType(const View *view) const
    {
      return dynamic_cast<const T *>(view) != nullptr;
    }

  protected:
    /**
     * Render type-specific view content.
     * Override this to provide custom rendering
     * for specific view types.
     *
     * Default implementation calls view->draw().
     *
     * @param view The view to render
     * @param surface The surface to render to
     */
    virtual void renderViewContent(View *view, RenSurface *surface);
#ifdef LITE_USE_SDL_RENDERER
    virtual void renderViewContent(View *view, SDL_Renderer *renderer);
#endif

  private:
    bool clippingEnabled = false;
  };

} // namespace view

#endif // VIEW_RENDERER_HPP
