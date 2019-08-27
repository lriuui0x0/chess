#include "../src/util.cpp"

#include "char_string.cpp"

struct Reader
{
    Str buffer;
    Int pos;
};

typedef Int8 OtfTag[4];

OtfTag otf_tag_cmap = {'c', 'm', 'a', 'p'};
OtfTag otf_tag_cff = {'C', 'F', 'F', ' '};

struct OtfOffsetTable
{
    UInt32 version;
    UInt16 table_count;
    UInt16 search_range;
    UInt16 entry_selector;
    UInt16 range_shift;
};

struct OtfTableRecord
{
    OtfTag tag;
    UInt32 checksum;
    UInt32 offset;
    UInt32 length;

    Void *table;
};

struct OtfEncodingFormat4
{
    UInt16 format;
    UInt16 length;
    UInt16 language;
    UInt16 segment_count_x2;
    UInt16 search_range;
    UInt16 entry_selector;
    UInt16 range_shift;

    UInt16 *end_code;
    UInt16 *start_code;
    Int16 *id_delta;
    UInt16 *id_range_offset;
    UInt16 *glyph_id;
};

struct OtfEncodingRecord
{
    UInt16 platform_id;
    UInt16 encoding_id;
    UInt32 offset;
    Void *table;
};

struct OtfCmapTable
{
    UInt16 version;
    UInt16 encoding_table_count;
    OtfEncodingRecord *encoding_records;
};

struct CffIndex
{
    UInt16 count;
    UInt8 offset_size;
    Void *offset;
    Void *data;

    Void *objects;
};

enum struct CffDictValueType
{
    integer,
    real,
    array,
};

struct CffDictValue
{
    CffDictValueType type;
    union {
        Int32 integer;
        Real32 real;
        Array<CffDictValue> array;
    };
};

struct CffDictEntry
{
    CffDictValue value;
    UInt16 key;
};

struct CffDict
{
    Array<CffDictEntry> entries;
};

struct CffHeader
{
    UInt8 major;
    UInt8 minor;
    UInt8 header_size;
    UInt8 offset_size;
};

struct CffTable
{
    CffHeader header;
    CffIndex name_index;
    CffIndex top_index;
    CffIndex char_string_index;
    CffDict private_dict;
    CffIndex subr_index;
};

struct OtfFont
{
    OtfOffsetTable offset_table;
    OtfTableRecord *table_records;
};

Void peek8(Reader *reader, OUT UInt8 *result)
{
    assert(reader->pos + 1 <= reader->buffer.length);
    *result = reader->buffer[reader->pos];
}

Void read8(Reader *reader, OUT UInt8 *result)
{
    assert(reader->pos + 1 <= reader->buffer.length);
    *result = reader->buffer[reader->pos++];
}

Void read16(Reader *reader, OUT UInt16 *result)
{
    assert(reader->pos + 2 <= reader->buffer.length);
    *result = 0;
    *result |= reader->buffer[reader->pos++] << 8;
    *result |= reader->buffer[reader->pos++];
}

Void read32(Reader *reader, OUT UInt32 *result)
{
    assert(reader->pos + 4 <= reader->buffer.length);
    *result = 0;
    *result |= reader->buffer[reader->pos++] << 24;
    *result |= reader->buffer[reader->pos++] << 16;
    *result |= reader->buffer[reader->pos++] << 8;
    *result |= reader->buffer[reader->pos++];
}

Void read_tag(Reader *reader, OUT OtfTag *result)
{
    assert(reader->pos + 4 <= reader->buffer.length);

    Int8 *pointer = (Int8 *)result;
    *pointer++ = reader->buffer[reader->pos++];
    *pointer++ = reader->buffer[reader->pos++];
    *pointer++ = reader->buffer[reader->pos++];
    *pointer++ = reader->buffer[reader->pos++];
}

UInt16 to_little_endian16(UInt16 number)
{
    return ((number & 0xff) << 8) | ((number & 0xff00) >> 8);
}

Bool tag_equal(OtfTag tag1, OtfTag tag2)
{
    return tag1[0] == tag2[0] &&
           tag1[1] == tag2[1] &&
           tag1[2] == tag2[2] &&
           tag1[3] == tag2[3];
}

UInt get_cff_index_data_start(CffIndex *index)
{
    return 2 + 1 + index->offset_size * (index->count + 1);
}

UInt get_cff_index_offset(CffIndex *index, Int offset_i)
{
    UInt result = 0;
    UInt8 *pointer = (UInt8 *)index->offset + offset_i * index->offset_size;

    if (index->offset_size == 1)
    {
        result = *pointer;
    }
    else if (index->offset_size == 2)
    {
        result |= *pointer++ << 8;
        result |= *pointer;
    }
    else if (index->offset_size == 3)
    {
        result |= *pointer++ << 16;
        result |= *pointer++ << 8;
        result |= *pointer;
    }
    else if (index->offset_size == 4)
    {
        result |= *pointer++ << 24;
        result |= *pointer++ << 16;
        result |= *pointer++ << 8;
        result |= *pointer;
    }
    else
    {
        assert(false);
        return 0;
    }

    assert(result >= 1);
    return result - 1;
}

void read_cff_index(Reader *reader, OUT CffIndex *index)
{
    read16(reader, &index->count);

    if (index->count > 0)
    {
        read8(reader, &index->offset_size);

        index->offset = &reader->buffer[reader->pos];
        reader->pos += (index->count + 1) * index->offset_size;

        index->data = &reader->buffer[reader->pos];
        UInt end_offset = get_cff_index_offset(index, index->count);
        reader->pos += end_offset;
    }
    else
    {
        index->offset_size = 0;
        index->offset = NULL;
        index->data = NULL;
    }
}

void read_cff_dict_single_value(Reader *reader, OUT CffDictValue *value)
{
    UInt8 byte0;
    read8(reader, &byte0);

    if (byte0 >= 32 && byte0 <= 246)
    {
        value->type = CffDictValueType::integer;
        value->integer = (Int)byte0 - 139;
        assert(value->integer >= -107 && value->integer <= 107);
    }
    else if (byte0 >= 247 && byte0 <= 250)
    {
        value->type = CffDictValueType::integer;
        value->integer = ((Int)byte0 - 247) * 256;

        UInt8 byte1;
        read8(reader, &byte1);
        value->integer += (Int)byte1 + 108;
        assert(value->integer >= 108 && value->integer <= 1131);
    }
    else if (byte0 >= 251 && byte0 <= 254)
    {
        value->type = CffDictValueType::integer;
        value->integer = -((Int)byte0 - 251) * 256;

        UInt8 byte1;
        read8(reader, &byte1);
        value->integer += -(Int)byte1 - 108;
        assert(value->integer >= -1131 && value->integer <= -108);
    }
    else if (byte0 == 28)
    {
        value->type = CffDictValueType::integer;

        UInt8 byte1;
        read8(reader, &byte1);
        UInt8 byte2;
        read8(reader, &byte2);

        Int16 integer_value = ((Int)byte1 << 8) | (Int)byte2;
        value->integer = integer_value;
        assert(value->integer >= -32768 && value->integer <= 32767);
    }
    else if (byte0 == 29)
    {
        value->type = CffDictValueType::integer;

        UInt8 byte1;
        read8(reader, &byte1);
        UInt8 byte2;
        read8(reader, &byte2);
        UInt8 byte3;
        read8(reader, &byte3);
        UInt8 byte4;
        read8(reader, &byte4);

        value->integer = byte1 << 24;
        value->integer += byte2 << 16;
        value->integer += byte3 << 8;
        value->integer += byte4;
    }
    else if (byte0 == 30)
    {
        value->type = CffDictValueType::real;

        // NOTE:
        // part == 0 : integer
        // part == 1 : fraction
        // part == 2 : positive exponent
        // part == 3 : negative exponent
        Int part = 0;
        Real integer_part = 0.0f;
        Real fraction_part = 0.0f;
        Int fraction_count = 0;
        Real exponent_part = 0.0f;
        Bool is_negative = false;
        Bool found_end = false;

        while (!found_end)
        {
            UInt8 byte1;
            read8(reader, &byte1);

            UInt8 nibble[2];
            nibble[0] = (byte1 >> 4) & 0xf;
            nibble[1] = byte1 & 0xf;

            for (Int i = 0; i < 2; i++)
            {
                if (nibble[i] >= 0 && nibble[i] <= 9)
                {
                    if (part == 0)
                    {
                        integer_part = integer_part * 10 + (Real)nibble[i];
                    }
                    else if (part == 1)
                    {
                        fraction_part = fraction_part * 10 + (Real)nibble[i];
                        fraction_count++;
                    }
                    else if (part == 2 || part == 3)
                    {
                        exponent_part = exponent_part * 10 + (Real)nibble[i];
                    }
                    else
                    {
                        assert(false);
                    }
                }
                else if (nibble[i] == 0xa)
                {
                    part = 1;
                }
                else if (nibble[i] == 0xb)
                {
                    part = 3;
                }
                else if (nibble[i] == 0xc)
                {
                    part = 4;
                }
                else if (nibble[i] == 0xd)
                {
                    assert(false);
                }
                else if (nibble[i] == 0xe)
                {
                    assert(!is_negative);
                    is_negative = true;
                }
                else if (nibble[i] == 0xf)
                {
                    found_end = true;
                    break;
                }
            }
        }

        value->real = integer_part + fraction_part * powf(10, -fraction_count);

        if (part == 2)
        {
            value->real = value->real * powf(10, exponent_part);
        }
        else if (part == 3)
        {
            value->real = value->real * powf(10, -exponent_part);
        }

        if (is_negative)
        {
            value->real = -value->real;
        }
    }
    else
    {
        assert(false);
    }
}

void read_cff_dict_entry(Reader *reader, OUT CffDictEntry *entry)
{
    read_cff_dict_single_value(reader, &entry->value);

    while (true)
    {
        UInt8 byte0;
        peek8(reader, &byte0);

        if (byte0 >= 0 && byte0 <= 21)
        {
            read8(reader, &byte0);

            if (byte0 == 12)
            {
                UInt8 byte1;
                read8(reader, &byte1);

                entry->key = (byte0 << 8) | byte1;
            }
            else
            {
                entry->key = byte0;
            }

            break;
        }
        else
        {
            if (entry->value.type != CffDictValueType::array)
            {
                CffDictValue current_value = entry->value;
                entry->value.type = CffDictValueType::array;
                entry->value.array = create_array<CffDictValue>();
                *entry->value.array.push() = current_value;
            }

            read_cff_dict_single_value(reader, entry->value.array.push());
        }
    }
}

void read_cff_dict(Reader *reader, Int end_pos, OUT CffDict *dict)
{
    dict->entries = create_array<CffDictEntry>();

    while (reader->pos < end_pos)
    {
        read_cff_dict_entry(reader, dict->entries.push());
    }
}

Int main(Int argc, RawStr *argv)
{
    argc--;
    argv++;

    Reader reader;
    assert(read_file("../asset/raw/consola.otf", &reader.buffer));
    reader.pos = 0;

    OtfFont font;
    read32(&reader, &font.offset_table.version);
    read16(&reader, &font.offset_table.table_count);
    read16(&reader, &font.offset_table.search_range);
    read16(&reader, &font.offset_table.entry_selector);
    read16(&reader, &font.offset_table.range_shift);

    font.table_records = (OtfTableRecord *)malloc(sizeof(OtfTableRecord) * font.offset_table.table_count);
    for (Int table_index = 0; table_index < font.offset_table.table_count; table_index++)
    {
        OtfTableRecord *table_record = &font.table_records[table_index];
        read_tag(&reader, &table_record->tag);
        read32(&reader, &table_record->checksum);
        read32(&reader, &table_record->offset);
        read32(&reader, &table_record->length);

        table_record->table = NULL;
    }

    for (Int table_index = 0; table_index < font.offset_table.table_count; table_index++)
    {
        OtfTableRecord *table_record = &font.table_records[table_index];

        assert(table_record->offset + table_record->length <= (UInt)reader.buffer.length);
        reader.pos = table_record->offset;

        if (tag_equal(table_record->tag, otf_tag_cmap))
        {
            table_record->table = malloc(sizeof(OtfCmapTable));

            OtfCmapTable *cmap_table = (OtfCmapTable *)table_record->table;
            read16(&reader, &cmap_table->version);
            read16(&reader, &cmap_table->encoding_table_count);

            cmap_table->encoding_records = (OtfEncodingRecord *)malloc(sizeof(OtfEncodingRecord) * cmap_table->encoding_table_count);
            for (Int encoding_record_index = 0; encoding_record_index < cmap_table->encoding_table_count; encoding_record_index++)
            {
                OtfEncodingRecord *encoding_record = (OtfEncodingRecord *)&cmap_table->encoding_records[encoding_record_index];
                read16(&reader, &encoding_record->platform_id);
                read16(&reader, &encoding_record->encoding_id);
                read32(&reader, &encoding_record->offset);
            }

            for (Int encoding_record_index = 0; encoding_record_index < cmap_table->encoding_table_count; encoding_record_index++)
            {
                OtfEncodingRecord *encoding_record = (OtfEncodingRecord *)&cmap_table->encoding_records[encoding_record_index];
                reader.pos = table_record->offset + encoding_record->offset;

                UInt16 format;
                read16(&reader, &format);
                if (format == 4)
                {
                    encoding_record->table = (OtfEncodingFormat4 *)malloc(sizeof(OtfEncodingFormat4));
                    OtfEncodingFormat4 *encoding_format = (OtfEncodingFormat4 *)encoding_record->table;

                    encoding_format->format = format;
                    read16(&reader, &encoding_format->length);
                    read16(&reader, &encoding_format->language);
                    read16(&reader, &encoding_format->segment_count_x2);
                    read16(&reader, &encoding_format->search_range);
                    read16(&reader, &encoding_format->entry_selector);
                    read16(&reader, &encoding_format->range_shift);

                    encoding_format->end_code = (UInt16 *)&reader.buffer[reader.pos];
                    reader.pos += encoding_format->segment_count_x2;

                    reader.pos += 2;

                    encoding_format->start_code = (UInt16 *)&reader.buffer[reader.pos];
                    reader.pos += encoding_format->segment_count_x2;

                    encoding_format->id_delta = (Int16 *)&reader.buffer[reader.pos];
                    reader.pos += encoding_format->segment_count_x2;

                    encoding_format->id_range_offset = (UInt16 *)&reader.buffer[reader.pos];
                    reader.pos += encoding_format->segment_count_x2;

                    encoding_format->glyph_id = (UInt16 *)&reader.buffer[reader.pos];
                }
                else
                {
                    encoding_record->table = NULL;
                }
            }
        }
        else if (tag_equal(table_record->tag, otf_tag_cff))
        {
            table_record->table = malloc(sizeof(CffTable));

            CffTable *cff_table = (CffTable *)table_record->table;
            Int cff_table_start = reader.pos;
            read8(&reader, &cff_table->header.major);
            read8(&reader, &cff_table->header.minor);
            read8(&reader, &cff_table->header.header_size);
            read8(&reader, &cff_table->header.offset_size);
            reader.pos += cff_table->header.header_size - 4;

            read_cff_index(&reader, &cff_table->name_index);

            Int top_index_start = reader.pos;
            read_cff_index(&reader, &cff_table->top_index);
            Int top_index_end = reader.pos;

            Int top_index_data_start = top_index_start + get_cff_index_data_start(&cff_table->top_index);
            cff_table->top_index.objects = malloc(sizeof(CffDict) * cff_table->top_index.count);
            for (Int dict_i = 0; dict_i < cff_table->top_index.count; dict_i++)
            {
                Int dict_start = top_index_data_start + get_cff_index_offset(&cff_table->top_index, dict_i);
                Int dict_end = top_index_data_start + get_cff_index_offset(&cff_table->top_index, dict_i + 1);

                reader.pos = dict_start;
                read_cff_dict(&reader, dict_end, (CffDict *)cff_table->top_index.objects + dict_i);
            }
            reader.pos = top_index_end;

            for (Int dict_i = 0; dict_i < cff_table->top_index.count; dict_i++)
            {
                CffDict *dict = (CffDict *)cff_table->top_index.objects + dict_i;

                for (Int entry_i = 0; entry_i < dict->entries.length; entry_i++)
                {
                    CffDictEntry *entry = &dict->entries[entry_i];

                    if (entry->key == 17)
                    {
                        Int char_string_index_start = cff_table_start + entry->value.integer;
                        reader.pos = char_string_index_start;
                        read_cff_index(&reader, &cff_table->char_string_index);

                        Int char_string_index_data_start = char_string_index_start + get_cff_index_data_start(&cff_table->char_string_index);
                        cff_table->char_string_index.objects = malloc(sizeof(CharString) * cff_table->char_string_index.count);
                        for (Int char_string_i = 0; char_string_i < cff_table->char_string_index.count; char_string_i++)
                        {
                            Int char_string_start = char_string_index_data_start + get_cff_index_offset(&cff_table->char_string_index, char_string_i);
                            Int char_string_end = char_string_index_data_start + get_cff_index_offset(&cff_table->char_string_index, char_string_i + 1);

                            CharStringReader char_string_reader;
                            char_string_reader.buffer = copy_str(reader.buffer.data + char_string_start, char_string_end - char_string_start);
                            char_string_reader.pos = 0;

                            CharString *char_string = (CharString *)cff_table->char_string_index.objects + char_string_i;
                            read_char_string(&char_string_reader, char_string);
                        }
                    }
                    else if (entry->key == 18)
                    {
                        assert(entry->value.type == CffDictValueType::array && entry->value.array.length == 2);
                        Int private_dict_size = entry->value.array[0].integer;
                        Int private_dict_offset = entry->value.array[1].integer;

                        Int private_dict_start = cff_table_start + private_dict_offset;
                        Int private_dict_end = private_dict_start + private_dict_size;
                        reader.pos = private_dict_start;
                        read_cff_dict(&reader, private_dict_end, &cff_table->private_dict);

                        for (Int entry_i = 0; entry_i < cff_table->private_dict.entries.length; entry_i++)
                        {
                            CffDictEntry *entry = &cff_table->private_dict.entries[entry_i];
                            if (entry->key == 19)
                            {
                                Int subr_offset = entry->value.integer;
                                Int subr_index_start = private_dict_start + subr_offset;
                                reader.pos = subr_index_start;
                                read_cff_index(&reader, &cff_table->subr_index);

                                Int subr_index_data_start = subr_index_start + get_cff_index_data_start(&cff_table->subr_index);
                                cff_table->subr_index.objects = malloc(sizeof(CharString) * cff_table->subr_index.count);
                                for (Int subr_i = 0; subr_i < cff_table->subr_index.count; subr_i++)
                                {
                                    Int subr_start = subr_index_data_start + get_cff_index_offset(&cff_table->subr_index, subr_i);
                                    Int subr_end = subr_index_data_start + get_cff_index_offset(&cff_table->subr_index, subr_i + 1);

                                    CharStringReader subr_reader;
                                    subr_reader.buffer = copy_str(reader.buffer.data + subr_start, subr_end - subr_start);
                                    subr_reader.pos = 0;

                                    CharString *subr = (CharString *)cff_table->subr_index.objects + subr_i;
                                    read_char_string(&subr_reader, subr);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    OtfCmapTable *cmap_table = NULL;
    CffTable *cff_table = NULL;
    for (Int table_index = 0; table_index < font.offset_table.table_count; table_index++)
    {
        if (tag_equal(font.table_records[table_index].tag, otf_tag_cmap))
        {
            cmap_table = (OtfCmapTable *)font.table_records[table_index].table;
        }
        else if (tag_equal(font.table_records[table_index].tag, otf_tag_cff))
        {
            cff_table = (CffTable *)font.table_records[table_index].table;
        }
    }
    assert(cmap_table && cff_table);

    OtfEncodingFormat4 *encoding_table = NULL;
    for (Int encoding_table_index = 0; encoding_table_index < cmap_table->encoding_table_count; encoding_table_index++)
    {
        if (cmap_table->encoding_records[encoding_table_index].table)
        {
            encoding_table = (OtfEncodingFormat4 *)cmap_table->encoding_records[encoding_table_index].table;
            break;
        }
    }

    if (encoding_table)
    {
        assert(encoding_table->format == 4);
        for (UInt16 character = 0x21; character <= 0x7e; character++)
        {
            Int found_segment_index = -1;
            for (Int segment_index = 0; segment_index < encoding_table->segment_count_x2 / 2; segment_index++)
            {
                UInt16 end_code = to_little_endian16(encoding_table->end_code[segment_index]);

                if (end_code >= character)
                {
                    found_segment_index = segment_index;
                    break;
                }
            }

            assert(found_segment_index != -1);

            UInt16 start_code = to_little_endian16(encoding_table->start_code[found_segment_index]);
            if (start_code <= character)
            {
                Int16 id_delta = to_little_endian16(encoding_table->id_delta[found_segment_index]);
                UInt16 id_range_offset = to_little_endian16(encoding_table->id_range_offset[found_segment_index]);
                Int glyph_id;
                if (id_range_offset)
                {
                    glyph_id = *(id_range_offset / 2 +
                                 (character - start_code) + &id_range_offset);
                }
                else
                {
                    glyph_id = id_delta + character;
                }

                assert(glyph_id < cff_table->char_string_index.count);

                printf("%c - %d\n", character, glyph_id);

                CharString *char_string = (CharString *)cff_table->char_string_index.objects + glyph_id;

                CharStringRunner runner;
                runner.subr_count = cff_table->subr_index.count;
                runner.subr_list = (CharString *)cff_table->subr_index.objects;
                runner.stack_length = 0;
                runner.x = {0};
                runner.y = {0};
                runner.min_x.integer = INT16_MAX;
                runner.max_x.integer = INT16_MIN;
                runner.min_y.integer = INT16_MAX;
                runner.max_y.integer = INT16_MIN;
                runner.subr_depth = 0;
                runner.end = false;

                memset(bitmap, 0, sizeof(bitmap));
                run_char_string(&runner, char_string);

                UInt8 filename_data[8] = {' ', ' ', ' ', '.', 'b', 'm', 'p', '\0'};
                filename_data[0] = character / 100 + '0';
                filename_data[1] = character / 10 % 10 + '0';
                filename_data[2] = character % 10 + '0';
                Str filename;
                filename.length = 7;
                filename.data = filename_data;
                write_bitmap(filename);
            }
            else
            {
                assert(false);
            }
        }
    }
}
