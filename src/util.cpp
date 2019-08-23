#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define OUT

typedef int8_t Int1;
typedef int16_t Int2;
typedef int32_t Int4;
typedef int64_t Int8;
typedef Int4 Int;

typedef uint8_t UInt1;
typedef uint16_t UInt2;
typedef uint32_t UInt4;
typedef uint64_t UInt8;
typedef UInt4 UInt;

typedef float Real4;
typedef double Real8;
typedef Real4 Real;

typedef bool Bool;

void __builtin_trap();
void *__builtin_alloca(Int size);

void assert(Bool predicate)
{
    if (!predicate)
    {
        __builtin_trap();
    }
}

template <typename T, typename S>
Bool has_flag(T value, S flag)
{
    return (value & flag) == flag;
}

typedef char *RawStr;

struct Str
{
    Int length;
    Int1 *data;
};

constexpr Str wrap_str(const char *raw_str)
{
    if (raw_str != NULL)
    {
        Str result = {};

        result.data = (Int1 *)raw_str;
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

Str concat_str(Str a, Str b)
{
    assert(a.data && b.data);

    Int1 *data = (Int1 *)malloc(a.length + b.length + 1);
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
void destroy_array(Array<T> array)
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

    Int1 *data = (Int1 *)malloc(length + 1);
    Int read_length = fread(data, 1, length, file_handle);
    assert(read_length == length);
    data[length] = 0;

    file->data = data;
    file->length = length;
    return true;
}
