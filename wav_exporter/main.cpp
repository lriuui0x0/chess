#include "../lib/util.hpp"

struct Reader
{
    Str buffer;
    Int pos;
};

Int32 get_name(Int8 data[4])
{
    return ((Int32)data[0]) | ((Int32)data[1] << 8) | ((Int32)data[2] << 16) | ((Int32)data[3] << 24);
}

Int16 read16(Reader *reader)
{
    Int16 result = *(Int16 *)&reader->buffer[reader->pos];
    reader->pos += 2;
    return result;
}

Int32 read32(Reader *reader)
{
    Int32 result = *(Int32 *)&reader->buffer[reader->pos];
    reader->pos += 4;
    return result;
}

Int main(Int argc, CStr *argv)
{
    argc--;
    argv++;

    CStr filename = argv[0];
    CStr output_filename = argv[1];

    Reader reader;
    ASSERT(read_file(filename, &reader.buffer));
    reader.pos = 0;

    ASSERT(read32(&reader) == get_name((Int8 *)"RIFF"));
    Int chunk_size = read32(&reader);
    ASSERT(read32(&reader) == get_name((Int8 *)"WAVE"));

    ASSERT(read32(&reader) == get_name((Int8 *)"fmt "));
    ASSERT(read32(&reader) == 16);
    ASSERT(read16(&reader) == 1);
    Int channel_count = read16(&reader);
    ASSERT(channel_count == 2);
    Int frame_rate = read32(&reader);
    Int byte_rate = read32(&reader);
    Int block_align = read16(&reader);
    Int bits_per_sample = read16(&reader);
    ASSERT(bits_per_sample % 8 == 0);
    Int sample_byte_count = bits_per_sample / 8;
    ASSERT(bits_per_sample == 16);
    ASSERT(byte_rate == frame_rate * channel_count * sample_byte_count);
    ASSERT(block_align == channel_count * sample_byte_count);

    ASSERT(read32(&reader) == get_name((Int8 *)"data"));
    Int data_size = read32(&reader);
    ASSERT(chunk_size == (data_size + 8) + (16 + 8) + 4);
    Int frame_count = data_size / sample_byte_count / channel_count;

    FILE *file_handle = fopen((CStr)output_filename, "wb");
    ASSERT(file_handle);
    fwrite(&frame_count, sizeof(Int32), 1, file_handle);
    fwrite(&frame_rate, sizeof(Int32), 1, file_handle);
    fwrite(&channel_count, sizeof(Int32), 1, file_handle);
    fwrite(&sample_byte_count, sizeof(Int32), 1, file_handle);
    fwrite(reader.buffer.data + reader.pos, sizeof(Int8), frame_count * channel_count * sample_byte_count, file_handle);
}

#include "../lib/util.cpp"
