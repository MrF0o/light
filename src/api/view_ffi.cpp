#include "core/views/View.hpp"
#include <cstring>

using namespace core::view;

extern "C"
{

    View *view_new()
    {
        return new (std::nothrow) View();
    }

    void view_delete(View *view)
    {
        delete view;
    }

    const char *view_to_string(View *view)
    {
        static thread_local std::string result;
        result = view->toString();
        return result.c_str();
    }

    const char *view_get_name(View *view)
    {
        if (!view)
            return "---";
        static thread_local std::string result;
        result = view->getName();
        return result.c_str();
    }

    float view_get_scrollable_size(View *view)
    {
        return view->getScrollableSize();
    }

    float view_get_h_scrollable_size(View *view)
    {
        return view->getHScrollableSize();
    }

    bool view_supports_text_input(View *view)
    {
        if (!view)
            return false;
        return view->supportsTextInput();
    }

    bool view_on_mouse_pressed(View *view, const char *button, float x, float y, int clicks)
    {
        return view->onMousePressed(button, x, y, clicks);
    }

    void view_on_mouse_released(View *view, const char *button, float x, float y)
    {
        view->onMouseReleased(button, x, y);
    }

    bool view_on_mouse_moved(View *view, float x, float y, float dx, float dy)
    {
        return view->onMouseMoved(x, y, dx, dy);
    }

    void view_on_mouse_left(View *view)
    {
        view->onMouseLeft();
    }

    bool view_on_mouse_wheel(View *view, float y, float x)
    {
        return view->onMouseWheel(y, x);
    }

    bool view_on_file_dropped(View *view, const char *filename, float x, float y)
    {
        return view->onFileDropped(filename, x, y);
    }

    void view_on_text_input(View *view, const char *text)
    {
        view->onTextInput(text);
    }

    void view_on_ime_text_editing(View *view, const char *text, int start, int length)
    {
        view->onImeTextEditing(text, start, length);
    }

    void view_on_touch_moved(View *view, float x, float y, float dx, float dy, int i)
    {
        view->onTouchMoved(x, y, dx, dy, i);
    }

    void view_on_scale_change(View *view, float new_scale, float prev_scale)
    {
        view->onScaleChange(new_scale, prev_scale);
    }

    void view_get_content_bounds(View *view, float *x1, float *y1, float *x2, float *y2)
    {
        view->getContentBounds(*x1, *y1, *x2, *y2);
    }

    void view_get_content_offset(View *view, float *offset_x, float *offset_y)
    {
        view->getContentOffset(*offset_x, *offset_y);
    }

    void view_clamp_scroll_position(View *view)
    {
        view->clampScrollPosition();
    }

    void view_update(View *view)
    {
        view->update();
    }

    void view_draw(View *view, void *surface)
    {
        view->draw((RenSurface *)surface);
    }

    void view_draw_background(View *view, void *surface, int r, int g, int b, int a)
    {
        RenColor color = {(unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a};
        view->drawBackground((RenSurface *)surface, color);
    }

    void *view_on_context_menu(View *view, float x, float y)
    {
        return view->onContextMenu(x, y);
    }

    void view_try_close(View *view, void (*do_close)())
    {
        if (do_close)
        {
            view->tryClose([do_close]()
                           { do_close(); });
        }
        else
        {
            view->tryClose(nullptr);
        }
    }

    void *view_get_position_ptr(View *view)
    {
        return &view->position;
    }

    void *view_get_size_ptr(View *view)
    {
        return &view->size;
    }

    void *view_get_scroll_ptr(View *view)
    {
        return &view->scroll;
    }

    bool *view_get_scrollable_ptr(View *view)
    {
        return &view->scrollable;
    }

    const char *view_get_cursor(View *view)
    {
        return view->cursor.c_str();
    }

    void view_set_cursor(View *view, const char *cursor)
    {
        view->cursor = cursor;
    }

    const char *view_get_context(View *view)
    {
        return view->context == ViewContext::Application ? "application" : "session";
    }

    void view_set_context(View *view, const char *context)
    {
        view->context = (strcmp(context, "session") == 0) ? ViewContext::Session : ViewContext::Application;
    }

    bool view_get_scrollable(View *view)
    {
        return view->scrollable;
    }

    void view_set_scrollable(View *view, bool scrollable)
    {
        view->scrollable = scrollable;
    }

    float view_get_current_scale(View *view)
    {
        return view->currentScale;
    }

    void view_set_current_scale(View *view, float scale)
    {
        view->currentScale = scale;
    }

} // extern "C"
