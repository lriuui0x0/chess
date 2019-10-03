
clang-cl /W4 /Zi -O0 /EHa -o bin/bitboard.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    bitboard/main.cpp `
    /link /NATVIS:misc/debug.natvis user32.lib

bin/bitboard.exe asset/bitboard.asset
