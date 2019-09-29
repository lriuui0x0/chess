clang-cl /W4 /Zi -O0 /EHa -o bin/png_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    png_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis

bin/png_exporter.exe asset/raw/Board.png asset/board_lightmap.asset
bin/png_exporter.exe asset/raw/Rook.png asset/rook_lightmap.asset
bin/png_exporter.exe asset/raw/Knight.png asset/knight_lightmap.asset
bin/png_exporter.exe asset/raw/Bishop.png asset/bishop_lightmap.asset
bin/png_exporter.exe asset/raw/Queen.png asset/queen_lightmap.asset
bin/png_exporter.exe asset/raw/King.png asset/king_lightmap.asset
bin/png_exporter.exe asset/raw/Pawn.png asset/pawn_lightmap.asset
