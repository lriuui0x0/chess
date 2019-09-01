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
        Real number;
        CharStringOpType op;
    };
};

struct CharString
{
    Array<CharStringAtom> atoms;
};

Real min(Real a, Real b)
{
    return a < b ? a : b;
}

Real max(Real a, Real b)
{
    return a > b ? a : b;
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

            atom->number = (Int16)(byte1 << 8 | byte2);
            assert(atom->number >= -32768 && atom->number <= 32767);
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
            atom->number = (Int)byte0 - 139;
            assert(atom->number >= -107 && atom->number <= 107);
        }
        else if (byte0 >= 247 && byte0 <= 250)
        {
            atom->type = CharStringAtomType::number;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);

            atom->number = ((Int)byte0 - 247) * 256 + (Int)byte1 + 108;
            assert(atom->number >= 108 && atom->number <= 1131);
        }
        else if (byte0 >= 251 && byte0 <= 254)
        {
            atom->type = CharStringAtomType::number;

            UInt8 byte1;
            read_char_string_byte(reader, &byte1);

            atom->number = -((Int)byte0 - 251) * 256 - (Int)byte1 - 108;
            assert(atom->number >= -1131 && atom->number <= -108);
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

            atom->number = (Int)((byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4) / (Real)0x10000;
        }
    }
}

struct Line
{
    Real x0;
    Real y0;
    Real x1;
    Real y1;
};

struct Path
{
    Array<Line> lines;
};

struct CharStringRunner
{
    Int subr_count;
    CharString *subr_list;

    Real stack[50];
    Int stack_length;

    Real x;
    Real y;
    Real min_x;
    Real max_x;
    Real min_y;
    Real max_y;
    Real first_x;
    Real first_y;
    Int subr_depth;

    Bool started;
    Bool ended;

    Array<Path> paths;
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

void move(CharStringRunner *runner, Real x, Real y)
{
    runner->x = x;
    runner->y = y;
}

void refine_bbox(CharStringRunner *runner, Real x, Real y)
{
    runner->min_x = min(runner->min_x, x);
    runner->max_x = max(runner->max_x, x);
    runner->min_y = min(runner->min_y, y);
    runner->max_y = max(runner->max_y, y);
}

void add_line(CharStringRunner *runner, Real x0, Real y0, Real x1, Real y1)
{
    Line *line = runner->paths[runner->paths.length - 1].lines.push();
    line->x0 = x0;
    line->y0 = y0;
    line->x1 = x1;
    line->y1 = y1;
}

void close_path(CharStringRunner *runner)
{
    if (runner->started)
    {
        if (runner->x != runner->first_x || runner->y != runner->first_y)
        {
            add_line(runner, runner->x, runner->y, runner->first_x, runner->first_y);
        }
    }
}

void add_cubic_curve(CharStringRunner *runner, Real x0, Real y0, Real x1, Real y1, Real x2, Real y2, Real x3, Real y3)
{
    Real mx0 = (x0 + x1) / 2;
    Real my0 = (y0 + y1) / 2;
    Real mx1 = (x1 + x2) / 2;
    Real my1 = (y1 + y2) / 2;
    Real mx2 = (x2 + x3) / 2;
    Real my2 = (y2 + y3) / 2;

    Real mmx0 = (mx0 + mx1) / 2;
    Real mmy0 = (my0 + my1) / 2;
    Real mmx1 = (mx1 + mx2) / 2;
    Real mmy1 = (my1 + my2) / 2;

    Real mmmx0 = (mmx0 + mmx1) / 2;
    Real mmmy0 = (mmy0 + mmy1) / 2;

    Real one_dist = hypot(x3 - x0, y3 - y0);
    Real three_dist = hypot(x1 - x0, y1 - y0) + hypot(x2 - x1, y2 - y1) + hypot(x3 - x2, y3 - y2);
    Real flatness = three_dist - one_dist;

    if (flatness < 0.1)
    {
        add_line(runner, x0, y0, x3, y3);
    }
    else
    {
        add_cubic_curve(runner, x0, y0, mx0, my0, mmx0, mmy0, mmmx0, mmmy0);
        add_cubic_curve(runner, mmmx0, mmmy0, mmx1, mmy1, mx2, my2, x3, y3);
    }
}

void run_char_string(CharStringRunner *runner, CharString *char_string)
{
    for (Int atom_i = 0; atom_i < char_string->atoms.length; atom_i++)
    {
        if (runner->ended)
        {
            return;
        }

        CharStringAtom *atom = &char_string->atoms[atom_i];

        Real dx1;
        Real dy1;
        Real dx2;
        Real dy2;
        Real dx3;
        Real dy3;

        if (atom->type == CharStringAtomType::number)
        {
            runner->stack[runner->stack_length++] = atom->number;
        }
        else
        {
            assert(atom->type == CharStringAtomType::op);

            switch (atom->op)
            {
            case CharStringOpType::rmoveto:
            {
                assert(runner->stack_length == 2);

                close_path(runner);
                runner->paths.push()->lines = create_array<Line>();

                dx1 = runner->stack[0];
                dy1 = runner->stack[1];
                Real x1 = runner->x + dx1;
                Real y1 = runner->y + dy1;
                refine_bbox(runner, x1, y1);
                move(runner, x1, y1);

                runner->started = true;
                runner->first_x = runner->x;
                runner->first_y = runner->y;

                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::hmoveto:
            {
                assert(runner->stack_length == 1);

                close_path(runner);
                runner->paths.push()->lines = create_array<Line>();

                dx1 = runner->stack[0];
                Real x1 = runner->x + dx1;
                refine_bbox(runner, x1, runner->y);
                move(runner, x1, runner->y);

                runner->started = true;
                runner->first_x = runner->x;
                runner->first_y = runner->y;

                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::vmoveto:
            {
                assert(runner->stack_length == 1);

                close_path(runner);
                runner->paths.push()->lines = create_array<Line>();

                dy1 = runner->stack[0];
                Real y1 = runner->y + dy1;
                refine_bbox(runner, runner->x, y1);
                move(runner, runner->x, y1);

                runner->started = true;
                runner->first_x = runner->x;
                runner->first_y = runner->y;

                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::rlineto:
            {
                assert(runner->stack_length > 0);
                assert(runner->stack_length % 2 == 0);

                Int arg_i = 0;
                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = runner->stack[arg_i++];
                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);
                    add_line(runner, runner->x, runner->y, x1, y1);
                    move(runner, x1, y1);
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::hlineto:
            case CharStringOpType::vlineto:
            {
                assert(runner->stack_length > 0);

                Int arg_i = 0;
                if (atom->op == CharStringOpType::vlineto)
                {
                    goto start_vlineto;
                }

                while (arg_i < runner->stack_length)
                {
                start_hlineto:
                {
                    dx1 = runner->stack[arg_i++];
                    Real x1 = runner->x + dx1;
                    refine_bbox(runner, x1, runner->y);
                    add_line(runner, runner->x, runner->y, x1, runner->y);
                    move(runner, x1, runner->y);
                }

                    if (arg_i >= runner->stack_length)
                    {
                        break;
                    }

                start_vlineto:
                {
                    dy1 = runner->stack[arg_i++];
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, runner->x, y1);
                    add_line(runner, runner->x, runner->y, runner->x, y1);
                    move(runner, runner->x, y1);
                }
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::rrcurveto:
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

                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);

                    Real x2 = x1 + dx2;
                    Real y2 = y1 + dy2;
                    refine_bbox(runner, x2, y2);

                    Real x3 = x2 + dx3;
                    Real y3 = y2 + dy3;
                    refine_bbox(runner, x3, y3);

                    add_cubic_curve(runner, runner->x, runner->y, x1, y1, x2, y2, x3, y3);
                    move(runner, x3, y3);
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::hhcurveto:
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
                    dy1 = 0;
                }

                while (arg_i < runner->stack_length)
                {
                    dx1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = runner->stack[arg_i++];
                    dy3 = 0;

                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);

                    Real x2 = x1 + dx2;
                    Real y2 = y1 + dy2;
                    refine_bbox(runner, x2, y2);

                    Real x3 = x2 + dx3;
                    Real y3 = y2 + dy3;
                    refine_bbox(runner, x3, y3);

                    add_cubic_curve(runner, runner->x, runner->y, x1, y1, x2, y2, x3, y3);
                    move(runner, x3, y3);

                    dy1 = 0;
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::vvcurveto:
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
                    dx1 = 0;
                }

                while (arg_i < runner->stack_length)
                {
                    dy1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = 0;
                    dy3 = runner->stack[arg_i++];

                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);

                    Real x2 = x1 + dx2;
                    Real y2 = y1 + dy2;
                    refine_bbox(runner, x2, y2);

                    Real x3 = x2 + dx3;
                    Real y3 = y2 + dy3;
                    refine_bbox(runner, x3, y3);

                    add_cubic_curve(runner, runner->x, runner->y, x1, y1, x2, y2, x3, y3);
                    move(runner, x3, y3);

                    dx1 = 0;
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::hvcurveto:
            case CharStringOpType::vhcurveto:
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
                start_hvcurveto:
                {
                    dx1 = runner->stack[arg_i++];
                    dy1 = 0;
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = 0;
                    dy3 = runner->stack[arg_i++];
                    if (arg_i == runner->stack_length - 1)
                    {
                        dx3 = runner->stack[arg_i++];
                    }

                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);

                    Real x2 = x1 + dx2;
                    Real y2 = y1 + dy2;
                    refine_bbox(runner, x2, y2);

                    Real x3 = x2 + dx3;
                    Real y3 = y2 + dy3;
                    refine_bbox(runner, x3, y3);

                    add_cubic_curve(runner, runner->x, runner->y, x1, y1, x2, y2, x3, y3);
                    move(runner, x3, y3);
                }

                    if (arg_i >= runner->stack_length)
                    {
                        break;
                    }

                start_vhcurveto:
                {
                    dx1 = 0;
                    dy1 = runner->stack[arg_i++];
                    dx2 = runner->stack[arg_i++];
                    dy2 = runner->stack[arg_i++];
                    dx3 = runner->stack[arg_i++];
                    dy3 = 0;
                    if (arg_i == runner->stack_length - 1)
                    {
                        dy3 = runner->stack[arg_i++];
                    }

                    Real x1 = runner->x + dx1;
                    Real y1 = runner->y + dy1;
                    refine_bbox(runner, x1, y1);

                    Real x2 = x1 + dx2;
                    Real y2 = y1 + dy2;
                    refine_bbox(runner, x2, y2);

                    Real x3 = x2 + dx3;
                    Real y3 = y2 + dy3;
                    refine_bbox(runner, x3, y3);

                    add_cubic_curve(runner, runner->x, runner->y, x1, y1, x2, y2, x3, y3);
                    move(runner, x3, y3);
                }
                }
                runner->stack_length = 0;
            }
            break;

            case CharStringOpType::callsubr:
            {
                assert(runner->stack_length > 0);

                CharString *subr_char_string = get_subr(runner, (Int)runner->stack[--runner->stack_length]);
                runner->subr_depth++;
                run_char_string(runner, subr_char_string);
            }
            break;

            case CharStringOpType::return_:
            {
                assert(runner->subr_depth > 0);
                runner->subr_depth--;
                return;
            }
            break;

            case CharStringOpType::endchar:
            {
                close_path(runner);
                runner->stack_length = 0;
                runner->ended = true;
            }
            break;

            default:
            {
                assert(false);
            }
            break;
            }
        }
    }
}
