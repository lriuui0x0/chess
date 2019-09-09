#include "os.hpp"
#include <windows.h>

Handle load_library(Str library_name)
{
    return LoadLibraryA((LPCSTR)library_name.data);
}

Void *load_library_func(Handle library, Str func_name)
{
    return (Void *)GetProcAddress((HMODULE)library, (LPCSTR)func_name.data);
}

LRESULT window_procedure_(HWND window, UINT message_type, WPARAM wparam, LPARAM lparam)
{
    return DefWindowProcA(window, message_type, wparam, lparam);
}

Handle create_window_(Str title, Int client_width, Int client_height, Int window_x, Int window_y)
{
    HINSTANCE module_handle = GetModuleHandleA(NULL);

    WNDCLASSEXA window_class = {};
    window_class.cbSize = sizeof(WNDCLASSEX);
    window_class.style = CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = module_handle;
    window_class.lpszClassName = (LPCSTR)title.data;

    if (RegisterClassExA(&window_class))
    {
        RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = client_width;
        rect.bottom = client_height;
        AdjustWindowRectEx(&rect, WS_BORDER | WS_CAPTION, false, 0);
        Int window_width = rect.right - rect.left;
        Int window_height = rect.bottom - rect.top;

        HWND window_handle = CreateWindowExA(0, (LPCSTR)title.data, (LPCSTR)title.data, WS_OVERLAPPEDWINDOW,
                                             window_x, window_y, window_width, window_height,
                                             NULL, NULL, module_handle, NULL);

        return (Handle)window_handle;
    }
    else
    {
        return NULL;
    }
}
