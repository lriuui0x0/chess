#include "util.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>

UInt align_up(UInt x, UInt mask)
{
    UInt result = x + mask - ((x - 1) & (mask - 1)) - 1;
    return result;
}

constexpr Str str(char const *c_str)
{
    if (c_str != NULL)
    {
        Str result = {};

        result.data = (UInt8 *)c_str;
        while (*c_str)
        {
            c_str++;
            result.count++;
        }

        return result;
    }
    else
    {
        return {};
    }
}

Str copy_str(Void *data, Int count)
{
    ASSERT(data);
    ASSERT(count >= 0);

    UInt8 *str_data = (UInt8 *)malloc(count + 1);
    memcpy(str_data, data, count);
    str_data[count] = '\0';

    Str result;
    result.count = count;
    result.data = str_data;
    return result;
}

Str concat_str(Str a, Str b)
{
    ASSERT(a.data && b.data);

    UInt8 *data = (UInt8 *)malloc(a.count + b.count + 1);
    memcpy(data, a.data, a.count);
    memcpy(data + a.count, b.data, b.count);
    data[a.count + b.count] = '\0';

    Str result;
    result.count = a.count + b.count;
    result.data = data;
    return result;
}

Bool read_file(CStr filename, Str *contents)
{
    FILE *file_handle = fopen(filename, "rb");
    if (!file_handle)
    {
        return false;
    }

    Int result = fseek(file_handle, 0, SEEK_END);
    ASSERT(result == 0);
    Int count = ftell(file_handle);
    ASSERT(count >= 0);
    rewind(file_handle);

    UInt8 *data = (UInt8 *)malloc(count + 1);
    Int read_count = fread(data, 1, count, file_handle);
    ASSERT(read_count == count);
    data[count] = 0;

    ASSERT(fclose(file_handle) == 0);

    contents->data = data;
    contents->count = count;
    return true;
}

Bool write_file(CStr filename, Str contents)
{
    FILE *file_handle = fopen(filename, "wb");
    if (!file_handle)
    {
        return false;
    }

    Int write_length = fwrite(contents.data, 1, contents.count, file_handle);
    ASSERT(write_length == contents.count);

    ASSERT(fclose(file_handle) == 0);

    return true;
}

UInt64 get_random_number(RandomGenerator *generator)
{
    UInt64 next_seed = 6364136223846793005ull * generator->seed + 1442695040888963407ull;
    generator->seed = next_seed;
    return next_seed;
}

UInt64 get_random_number(RandomGenerator *generator, UInt64 min, UInt64 max)
{
    ASSERT(min < max);
    UInt64 random_number = get_random_number(generator);
    UInt64 result = random_number % (max - min) + min;
    return result;
}
