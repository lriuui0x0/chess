#pragma once

#include "../src/util.cpp"

struct CharStringReader
{
    Str buffer;
    Int pos;
};

enum struct CharStringOpType
{
    hstem,
    vstem,
    vmoveto,
    rlineto,
    hlineto,
    vlineto,
    rrcurveto,
    callsubr,
    return_,
    endchar,
    hstemhm,
    hintmask,
    cntrmask,
    rmoveto,
    hmoveto,
    vstemhm,
    rcurveline,
    rlinecurve,
    vvcurveto,
    hhcurveto,
    callgsubr,
    vhcurveto,
    hvcurveto,
};

enum struct CharStringAtomType
{
    op,
    number,
};

struct CharStringFixed
{
    Int16 integer;
    UInt16 fraction;
};

struct CharStringAtom
{
    CharStringAtomType type;
    union {
        CharStringFixed number;
        CharStringOpType op;
    };
};

struct CharString
{
    Array<CharStringAtom> atoms;
};

CharStringFixed operator+(CharStringFixed number, Int16 integer)
{
    number.integer += integer;
    return number;
}

CharStringFixed operator+(CharStringFixed number, CharStringFixed number2)
{
    UInt32 fraction = number.fraction + number2.fraction;
    number.fraction = fraction & 0xffff;
    number.integer += number2.integer + ((fraction >> 16) & 0xffff);
    return number;
}

void read_char_string_byte(CharStringReader *reader, OUT UInt8 *byte)
{
    assert(reader->pos + 1 <= reader->buffer.length);
    *byte = reader->buffer[reader->pos++];
}

void read_char_string(CharStringReader *reader, OUT CharString *char_string)
{
    char_string->atoms = create_array<CharStringAtom>();

    while (reader->pos < reader->buffer.length)
    {
        UInt8 byte0;
        read_char_string_byte(reader, &byte0);

        CharStringAtom *atom = char_string->atoms.push();
        if (byte0 == 0)
        {
            assert(false);
        }
        else if (byte0 == 1)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hstem;
        }
        else if (byte0 == 2)
        {
            assert(false);
        }
        else if (byte0 == 3)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vstem;
        }
        else if (byte0 == 4)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vmoveto;
        }
        else if (byte0 == 5)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::rlineto;
        }
        else if (byte0 == 6)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hlineto;
        }
        else if (byte0 == 7)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vlineto;
        }
        else if (byte0 == 8)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::rrcurveto;
        }
        else if (byte0 == 9)
        {
            assert(false);
        }
        else if (byte0 == 10)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::callsubr;
        }
        else if (byte0 == 11)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::return_;
        }
        else if (byte0 == 12)
        {
            assert(false);
        }
        else if (byte0 == 13)
        {
            assert(false);
        }
        else if (byte0 == 14)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::endchar;
        }
        else if (byte0 == 15)
        {
            assert(false);
        }
        else if (byte0 == 16)
        {
            assert(false);
        }
        else if (byte0 == 17)
        {
            assert(false);
        }
        else if (byte0 == 18)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hstemhm;
        }
        else if (byte0 == 19)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hintmask;
        }
        else if (byte0 == 20)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::cntrmask;
        }
        else if (byte0 == 21)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::rmoveto;
        }
        else if (byte0 == 22)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hmoveto;
        }
        else if (byte0 == 23)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vstemhm;
        }
        else if (byte0 == 24)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::rcurveline;
        }
        else if (byte0 == 25)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::rlinecurve;
        }
        else if (byte0 == 26)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vvcurveto;
        }
        else if (byte0 == 27)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hhcurveto;
        }
        else if (byte0 == 28)
        {
            atom->type = CharStringAtomType::number;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            UInt8 byte2;
            read_char_string_byte(reader, &byte2);

            atom->number.integer = (byte1 << 8) | byte2;
            atom->number.fraction = 0;
            assert(atom->number.integer >= -32768 && atom->number.integer <= 32767);
        }
        else if (byte0 == 29)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::callgsubr;
        }
        else if (byte0 == 30)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::vhcurveto;
        }
        else if (byte0 == 31)
        {
            atom->type = CharStringAtomType::op;
            atom->op = CharStringOpType::hvcurveto;
        }
        else if (byte0 >= 32 && byte0 <= 246)
        {
            atom->type = CharStringAtomType::number;
            atom->number.integer = (Int)byte0 - 139;
            atom->number.fraction = 0;
            assert(atom->number.integer >= -107 && atom->number.integer <= 107);
        }
        else if (byte0 >= 247 && byte0 <= 250)
        {
            atom->type = CharStringAtomType::number;
            atom->number.integer = ((Int)byte0 - 247) * 256;
            atom->number.fraction = 0;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            atom->number.integer += (Int)byte1 + 108;
            assert(atom->number.integer >= 108 && atom->number.integer <= 1131);
        }
        else if (byte0 >= 251 && byte0 <= 254)
        {
            atom->type = CharStringAtomType::number;
            atom->number.integer = -((Int)byte0 - 251) * 256;
            atom->number.fraction = 0;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            atom->number.integer += -(Int)byte1 - 108;
            assert(atom->number.integer >= -1131 && atom->number.integer <= -108);
        }
        else if (byte0 == 255)
        {
            atom->type = CharStringAtomType::number;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            UInt8 byte2;
            read_char_string_byte(reader, &byte2);
            UInt8 byte3;
            read_char_string_byte(reader, &byte3);
            UInt8 byte4;
            read_char_string_byte(reader, &byte4);

            atom->number.integer = (byte1 << 8) | byte2;
            atom->number.fraction = (byte3 << 8) | byte4;
        }
    }
}

struct CharStringRunner
{
    Int subr_count;
    CharString *subr_list;

    CharStringFixed stack[50];
    Int stack_length;

    CharStringFixed x;
    CharStringFixed y;
    CharStringFixed min_x;
    CharStringFixed max_x;
    CharStringFixed min_y;
    CharStringFixed max_y;
    Int subr_depth;
    Bool end;
};

CharString *get_subr(CharStringRunner *runner, Int subr_i)
{
    Int bias;
    if (runner->subr_count < 1240)
    {
        bias = 107;
    }
    else if (runner->subr_count < 33900)
    {
        bias = 1131;
    }
    else
    {
        bias = 32768;
    }

    Int subr_index = subr_i + bias;
    assert(subr_index >= 0 && subr_index < runner->subr_count);

    return runner->subr_list + subr_index;
}

void print_fixed(CharStringFixed number)
{
    printf("%d.%u", number.integer, number.fraction);
}

void print_state(CharStringOpType op, CharStringRunner *runner)
{
    if (op == CharStringOpType::rmoveto || op == CharStringOpType::hmoveto || op == CharStringOpType::vmoveto)
    {
        printf("move  - ");
    }
    else if (op == CharStringOpType::rlineto || op == CharStringOpType::hlineto || op == CharStringOpType::vlineto)
    {
        printf("line  - ");
    }
    else if (op == CharStringOpType::rrcurveto)
    {
        printf("curve - ");
    }
    else
    {
        return;
    }

    printf("x = ");
    print_fixed(runner->x);
    printf("\t\t\t\ty = ");
    print_fixed(runner->y);
    printf("\n");
}

const Int bitmap_size = 2200;
Bool bitmap[bitmap_size][bitmap_size];

void write_bitmap(Str filename)
{
    FILE *file_handle = fopen((RawStr)filename.data, "wb");
    assert(file_handle);

    UInt8 bf_type[2] = {'B', 'M'};
    fwrite(&bf_type, 2, 1, file_handle);
    UInt32 bf_size = 14 + 40 + 4 * bitmap_size * bitmap_size;
    fwrite(&bf_size, 4, 1, file_handle);
    UInt16 bf_reserved1 = 0;
    fwrite(&bf_reserved1, 2, 1, file_handle);
    UInt16 bf_reserved2 = 0;
    fwrite(&bf_reserved2, 2, 1, file_handle);
    UInt32 bf_off_bits = 14 + 40;
    fwrite(&bf_off_bits, 4, 1, file_handle);

    UInt32 bi_size = 40;
    fwrite(&bi_size, 4, 1, file_handle);
    UInt32 bi_width = bitmap_size;
    fwrite(&bi_width, 4, 1, file_handle);
    UInt32 bi_height = bitmap_size;
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

    for (Int y = 0; y < bitmap_size; y++)
    {
        for (Int x = 0; x < bitmap_size; x++)
        {
            UInt32 color = bitmap[y][x] ? 0 : 0xffffffff;
            fwrite(&color, 4, 1, file_handle);
        }
    }

    assert(fclose(file_handle) == 0);
}

void draw_line(CharStringFixed fx0, CharStringFixed fy0, CharStringFixed fx1, CharStringFixed fy1)
{
    Int x0 = fx0.integer;
    Int y0 = fy0.integer;
    Int x1 = fx1.integer;
    Int y1 = fy1.integer;

    Real dx = (x1 - x0) / 1000.0f;
    Real dy = (y1 - y0) / 1000.0f;
    for (Int i = 0; i < 1000; i++)
    {
        Int x = x0 + i * dx;
        Int y = y0 + i * dy;

        assert(x >= -500 && x < 1700);
        assert(y >= -500 && y < 1700);

        bitmap[y + 500][x + 500] = true;
    }
}

void run_char_string(CharStringRunner *runner, CharString *char_string)
{
    for (Int atom_i = 0; atom_i < char_string->atoms.length; atom_i++)
    {
        CharStringAtom *atom = &char_string->atoms[atom_i];

        CharStringFixed dx1;
        CharStringFixed dy1;
        CharStringFixed dx2;
        CharStringFixed dy2;
        CharStringFixed dx3;
        CharStringFixed dy3;

        if (atom->type == CharStringAtomType::number)
        {
            runner->stack[runner->stack_length++] = atom->number;
        }
        else
        {
            assert(atom->type == CharStringAtomType::op);

            if (atom->op == CharStringOpType::rmoveto)
            {
                assert(runner->stack_length == 2);

                runner->x = runner->x + runner->stack[0];
                runner->y = runner->y + runner->stack[1];
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::hmoveto)
            {
                assert(runner->stack_length == 1);

                runner->x = runner->x + runner->stack[0];
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::vmoveto)
            {
                assert(runner->stack_length == 1);

                runner->y = runner->y + runner->stack[0];
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::rlineto)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 2 == 0);

                Int arg_i = 0;
                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = runner->stack[arg_i++];

                    draw_line(runner->x, runner->y, runner->x + dx1, runner->y + dy1);
                    runner->x = runner->x + dx1;
                    runner->y = runner->y + dy1;
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::hlineto ||
                     atom->op == CharStringOpType::vlineto)
            {
                assert(runner->stack_length > 0);

                Int arg_i = 0;
                if (atom->op == CharStringOpType::vlineto)
                {
                    goto start_vlineto;
                }

                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = {};

                    draw_line(runner->x, runner->y, runner->x + dx1, runner->y + dy1);
                    runner->x = runner->x + dx1;
                    runner->y = runner->y + dy1;

                    if (arg_i >= runner->stack_length)
                    {
                        break;
                    }

                start_vlineto:
                    dx1 = {};
                    dy1 = runner->stack[arg_i++];

                    draw_line(runner->x, runner->y, runner->x + dx1, runner->y + dy1);
                    runner->x = runner->x + dx1;
                    runner->y = runner->y + dy1;
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::rrcurveto)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 6 == 0);

                Int arg_i = 0;
                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = runner->stack[arg_i++];
                    dy3 = runner->stack[arg_i++];

                    draw_line(runner->x, runner->y, runner->x + dx1 + dx2 + dx3, runner->y + dy1 + dy2 + dy3);
                    runner->x = runner->x + dx1 + dx2 + dx3;
                    runner->y = runner->y + dy1 + dy2 + dy3;
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::hhcurveto)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 4 == 0 || runner->stack_length % 4 == 1);

                Int arg_i = 0;
                if (runner->stack_length % 2 == 1)
                {
                    dy1 = runner->stack[arg_i++];
                }
                else
                {
                    dy1 = {};
                }

                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = runner->stack[arg_i++];
                    dy3 = {};

                    draw_line(runner->x, runner->y, runner->x + dx1 + dx2 + dx3, runner->y + dy1 + dy2 + dy3);
                    runner->x = runner->x + dx1 + dx2 + dx3;
                    runner->y = runner->y + dy1 + dy2 + dy3;

                    dy1 = {};
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::vvcurveto)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 4 == 0 || runner->stack_length % 4 == 1);

                Int arg_i = 0;
                if (runner->stack_length % 2 == 1)
                {
                    dx1 = runner->stack[arg_i++];
                }
                else
                {
                    dx1 = {};
                }

                while (arg_i < runner->stack_length)
                {
                    dy1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = {};
                    dy3 = runner->stack[arg_i++];

                    draw_line(runner->x, runner->y, runner->x + dx1 + dx2 + dx3, runner->y + dy1 + dy2 + dy3);
                    runner->x = runner->x + dx1 + dx2 + dx3;
                    runner->y = runner->y + dy1 + dy2 + dy3;

                    dx1 = {};
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::hvcurveto ||
                     atom->op == CharStringOpType::vhcurveto)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 8 == 0 || runner->stack_length % 8 == 1 ||
                       runner->stack_length % 8 == 4 || runner->stack_length % 8 == 5);

                Int arg_i = 0;
                if (atom->op == CharStringOpType::vhcurveto)
                {
                    goto start_vhcurveto;
                }

                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = {};
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = {};
                    dy3 = runner->stack[arg_i++];
                    if (arg_i == runner->stack_length - 1)
                    {
                        dx3 = runner->stack[arg_i++];
                    }

                    draw_line(runner->x, runner->y, runner->x + dx1 + dx2 + dx3, runner->y + dy1 + dy2 + dy3);
                    runner->x = runner->x + dx1 + dx2 + dx3;
                    runner->y = runner->y + dy1 + dy2 + dy3;

                    if (arg_i >= runner->stack_length)
                    {
                        break;
                    }

                start_vhcurveto:
                    dx1 = {};
                    dy1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = runner->stack[arg_i++];
                    dy3 = {};
                    if (arg_i == runner->stack_length - 1)
                    {
                        dy3 = runner->stack[arg_i++];
                    }

                    draw_line(runner->x, runner->y, runner->x + dx1 + dx2 + dx3, runner->y + dy1 + dy2 + dy3);
                    runner->x = runner->x + dx1 + dx2 + dx3;
                    runner->y = runner->y + dy1 + dy2 + dy3;
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::callsubr)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack[runner->stack_length - 1].fraction == 0);

                CharString *subr_char_string = get_subr(runner, runner->stack[--runner->stack_length].integer);
                runner->subr_depth++;
                run_char_string(runner, subr_char_string);
            }
            else if (atom->op == CharStringOpType::return_)
            {
                assert(runner->subr_depth > 0);
                runner->subr_depth--;
                return;
            }
            else if (atom->op == CharStringOpType::endchar)
            {
                runner->stack_length = 0;
                runner->end = true;
            }
            else
            {
                assert(false);
            }
        }

        if (runner->end)
        {
            return;
        }
    }
}
