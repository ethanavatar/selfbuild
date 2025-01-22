#include <windows.h>

inline static struct Window_State *window_get_state_from_handle(void *handle) {
    LONG_PTR ptr = GetWindowLongPtrA(handle, GWLP_USERDATA);
    struct Window_State *state = (struct Window_State *) ptr;
    return state;
}

static LRESULT CALLBACK window_proc(
    HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam
) {
    struct Window_State *state = NULL;

    // @Ref: https://learn.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
    // @Ref: https://stackoverflow.com/questions/4270673/c-c-windows-how-to-keep-track-of-hwnd-context-data
    if (msg == WM_CREATE) {
        CREATESTRUCT *pCreate = (CREATESTRUCT *) lParam;
        state = (struct Window_State *) pCreate->lpCreateParams;
        SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR) state);

    } else {
        state = window_get_state_from_handle(hwnd);
    }

    LRESULT result = 0;
    switch (msg) {

    //case WM_KEYUP:
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN: {
        unsigned int vk_code = wParam;
        if (0) { }
        else if (vk_code == VK_ESCAPE) { state->should_close = true; }
    } break;

    case WM_DESTROY: {
        state->should_close = true;
        PostQuitMessage(0);
    } break;

    case WM_SIZE: {
        RECT client_rect;
        GetClientRect(hwnd, &client_rect);
        state->width  = client_rect.right  - client_rect.left;
        state->height = client_rect.bottom - client_rect.top;
    } break;

    default: {
        result = DefWindowProcA(hwnd, msg, wParam, lParam);
    } break;
    }

    return result;
}
