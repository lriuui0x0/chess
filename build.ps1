C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/scene.vert.glsl -g -o bin/scene.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/scene.frag.glsl -g -o bin/scene.frag.spv

C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/debug_ui.vert.glsl -g -o bin/debug_ui.vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/debug_ui.frag.glsl -g -o bin/debug_ui.frag.spv

clang-cl /W4 -I C:\VulkanSDK\1.1.114.0\Include /Zi -O0 /EHa -o bin/chess.exe `
    -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    src/main.cpp `
    /link /LIBPATH:C:/VulkanSDK/1.1.114.0/Lib /NATVIS:misc/debug.natvis user32.lib vulkan-1.lib
