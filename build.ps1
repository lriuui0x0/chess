C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/scene.vert.glsl -g -o bin/scene.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/scene.frag.glsl -g -o bin/scene.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/shadow.vert.glsl -g -o bin/shadow.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/shadow.frag.glsl -g -o bin/shadow.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/scene.vert.glsl -g -o bin/scene.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/scene.frag.glsl -g -o bin/scene.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/debug_ui.vert.glsl -g -o bin/debug_ui.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/debug_ui.frag.glsl -g -o bin/debug_ui.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/debug_collision.vert.glsl -g -o bin/debug_collision.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/debug_collision.frag.glsl -g -o bin/debug_collision.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/debug_move.vert.glsl -g -o bin/debug_move.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/debug_move.frag.glsl -g -o bin/debug_move.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/blur.vert.glsl -g -o bin/blur.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/blur.frag.glsl -g -o bin/blur.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/menu.vert.glsl -g -o bin/menu.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/menu.frag.glsl -g -o bin/menu.frag.spv

clang-cl /W4 -I C:\VulkanSDK\1.1.114.0\Include /Zi -O0 /EHa -o bin/chess.exe `
    -Wno-logical-op-parentheses -Wno-switch -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    src/main.cpp `
    /link /STACK:0x100000 /LIBPATH:C:/VulkanSDK/1.1.114.0/Lib /NATVIS:misc/debug.natvis user32.lib vulkan-1.lib
