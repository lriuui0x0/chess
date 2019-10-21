clang-cl /W4 /Zi -O0 /EHa -o bin/wav_exporter.exe `
    -Wno-logical-op-parentheses -Wno-unused-label -Wno-unused-parameter -Wno-missing-field-initializers -Wno-missing-braces -Wno-writable-strings -Wno-deprecated-declarations `
    wav_exporter/main.cpp `
    /link /NATVIS:misc/debug.natvis

bin/wav_exporter.exe asset/raw/click_high.wav asset/sound_click_high.asset
bin/wav_exporter.exe asset/raw/click_low.wav asset/sound_click_low.asset
bin/wav_exporter.exe asset/raw/move.wav asset/sound_move.asset
bin/wav_exporter.exe asset/raw/error.wav asset/sound_error.asset
