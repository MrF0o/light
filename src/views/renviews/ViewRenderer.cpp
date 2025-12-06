#include "ViewRenderer.hpp"
#include <cxxabi.h>
#include <memory>

extern "C"
{
#include "renwindow.h"
}

namespace view
{

  void ViewRenderer::render(View *view, RenSurface *surface)
  {
    if (!view || !surface)
    {
      return;
    }

    if (!preRender(view, surface))
    {
      return;
    }

    renderViewContent(view, surface);

    postRender(view, surface);
  }

#ifdef LITE_USE_SDL_RENDERER
  void ViewRenderer::render(View *view, SDL_Renderer *renderer)
  {
    if (!view || !renderer)
    {
      return;
    }

    if (!preRender(view, renderer))
    {
      return;
    }

    renderViewContent(view, renderer);

    postRender(view, renderer);
  }
#endif

  bool ViewRenderer::preRender(View *view, RenSurface *surface)
  {
    RenRect clipRect = {
        static_cast<int>(view->position.x),
        static_cast<int>(view->position.y),
        static_cast<int>(view->size.x),
        static_cast<int>(view->size.y)};

    if (surface && surface->surface)
    {
      SDL_Rect sr;
      sr.x = clipRect.x * surface->scale;
      sr.y = clipRect.y * surface->scale;
      sr.w = clipRect.width * surface->scale;
      sr.h = clipRect.height * surface->scale;

      SDL_SetSurfaceClipRect(surface->surface, &sr);
      clippingEnabled = true;
    }

    return true;
  }

#ifdef LITE_USE_SDL_RENDERER
  bool ViewRenderer::preRender(View *view, SDL_Renderer *renderer)
  {
    RenRect clipRect = {
        static_cast<int>(view->position.x),
        static_cast<int>(view->position.y),
        static_cast<int>(view->size.x),
        static_cast<int>(view->size.y)};

    RenWindow *window = ren_get_target_window();
    int scale = 1;
    if (window)
    {
      RenSurface rs = renwin_get_surface(window);
      scale = rs.scale;
    }

    SDL_Rect sr;
    sr.x = clipRect.x * scale;
    sr.y = clipRect.y * scale;
    sr.w = clipRect.width * scale;
    sr.h = clipRect.height * scale;

    SDL_SetRenderClipRect(renderer, &sr);
    clippingEnabled = true;
    return true;
  }
#endif

  void ViewRenderer::postRender(View *view, RenSurface *surface)
  {
    if (clippingEnabled)
    {
      if (surface && surface->surface)
      {
        SDL_SetSurfaceClipRect(surface->surface, NULL);
        clippingEnabled = false;
      }
    }
  }

#ifdef LITE_USE_SDL_RENDERER
  void ViewRenderer::postRender(View *view, SDL_Renderer *renderer)
  {
    if (clippingEnabled)
    {
      SDL_SetRenderClipRect(renderer, NULL);
      clippingEnabled = false;
    }
  }
#endif

  void ViewRenderer::renderViewContent(View *view, RenSurface *surface)
  {
    view->draw(surface);
  }

#ifdef LITE_USE_SDL_RENDERER
  void ViewRenderer::renderViewContent(View *view, SDL_Renderer *renderer)
  {
    view->draw(renderer);
  }
#endif

  std::string ViewRenderer::getViewTypeName(const View *view) const
  {
    if (!view)
    {
      return "nullptr";
    }

    const std::type_info &ti = typeid(*view);
    const char *mangled = ti.name();

    // Demangle the name for readability
    int status = 0;
    std::unique_ptr<char, void (*)(void *)> demangled(
        abi::__cxa_demangle(mangled, nullptr, nullptr, &status), std::free);

    if (status == 0 && demangled)
    {
      std::string result = demangled.get();
      // Remove namespace prefix if present
      size_t pos = result.find_last_of(':');
      if (pos != std::string::npos && pos + 1 < result.length())
      {
        return result.substr(pos + 1);
      }
      return result;
    }

    return mangled;
  }

} // namespace view
