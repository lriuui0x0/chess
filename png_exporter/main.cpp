#include "../lib/util.hpp"

struct Reader
{
    Str buffer;
    Int pos;
};

struct Chunk
{
    Int length;
    Int8 type[4];
    UInt8 *data;
    UInt CRC;
};

Int8 chunk_type_ihdr[4] = {'I', 'H', 'D', 'R'};
Int8 chunk_type_plte[4] = {'P', 'L', 'T', 'E'};
Int8 chunk_type_idat[4] = {'I', 'D', 'A', 'T'};
Int8 chunk_type_iend[4] = {'I', 'E', 'N', 'D'};

Bool chunk_type_equal(Int8 type1[4], Int8 type2[4])
{
    for (Int i = 0; i < 4; i++)
    {
        if (type1[i] != type2[i])
        {
            return false;
        }
    }
    return true;
}

struct Header
{
    Int width;
    Int height;
    Int8 depth;
    Int8 color_type;
    Int8 compression;
    Int8 filter;
    Int8 interlace;
};

UInt to_little_endian(UInt x)
{
    UInt result;
    result = ((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | ((x & 0xff000000) >> 24);
    return result;
}

Chunk read_chunk(Reader *reader)
{
    Chunk result;
    result.length = to_little_endian(*(UInt *)&reader->buffer[reader->pos]);
    reader->pos += 4;

    *(Int *)&result.type[0] = *(Int *)&reader->buffer[reader->pos];
    reader->pos += 4;

    result.data = &reader->buffer[reader->pos];
    reader->pos += result.length;

    result.CRC = to_little_endian(*(UInt *)&reader->buffer[reader->pos]);
    reader->pos += 4;

    return result;
}

Header read_header(UInt8 *data)
{
    Header result;
    result.width = to_little_endian(*(UInt *)data);
    data += 4;

    result.height = to_little_endian(*(UInt *)data);
    data += 4;

    result.depth = *(UInt8 *)data;
    data += 1;

    result.color_type = *(UInt8 *)data;
    data += 1;

    result.compression = *(UInt8 *)data;
    data += 1;

    result.filter = *(UInt8 *)data;
    data += 1;

    result.interlace = *(UInt8 *)data;
    data += 1;

    return result;
}

struct DeflateProcessor
{
    UInt8 *input;
    Int input_length;
    Int input_pos;

    Int bit_count;
    UInt8 byte;

    Array<Int8> output;
};

Int get_bit(DeflateProcessor *processor, Int bit_count)
{
    Int result = 0;
    Int current_bit_count = 0;
    while (current_bit_count < bit_count)
    {
        if (processor->bit_count > 0)
        {
            Int bit_count_to_load = MIN(bit_count - current_bit_count, processor->bit_count);
            result = ((processor->byte & ((1 << bit_count_to_load) - 1)) << current_bit_count) | result;
            processor->bit_count -= bit_count_to_load;
            processor->byte >>= bit_count_to_load;
            current_bit_count += bit_count_to_load;
        }
        else
        {
            if (processor->input_pos < processor->input_length)
            {
                processor->byte = processor->input[processor->input_pos++];
                processor->bit_count = 8;
            }
            else
            {
                ASSERT(false);
            }
        }
    }
    return result;
}

Int get_byte(DeflateProcessor *processor, Int byte_count)
{
    ASSERT(byte_count < 4);
    processor->bit_count = 0;
    Int result = 0;
    for (Int byte_i = 0; byte_i < byte_count; byte_i++)
    {
        if (processor->input_pos < processor->input_length)
        {
            result = result | (processor->input[processor->input_pos++] << (byte_i * 8));
        }
        else
        {
            ASSERT(false);
        }
    }
    return result;
}

#define MAX_HUFFMAN_CODE_LENGTH (20)
#define MAX_HUFFMAN_SYMBOL_COUNT (300)

struct HuffmanCode
{
    Int symbol_count;
    Int code_length[MAX_HUFFMAN_SYMBOL_COUNT];

    Int max_code_length;
    Int code_count_by_length[MAX_HUFFMAN_CODE_LENGTH];
    Int start_code_by_length[MAX_HUFFMAN_CODE_LENGTH];

    Int symbol_table_offset_by_length[MAX_HUFFMAN_CODE_LENGTH];
    Int symbol_table[MAX_HUFFMAN_SYMBOL_COUNT];
};

Void build_huffman_code(HuffmanCode *huffman)
{
    huffman->max_code_length = 0;
    memset(huffman->code_count_by_length, 0, sizeof(huffman->code_count_by_length));
    for (Int symbol = 0; symbol < huffman->symbol_count; symbol++)
    {
        Int code_length = huffman->code_length[symbol];
        if (code_length > 0)
        {
            huffman->code_count_by_length[code_length]++;
            if (code_length > huffman->max_code_length)
            {
                huffman->max_code_length = code_length;
            }
        }
    }
    ASSERT(huffman->max_code_length >= 1 && huffman->max_code_length < MAX_HUFFMAN_CODE_LENGTH);

    Int current_code = 0;
    huffman->start_code_by_length[0] = current_code;
    huffman->symbol_table_offset_by_length[0] = 0;
    for (Int code_length = 1; code_length <= huffman->max_code_length; code_length++)
    {
        current_code = (current_code + huffman->code_count_by_length[code_length - 1]) << 1;
        huffman->start_code_by_length[code_length] = current_code;
        huffman->symbol_table_offset_by_length[code_length] = huffman->symbol_table_offset_by_length[code_length - 1] + huffman->code_count_by_length[code_length - 1];
    }

    Int symbol_seen_count_by_length[MAX_HUFFMAN_SYMBOL_COUNT] = {};
    for (Int symbol = 0; symbol < huffman->symbol_count; symbol++)
    {
        Int code_length = huffman->code_length[symbol];
        if (code_length > 0)
        {
            Int symbol_offset = huffman->symbol_table_offset_by_length[code_length] + symbol_seen_count_by_length[code_length];
            huffman->symbol_table[symbol_offset] = symbol;
            symbol_seen_count_by_length[code_length]++;
        }
    }
}

Int huffman_decode(DeflateProcessor *processor, HuffmanCode *huffman)
{
    Int code = 0;
    for (Int code_length = 1; code_length <= huffman->max_code_length; code_length++)
    {
        code = (code << 1) | get_bit(processor, 1);
        if (code >= huffman->start_code_by_length[code_length] &&
            code < huffman->start_code_by_length[code_length] + huffman->code_count_by_length[code_length])
        {
            Int symbol_offset = huffman->symbol_table_offset_by_length[code_length] + (code - huffman->start_code_by_length[code_length]);
            Int result = huffman->symbol_table[symbol_offset];
            ASSERT(result >= 0 && result < huffman->symbol_count);
            return result;
        }
    }
    ASSERT(false);
    return -1;
}

Void push_byte(DeflateProcessor *processor, Int8 byte)
{
    *processor->output.push() = byte;
}

Void push_input_range(DeflateProcessor *processor, Int length)
{
    processor->bit_count = 0;
    while (length > 0)
    {
        if (processor->input_pos < processor->input_length)
        {
            *processor->output.push() = processor->input[processor->input_pos++];
            length--;
        }
        else
        {
            ASSERT(false);
        }
    }
}

Void push_output_range(DeflateProcessor *processor, Int length, Int distance)
{
    ASSERT(distance <= processor->output.count);
    Int count = processor->output.count;
    Int start = processor->output.count - distance;
    Int index = start;
    while (length > 0)
    {
        if (index >= count)
        {
            index = start;
        }

        // TODO: Calling push every time is not efficient, should pre-allocate
        *processor->output.push() = processor->output[index++];
        length--;
    }
}

void deflate_decompress(DeflateProcessor *processor)
{
    HuffmanCode fixed_length_huffman;
    fixed_length_huffman.symbol_count = 288;
    for (Int symbol = 0; symbol < fixed_length_huffman.symbol_count; symbol++)
    {
        if (symbol >= 0 && symbol <= 143)
        {
            fixed_length_huffman.code_length[symbol] = 8;
        }
        else if (symbol >= 144 && symbol <= 255)
        {
            fixed_length_huffman.code_length[symbol] = 9;
        }
        else if (symbol >= 256 && symbol <= 279)
        {
            fixed_length_huffman.code_length[symbol] = 7;
        }
        else if (symbol >= 280 && symbol <= 287)
        {
            fixed_length_huffman.code_length[symbol] = 8;
        }
        else
        {
            ASSERT(false);
        }
    }
    build_huffman_code(&fixed_length_huffman);

    HuffmanCode fixed_distance_huffman;
    fixed_distance_huffman.symbol_count = 32;
    for (Int symbol = 0; symbol < fixed_distance_huffman.symbol_count; symbol++)
    {
        fixed_distance_huffman.code_length[symbol] = 5;
    }
    build_huffman_code(&fixed_distance_huffman);

    Int is_final = false;
    while (!is_final)
    {
        is_final = get_bit(processor, 1);
        Int block_type = get_bit(processor, 2);
        switch (block_type)
        {
        case 0x0:
        {
            Int length = get_byte(processor, 2);
            Int nlength = get_byte(processor, 2);
            ASSERT(nlength == ~((UInt16)length));

            push_input_range(processor, length);
        }
        break;

        case 0x1:
        case 0x2:
        {
            HuffmanCode dynamic_length_huffman;
            HuffmanCode dynamic_distance_huffman;
            if (block_type == 2)
            {
                Int length_code_count = get_bit(processor, 5) + 257;
                ASSERT(length_code_count >= 257 && length_code_count <= 286);
                Int distance_code_count = get_bit(processor, 5) + 1;
                ASSERT(distance_code_count >= 1 && distance_code_count <= 32);
                Int code_length_count = get_bit(processor, 4) + 4;
                ASSERT(code_length_count >= 4 && code_length_count <= 19);

                HuffmanCode code_length_huffman;
                code_length_huffman.symbol_count = 19;
                Int code_length_index[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
                for (Int i = 0; i < 19; i++)
                {
                    Int symbol = code_length_index[i];
                    if (i < code_length_count)
                    {
                        code_length_huffman.code_length[symbol] = get_bit(processor, 3);
                    }
                    else
                    {
                        code_length_huffman.code_length[symbol] = 0;
                    }
                }
                build_huffman_code(&code_length_huffman);

                Int code_length_both[MAX_HUFFMAN_SYMBOL_COUNT * 2];
                Int code_length_both_count = 0;
                while (code_length_both_count < length_code_count + distance_code_count)
                {
                    Int code_length = huffman_decode(processor, &code_length_huffman);
                    if (code_length >= 0 && code_length <= 15)
                    {
                        code_length_both[code_length_both_count++] = code_length;
                    }
                    else if (code_length == 16)
                    {
                        Int copy_count = get_bit(processor, 2) + 3;
                        ASSERT(copy_count >= 3 && copy_count <= 6);
                        Int last_code_length = code_length_both[code_length_both_count - 1];
                        for (Int i = 0; i < copy_count; i++)
                        {
                            code_length_both[code_length_both_count++] = last_code_length;
                        }
                    }
                    else if (code_length == 17)
                    {
                        Int copy_count = get_bit(processor, 3) + 3;
                        ASSERT(copy_count >= 3 && copy_count <= 10);
                        for (Int i = 0; i < copy_count; i++)
                        {
                            code_length_both[code_length_both_count++] = 0;
                        }
                    }
                    else if (code_length == 18)
                    {
                        Int copy_count = get_bit(processor, 7) + 11;
                        ASSERT(copy_count >= 11 && copy_count <= 138);
                        for (Int i = 0; i < copy_count; i++)
                        {
                            code_length_both[code_length_both_count++] = 0;
                        }
                    }
                    else
                    {
                        ASSERT(false);
                    }
                }
                ASSERT(code_length_both_count == length_code_count + distance_code_count);

                dynamic_length_huffman.symbol_count = length_code_count;
                memcpy(dynamic_length_huffman.code_length, code_length_both, length_code_count * sizeof(dynamic_length_huffman.code_length[0]));
                build_huffman_code(&dynamic_length_huffman);

                dynamic_distance_huffman.symbol_count = distance_code_count;
                memcpy(dynamic_distance_huffman.code_length, code_length_both + length_code_count, distance_code_count * sizeof(dynamic_distance_huffman.code_length[0]));
                build_huffman_code(&dynamic_distance_huffman);
            }

            HuffmanCode *length_huffman;
            HuffmanCode *distance_huffman;
            if (block_type == 1)
            {
                length_huffman = &fixed_length_huffman;
                distance_huffman = &fixed_distance_huffman;
            }
            else
            {
                length_huffman = &dynamic_length_huffman;
                distance_huffman = &dynamic_distance_huffman;
            }

            while (true)
            {
                Int code = huffman_decode(processor, length_huffman);
                if (code == 256)
                {
                    break;
                }
                else if (code < 256)
                {
                    push_byte(processor, code);
                }
                else
                {
                    Int length;
                    if (code >= 257 && code <= 264)
                    {
                        length = code - 257 + 3;
                        ASSERT(length >= 3 && length <= 10);
                    }
                    else if (code >= 265 && code <= 284)
                    {
                        Int extra_bit_count = (code - 265) / 4 + 1;
                        Int extra_bit = get_bit(processor, extra_bit_count);
                        length = (((code - 265) % 4 + 4) << extra_bit_count) + 3 + extra_bit;
                        ASSERT(length >= 11 && length <= 257);
                    }
                    else if (code == 285)
                    {
                        length = 258;
                    }
                    else
                    {
                        length = -1;
                        ASSERT(false);
                    }
                    ASSERT(length >= 3 && length <= 258);

                    code = huffman_decode(processor, distance_huffman);
                    Int distance;
                    if (code >= 0 && code <= 3)
                    {
                        distance = code + 1;
                        ASSERT(distance >= 1 && distance <= 4);
                    }
                    else if (code >= 4 && code <= 29)
                    {
                        Int extra_bit_count = code / 2 - 1;
                        Int extra_bit = get_bit(processor, extra_bit_count);
                        distance = ((code % 2 + 2) << extra_bit_count) + 1 + extra_bit;
                        ASSERT(distance >= 5 && distance <= 32768);
                    }
                    else
                    {
                        distance = -1;
                        ASSERT(false);
                    }
                    ASSERT(distance >= 1 && distance <= 32768);

                    push_output_range(processor, length, distance);
                }
            }
        }
        break;

        case 0x3:
        {
            ASSERT(false);
        }
        break;
        }
    }
}

struct Image
{
    Int width;
    Int height;
    UInt8 *data;
};

UInt8 paeth_filter(UInt8 a, UInt8 b, UInt8 c)
{
    Int p = a + b - c;
    Int pa = ABS(p - a);
    Int pb = ABS(p - b);
    Int pc = ABS(p - c);
    if (pa <= pb && pa <= pc)
    {
        return a;
    }
    else if (pb <= pc)
    {
        return b;
    }
    else
    {
        return c;
    }
}

void write_bitmap(Str filename, Image *image)
{
    FILE *file_handle = fopen((CStr)filename.data, "wb");
    ASSERT(file_handle);

    UInt8 bf_type[2] = {'B', 'M'};
    fwrite(&bf_type, 2, 1, file_handle);
    UInt32 bf_size = 14 + 40 + 4 * image->width * image->height;
    fwrite(&bf_size, 4, 1, file_handle);
    UInt16 bf_reserved1 = 0;
    fwrite(&bf_reserved1, 2, 1, file_handle);
    UInt16 bf_reserved2 = 0;
    fwrite(&bf_reserved2, 2, 1, file_handle);
    UInt32 bf_off_bits = 14 + 40;
    fwrite(&bf_off_bits, 4, 1, file_handle);

    UInt32 bi_size = 40;
    fwrite(&bi_size, 4, 1, file_handle);
    UInt32 bi_width = image->width;
    fwrite(&bi_width, 4, 1, file_handle);
    UInt32 bi_height = image->height;
    fwrite(&bi_height, 4, 1, file_handle);
    UInt16 bi_planes = 1;
    fwrite(&bi_planes, 2, 1, file_handle);
    UInt16 bi_bit_count = 32;
    fwrite(&bi_bit_count, 2, 1, file_handle);
    UInt32 bi_compression = 0;
    fwrite(&bi_compression, 4, 1, file_handle);
    UInt32 bi_size_image = 0;
    fwrite(&bi_size_image, 4, 1, file_handle);
    UInt32 bi_x_pels_per_meter = 0;
    fwrite(&bi_x_pels_per_meter, 4, 1, file_handle);
    UInt32 bi_y_pels_per_meter = 0;
    fwrite(&bi_y_pels_per_meter, 4, 1, file_handle);
    UInt32 bi_clr_used = 0;
    fwrite(&bi_clr_used, 4, 1, file_handle);
    UInt32 bi_clr_important = 0;
    fwrite(&bi_clr_important, 4, 1, file_handle);

    UInt8 *row = image->data;
    for (Int y = 0; y < image->height; y++)
    {
        for (Int x = 0; x < image->width; x++)
        {
            UInt32 color = (row[x * 3] << 16) + (row[x * 3 + 1] << 8) + row[x * 3 + 2];
            fwrite(&color, 4, 1, file_handle);
        }
        row += image->width * 3;
    }

    ASSERT(fclose(file_handle) == 0);
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

    reader.pos += 8;
    Chunk chunk_ihdr = read_chunk(&reader);
    ASSERT(chunk_type_equal(chunk_ihdr.type, chunk_type_ihdr));
    Header header = read_header(chunk_ihdr.data);

    UInt8 *zlib_data = null;
    Int zlib_data_length = 0;
    while (true)
    {
        Chunk chunk = read_chunk(&reader);
        if (chunk_type_equal(chunk.type, chunk_type_idat))
        {
            zlib_data = (UInt8 *)realloc(zlib_data, zlib_data_length + chunk.length);
            memcpy(zlib_data + zlib_data_length, chunk.data, chunk.length);
            zlib_data_length += chunk.length;
        }
        else if (chunk_type_equal(chunk.type, chunk_type_iend))
        {
            break;
        }
    }

    DeflateProcessor processor;
    processor.input = zlib_data + 2;
    processor.input_length = zlib_data_length - 6;
    processor.input_pos = 0;
    processor.bit_count = 0;
    processor.output = create_array<Int8>();
    deflate_decompress(&processor);

    Image image;
    image.width = header.width;
    image.height = header.height;
    image.data = (UInt8 *)malloc(image.width * image.height * 3);

    Int depth = 3;
    Int output_pos = 0;
    UInt8 *row = image.data;
    for (Int y = 0; y < image.height; y++)
    {
        Int filter_type = processor.output[output_pos++];
        switch (filter_type)
        {
        case 0:
        {
            UInt8 *byte = row;
            for (Int x = 0; x < image.width * depth; x++)
            {
                *byte++ = processor.output[output_pos++];
            }
        }
        break;

        case 1:
        {
            UInt8 *byte = row;
            for (Int x = 0; x < image.width * depth; x++)
            {
                UInt8 a = x < depth ? 0 : *(byte - depth);
                *byte++ = processor.output[output_pos++] + a;
            }
        }
        break;

        case 2:
        {
            UInt8 *byte = row;
            for (Int x = 0; x < image.width * depth; x++)
            {
                UInt8 b = y == 0 ? 0 : *(byte - (image.width * depth));
                *byte++ = processor.output[output_pos++] + b;
            }
        }
        break;

        case 3:
        {
            UInt8 *byte = row;
            for (Int x = 0; x < image.width * depth; x++)
            {
                UInt8 a = x < depth ? 0 : *(byte - depth);
                UInt8 b = y == 0 ? 0 : *(byte - (image.width * depth));
                *byte++ = processor.output[output_pos++] + (a + b) / 2;
            }
        }
        break;

        case 4:
        {
            UInt8 *byte = row;
            for (Int x = 0; x < image.width * depth; x++)
            {
                UInt8 a = x < depth ? 0 : *(byte - depth);
                UInt8 b = y == 0 ? 0 : *(byte - (image.width * depth));
                UInt8 c = x < depth || y == 0 ? 0 : *(byte - (image.width * depth) - depth);
                *byte++ = processor.output[output_pos++] + paeth_filter(a, b, c);
            }
        }
        break;

        default:
        {
            ASSERT(false);
        }
        break;
        }

        row += image.width * depth;
    }
    ASSERT(output_pos == processor.output.count);

    FILE *file_handle = fopen((CStr)output_filename, "wb");
    ASSERT(file_handle);
    fwrite(&image.width, 4, 1, file_handle);
    fwrite(&image.height, 4, 1, file_handle);
    row = image.data;
    for (Int y = 0; y < image.height; y++)
    {
        UInt8 *byte = row;
        for (Int x = 0; x < image.width; x ++)
        {
            UInt8 value_r = *byte++;
            UInt8 value_g = *byte++;
            UInt8 value_b = *byte++;
            UInt8 value = (value_r + value_g + value_b) / 3;
            fwrite(&value, 1, 1, file_handle);
        }
        row += image.width * depth;
    }
}

#include "../lib/util.cpp"
