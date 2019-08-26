#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define OUT

typedef int8_t Int8;
typedef int16_t Int16;
typedef int32_t Int32;
typedef int64_t Int64;
typedef Int32 Int;

typedef uint8_t UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;
typedef uint64_t UInt64;
typedef UInt32 UInt;

typedef float Real32;
typedef double Real64;
typedef Real32 Real;

typedef bool Bool;
typedef void Void;

Void __builtin_trap();
Void *__builtin_alloca(Int size);

Void assert(Bool predicate)
{
    if (!predicate)
    {
        __builtin_trap();
    }
}

Bool has_flag(UInt64 value, UInt64 flag)
{
    return (value & flag) == flag;
}

typedef char *RawStr;

struct Str
{
    Int length;
    UInt8 *data;

    UInt8 &operator[](Int index);
};

UInt8 &Str::operator[](Int index)
{
    return this->data[index];
}

constexpr Str wrap_str(const char *raw_str)
{
    if (raw_str != NULL)
    {
        Str result = {};

        result.data = (UInt8 *)raw_str;
        while (*raw_str)
        {
            raw_str++;
            result.length++;
        }

        return result;
    }
    else
    {
        return {};
    }
}

Str copy_str(Void *data, Int length)
{
    assert(data);
    assert(length >= 0);

    UInt8 *str_data = (UInt8 *)malloc(length + 1);
    memcpy(str_data, data, length);
    str_data[length] = '\0';

    Str result;
    result.length = length;
    result.data = str_data;
    return result;
}

Str concat_str(Str a, Str b)
{
    assert(a.data && b.data);

    UInt8 *data = (UInt8 *)malloc(a.length + b.length + 1);
    memcpy(data, a.data, a.length);
    memcpy(data + a.length, b.data, b.length);
    data[a.length + b.length] = '\0';

    Str result;
    result.length = a.length + b.length;
    result.data = data;
    return result;
}

template <typename T>
struct Array
{
    T *data;
    Int length;
    Int capacity;

    T &operator[](int index);
    T *push();
};

template <typename T>
Array<T> create_array(Int initial_size = 16)
{
    Array<T> result;
    result.capacity = initial_size;
    result.data = (T *)malloc(result.capacity * sizeof(T));
    result.length = 0;
    return result;
}

template <typename T>
Void destroy_array(Array<T> array)
{
    free(array.data);
}

template <typename T>
T &Array<T>::operator[](int index)
{
    assert(index < this->length);
    return this->data[index];
}

template <typename T>
T *Array<T>::push()
{
    if (this->length >= this->capacity)
    {
        Int old_capacity = this->capacity;
        T *old_data = this->data;
        this->capacity *= 2;
        this->data = (T *)malloc(this->capacity * sizeof(T));
        memcpy(this->data, old_data, old_capacity * sizeof(T));
    }

    this->length++;
    return &this->data[this->length - 1];
}

Bool read_file(RawStr filename, Str *file)
{
    FILE *file_handle = fopen(filename, "rb");
    if (!file_handle)
    {
        return false;
    }

    Int result = fseek(file_handle, 0, SEEK_END);
    assert(result == 0);
    Int length = ftell(file_handle);
    assert(length >= 0);
    rewind(file_handle);

    UInt8 *data = (UInt8 *)malloc(length + 1);
    Int read_length = fread(data, 1, length, file_handle);
    assert(read_length == length);
    data[length] = 0;

    file->data = data;
    file->length = length;
    return true;
}
