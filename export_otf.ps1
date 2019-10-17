clang-cl /W4 /Zi -O0 /EHa -o bin/otf_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    otf_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis

bin/otf_exporter.exe asset/raw/consola.otf asset/debug_font.asset 16
bin/otf_exporter.exe asset/raw/cinzel.otf asset/menu_large_font.asset 60
bin/otf_exporter.exe asset/raw/cinzel.otf asset/menu_medium_font.asset 48
bin/otf_exporter.exe asset/raw/cinzel.otf asset/menu_small_font.asset 36
