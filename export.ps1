clang-cl /W4 -I 'C:\Program Files\Autodesk\FBX\FBX SDK\2019.5\include' /Zi -O0 /EHa -o bin/fbx_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    fbx_exporter/main.cpp `
    /link /LIBPATH:'C:\Program Files\Autodesk\FBX\FBX SDK\2019.5\lib\vs2017\x64\release' /NATVIS:misc/debug.natvis libfbxsdk-mt.lib libxml2-mt.lib zlib-mt.lib Advapi32.lib

bin/fbx_exporter.exe asset/raw/Board.fbx asset/board.asset 0

bin/fbx_exporter.exe asset/raw/RookDark.fbx asset/rook_black.asset
bin/fbx_exporter.exe asset/raw/KnightDark.fbx asset/knight_black.asset
bin/fbx_exporter.exe asset/raw/BishopDark.fbx asset/bishop_black.asset
bin/fbx_exporter.exe asset/raw/QueenDark.fbx asset/queen_black.asset
bin/fbx_exporter.exe asset/raw/KingDark.fbx asset/king_black.asset
bin/fbx_exporter.exe asset/raw/PawnDark.fbx asset/pawn_black.asset

bin/fbx_exporter.exe asset/raw/RookLight.fbx asset/rook_white.asset
bin/fbx_exporter.exe asset/raw/KnightLight.fbx asset/knight_white.asset
bin/fbx_exporter.exe asset/raw/BishopLight.fbx asset/bishop_white.asset
bin/fbx_exporter.exe asset/raw/QueenLight.fbx asset/queen_white.asset
bin/fbx_exporter.exe asset/raw/KingLight.fbx asset/king_white.asset
bin/fbx_exporter.exe asset/raw/PawnLight.fbx asset/pawn_white.asset

clang-cl /W4 /Zi -O0 /EHa -o bin/otf_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    otf_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis

bin/otf_exporter.exe asset/raw/consola.otf asset/debug_font.asset 36
