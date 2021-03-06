#pragma once

#include "../lib/util.hpp"

#include "windows.h"
#include "windowsx.h"

// TODO: Change this to a library
typedef Void *Window;

enum WindowProcedureRelayMessage
{
    WM_USER_CLOSE = WM_USER,
    WM_USER_SIZE,
};

LRESULT window_procedure(HWND window, UINT message_type, WPARAM wparam, LPARAM lparam)
{
    switch (message_type)
    {
    case WM_CLOSE:
    {
        Bool result = PostMessageA(window, WM_USER_CLOSE, wparam, lparam);
        ASSERT(result);
        return 0;
    }
    case WM_SIZE:
    {
        Bool result = PostMessageA(window, WM_USER_SIZE, wparam, lparam);
        ASSERT(result);
        return 0;
    }
    break;
    }

    return DefWindowProcA(window, message_type, wparam, lparam);
}

Void track_mouse_enter_leave_event(HWND window_handle, DWORD event)
{
    TRACKMOUSEEVENT track_mouse_event_info;
    track_mouse_event_info.cbSize = sizeof(TRACKMOUSEEVENT);
    track_mouse_event_info.dwFlags = event;
    track_mouse_event_info.hwndTrack = window_handle;
    track_mouse_event_info.dwHoverTime = USER_TIMER_MINIMUM;

    if (!TrackMouseEvent(&track_mouse_event_info))
    {
        OutputDebugStringA("Tracking mouse enter / leave failed.\n");
    }
}

Window create_window(Str title, Int client_width, Int client_height, Int window_x, Int window_y, Bool resizable)
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

        HWND window_handle = CreateWindowExA(0, (LPCSTR)title.data, (LPCSTR)title.data, WS_OVERLAPPEDWINDOW & ~(resizable ? 0 : WS_THICKFRAME | WS_MAXIMIZEBOX),
                                             window_x, window_y, window_width, window_height,
                                             NULL, NULL, module_handle, NULL);

        return (Window)window_handle;
    }
    else
    {
        return NULL;
    }
}

Void show_window(Window window)
{
    ShowWindow((HWND)window, SW_SHOW);
}

enum struct WindowMessageType
{
    close,
    resize,
    paint,
    mouse_down,
    mouse_up,
    mouse_move,
    mouse_enter,
    mouse_leave,
    key_down,
    key_up,
};

enum struct WindowMessageResizeType
{
    normal,
    maximize,
    minimize,
};

struct WindowMessageResizeData
{
    WindowMessageResizeType type;
    Int width;
    Int height;
};

enum struct WindowMessageMouseButtonType
{
    left = 0x1,
    right = 0x2,
    middle = 0x4,
};

struct WindowMessageMouseDownData
{
    WindowMessageMouseButtonType button_type;
    Int x;
    Int y;
};

struct WindowMessageMouseUpData
{
    WindowMessageMouseButtonType button_type;
    Int x;
    Int y;
};

struct WindowMessageMouseMoveData
{
    Int x;
    Int y;
};

enum struct WindowMessageKeyCode
{
    key_tab = '\t',

    key_0 = '0',
    key_1 = '1',
    key_2 = '2',
    key_3 = '3',
    key_4 = '4',
    key_5 = '5',
    key_6 = '6',
    key_7 = '7',
    key_8 = '8',
    key_9 = '9',

    key_a = 'A',
    key_b = 'B',
    key_c = 'C',
    key_d = 'D',
    key_e = 'E',
    key_f = 'F',
    key_g = 'G',
    key_h = 'H',
    key_i = 'I',
    key_j = 'J',
    key_k = 'K',
    key_l = 'L',
    key_m = 'M',
    key_n = 'N',
    key_o = 'O',
    key_p = 'P',
    key_q = 'Q',
    key_r = 'R',
    key_s = 'S',
    key_t = 'T',
    key_u = 'U',
    key_v = 'V',
    key_w = 'W',
    key_x = 'X',
    key_y = 'Y',
    key_z = 'Z',

    key_esc = VK_ESCAPE,
};

struct WindowMessageKeyDownData
{
    WindowMessageKeyCode key_code;
};

struct WindowMessageKeyUpData
{
    WindowMessageKeyCode key_code;
};

struct WindowMessage
{
    WindowMessageType type;
    union {
        WindowMessageResizeData resize_data;
        WindowMessageMouseDownData mouse_down_data;
        WindowMessageMouseUpData mouse_up_data;
        WindowMessageMouseMoveData mouse_move_data;
        WindowMessageKeyDownData key_down_data;
        WindowMessageKeyUpData key_up_data;
    };
};

Bool get_window_message(Window window, WindowMessage *message)
{
    Bool got_message = false;

    while (true)
    {
        MSG os_message;
        Bool has_message = PeekMessageA(&os_message, (HWND)window, 0, 0, PM_REMOVE);

        if (has_message)
        {
            if (os_message.message == WM_USER_CLOSE)
            {
                message->type = WindowMessageType::close;

                got_message = true;
                break;
            }
            else if (os_message.message == WM_USER_SIZE)
            {
                message->type = WindowMessageType::resize;
                message->resize_data.width = LOWORD(os_message.lParam);
                message->resize_data.height = HIWORD(os_message.lParam);
                if (os_message.wParam == SIZE_MAXIMIZED)
                {
                    message->resize_data.type = WindowMessageResizeType::maximize;
                }
                else if (os_message.wParam == SIZE_MINIMIZED)
                {
                    message->resize_data.type = WindowMessageResizeType::minimize;
                }
                else
                {
                    message->resize_data.type = WindowMessageResizeType::normal;
                }

                got_message = true;
            }
            else if (os_message.message == WM_PAINT)
            {
                message->type = WindowMessageType::paint;

                DispatchMessageA(&os_message);
                got_message = true;
            }
            else if (os_message.message == WM_LBUTTONDOWN)
            {
                message->type = WindowMessageType::mouse_down;
                message->mouse_down_data.button_type = WindowMessageMouseButtonType::left;
                message->mouse_down_data.x = GET_X_LPARAM(os_message.lParam);
                message->mouse_down_data.y = GET_Y_LPARAM(os_message.lParam);

                got_message = true;
            }
            else if (os_message.message == WM_LBUTTONUP)
            {
                message->type = WindowMessageType::mouse_up;
                message->mouse_up_data.button_type = WindowMessageMouseButtonType::left;
                message->mouse_up_data.x = GET_X_LPARAM(os_message.lParam);
                message->mouse_up_data.y = GET_Y_LPARAM(os_message.lParam);

                got_message = true;
            }
            else if (os_message.message == WM_RBUTTONDOWN)
            {
                message->type = WindowMessageType::mouse_down;
                message->mouse_down_data.button_type = WindowMessageMouseButtonType::right;
                message->mouse_down_data.x = GET_X_LPARAM(os_message.lParam);
                message->mouse_down_data.y = GET_Y_LPARAM(os_message.lParam);

                got_message = true;
            }
            else if (os_message.message == WM_RBUTTONUP)
            {
                message->type = WindowMessageType::mouse_up;
                message->mouse_up_data.button_type = WindowMessageMouseButtonType::right;
                message->mouse_up_data.x = GET_X_LPARAM(os_message.lParam);
                message->mouse_up_data.y = GET_Y_LPARAM(os_message.lParam);

                got_message = true;
            }
            else if (os_message.message == WM_MOUSEMOVE)
            {
                message->type = WindowMessageType::mouse_move;
                message->mouse_move_data.x = GET_X_LPARAM(os_message.lParam);
                message->mouse_move_data.y = GET_Y_LPARAM(os_message.lParam);

                got_message = true;
            }
            else if (os_message.message == WM_SYSKEYDOWN || os_message.message == WM_KEYDOWN)
            {
                if (!(os_message.lParam & (1 << 30)))
                {
                    message->type = WindowMessageType::key_down;
                    UINT char_code = MapVirtualKeyA(os_message.wParam, MAPVK_VK_TO_CHAR);
                    message->key_down_data.key_code = (WindowMessageKeyCode)char_code;

                    got_message = true;
                }
            }
            else if (os_message.message == WM_SYSKEYUP || os_message.message == WM_KEYUP)
            {
                message->type = WindowMessageType::key_up;
                UINT char_code = MapVirtualKeyA(os_message.wParam, MAPVK_VK_TO_CHAR);
                message->key_up_data.key_code = (WindowMessageKeyCode)char_code;

                got_message = true;
            }
            else
            {
                DispatchMessageA(&os_message);
            }

            if (got_message)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    return got_message;
}
