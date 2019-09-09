#pragma once

#include "util.hpp"

Handle load_library(Str library_name);
Void *load_library_func(Handle library, Str func_name);

Handle create_window(Str title, Int client_width, Int client_height, Int window_x, Int window_y);
