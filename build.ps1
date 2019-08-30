C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=vert src/vert.glsl -g -o bin/vert.spv
C:\VulkanSDK\1.1.114.0\Bin\glslc.exe -fshader-stage=frag src/frag.glsl -g -o bin/frag.spv

clang-cl /W4 -I C:\VulkanSDK\1.1.114.0\Include /Zi -O0 /EHa -o bin/chess.exe `
    -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    src/main.cpp `
    /link /LIBPATH:C:/VulkanSDK/1.1.114.0/Lib /NATVIS:misc/debug.natvis user32.lib vulkan-1.lib

clang-cl /W4 -I 'C:\Program Files\Autodesk\FBX\FBX SDK\2019.5\include' /Zi -O0 /EHa -o bin/fbx_exporter.exe `
    -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    fbx_exporter/main.cpp `
    /link /LIBPATH:'C:\Program Files\Autodesk\FBX\FBX SDK\2019.5\lib\vs2017\x64\release' /NATVIS:misc/debug.natvis libfbxsdk-mt.lib libxml2-mt.lib zlib-mt.lib Advapi32.lib

clang-cl /W4 /Zi -O0 /EHa -o bin/otf_exporter.exe `
    -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    otf_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis
