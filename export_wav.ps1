clang-cl /W4 /Zi -O0 /EHa -o bin/wav_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    wav_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis

bin/wav_exporter.exe asset/raw/error.wav asset/sound_error.asset
