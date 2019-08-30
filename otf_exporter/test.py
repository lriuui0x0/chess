import freetype

face = freetype.Face('asset/raw/consola.otf')
face.set_char_size(48*64)
face.load_char('a')
print(face.glyph.metrics.width, face.glyph.metrics.height)
face.load_char('b')
print(face.glyph.metrics.width, face.glyph.metrics.height)
