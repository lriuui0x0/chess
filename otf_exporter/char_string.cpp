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
    Int16 fraction;
};

struct CharStringAtom
{
    CharStringAtomType type;
    union {
        CharStringFixed fixed;
        CharStringOpType op;
    };
};

struct CharString
{
    Array<CharStringAtom> atoms;
};

CharStringFixed operator+(CharStringFixed fixed, Int16 integer)
{
    fixed.integer += integer;
    return fixed;
}

CharStringFixed operator+(CharStringFixed fixed, CharStringFixed fixed2)
{
    Int32 fraction = (Int16)fixed.fraction + (Int16)fixed2.fraction;
    fixed.fraction = fraction & 0xffff;
    fixed.integer += fixed2.integer + ((fraction >> 16) & 0xffff);
    return fixed;
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

            atom->fixed.integer = (byte1 << 8) | byte2;
            atom->fixed.fraction = 0;
            assert(atom->fixed.integer >= -32768 && atom->fixed.integer <= 32767);
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
            atom->fixed.integer = (Int)byte0 - 139;
            atom->fixed.fraction = 0;
            assert(atom->fixed.integer >= -107 && atom->fixed.integer <= 107);
        }
        else if (byte0 >= 247 && byte0 <= 250)
        {
            atom->type = CharStringAtomType::number;
            atom->fixed.integer = ((Int)byte0 - 247) * 256;
            atom->fixed.fraction = 0;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            atom->fixed.integer += (Int)byte1 + 108;
            assert(atom->fixed.integer >= 108 && atom->fixed.integer <= 1131);
        }
        else if (byte0 >= 251 && byte0 <= 254)
        {
            atom->type = CharStringAtomType::number;
            atom->fixed.integer = -((Int)byte0 - 251) * 256;
            atom->fixed.fraction = 0;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);
            atom->fixed.integer += -(Int)byte1 - 108;
            assert(atom->fixed.integer >= -1131 && atom->fixed.integer <= -108);
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

            atom->fixed.integer = byte1 << 8;
            atom->fixed.integer += byte2;
            atom->fixed.fraction = byte3 << 8;
            atom->fixed.fraction += byte4;
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

void print_fixed(CharStringFixed fixed)
{
    printf("%d.%016u", fixed.integer, fixed.fraction);
}

void print_state(CharStringOpType op, CharStringRunner * runner)
{
    if (op == CharStringOpType::rmoveto || op == CharStringOpType::hmoveto || op == CharStringOpType::vmoveto)
    {
        printf("move - ");
    }
    else if (op == CharStringOpType::rlineto || op == CharStringOpType::hlineto || op == CharStringOpType::vlineto || op == CharStringOpType::rrcurveto)
    {
        printf("draw - ");
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

void run_char_string(CharStringRunner *runner, CharString *char_string)
{
    for (Int atom_i = 0; atom_i < char_string->atoms.length; atom_i++)
    {
        CharStringAtom *atom = &char_string->atoms[atom_i];

        if (atom->type == CharStringAtomType::number)
        {
            runner->stack[runner->stack_length++] = atom->fixed;
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
                assert(runner->stack_length > 0 && runner->stack_length % 2 == 0);

                for (Int arg_i = 0; arg_i < runner->stack_length; arg_i += 2)
                {
                    runner->x = runner->x + runner->stack[arg_i];
                    runner->y = runner->y + runner->stack[arg_i + 1];
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::hlineto)
            {
                assert(runner->stack_length > 0);

                if (runner->stack_length % 2 == 1)
                {
                    runner->x = runner->x + runner->stack[0];

                    for (Int arg_i = 1; arg_i < runner->stack_length; arg_i += 2)
                    {
                        runner->y = runner->y + runner->stack[arg_i];
                        runner->x = runner->x + runner->stack[arg_i + 1];
                    }
                }
                else
                {
                    for (Int arg_i = 0; arg_i < runner->stack_length; arg_i += 2)
                    {
                        runner->x = runner->x + runner->stack[arg_i];
                        runner->y = runner->y + runner->stack[arg_i + 1];
                    }
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::vlineto)
            {
                assert(runner->stack_length > 0);

                if (runner->stack_length % 2 == 1)
                {
                    runner->y = runner->y + runner->stack[0];

                    for (Int arg_i = 1; arg_i < runner->stack_length; arg_i += 2)
                    {
                        runner->x = runner->x + runner->stack[arg_i];
                        runner->y = runner->y + runner->stack[arg_i + 1];
                    }
                }
                else
                {
                    for (Int arg_i = 0; arg_i < runner->stack_length; arg_i += 2)
                    {
                        runner->y = runner->y + runner->stack[arg_i];
                        runner->x = runner->x + runner->stack[arg_i + 1];
                    }
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::rrcurveto)
            {
                assert(runner->stack_length % 6 == 0);

                for (Int arg_i = 0; arg_i < runner->stack_length; arg_i += 6)
                {
                    CharStringFixed dx1 = runner->stack[arg_i];
                    CharStringFixed dy1 = runner->stack[arg_i + 1];
                    CharStringFixed dx2 = runner->stack[arg_i + 2];
                    CharStringFixed dy2 = runner->stack[arg_i + 3];
                    CharStringFixed dx3 = runner->stack[arg_i + 4];
                    CharStringFixed dy3 = runner->stack[arg_i + 5];

                    runner->x = runner->x + dx3;
                    runner->y = runner->y + dy3;
                }
                runner->stack_length = 0;
            }
            else if (atom->op == CharStringOpType::callsubr)
            {
                assert(runner->stack_length > 0);
                assert(runner->stack[runner->stack_length - 1].fraction == 0);

                CharString *subr_char_string = get_subr(runner, runner->stack[runner->stack_length - 1].integer);
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

            print_state(atom->op, runner);
        }

        if (runner->end)
        {
            return;
        }
    }
}
