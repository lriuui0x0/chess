#pragma once

#include <cstdint>

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
typedef Void *Handle;

#define null (0)

#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define ABS(x) ((x) > 0 ? (x) : (-(x)))
#define HAS_FLAG(number, flag) ((number) & (flag))
#define SET_FLAG(number, flag) ((number) | (flag))

Void __builtin_trap();
Void *__builtin_alloca(size_t count);

#define ASSERT(predicate) \
    if (!(predicate))     \
    {                     \
        __builtin_trap(); \
    }

#define ALLOCA(count) \
    __builtin_alloca(count)

template <typename T>
Void swap(T *x, T *y)
{
    T tmp = *x;
    *x = *y;
    *y = tmp;
}

UInt align_up(UInt x, UInt mask);

template <typename T>
struct Buffer
{
    Int count;
    T *data;

    T &operator[](Int index);
};

template <typename T>
T &Buffer<T>::operator[](Int index)
{
    ASSERT(index >= 0 && index < this->count);
    return this->data[index];
}

template <typename T, Int N>
struct InlineBuffer
{
    Int count;
    T data[N];

    T &operator[](Int index);
};

template <typename T, Int N>
T &InlineBuffer<T, N>::operator[](Int index)
{
    ASSERT(index >= 0 && index < N);
    return this->data[index];
}

typedef char *CStr;
typedef Buffer<UInt8> Str;

constexpr Str str(char const *c_str);
Str copy_str(Void *data, Int count);
Str concat_str(Str a, Str b);

template <typename T>
struct Array
{
    T *data;
    Int count;
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
    result.count = 0;
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
    ASSERT(index < this->count);
    return this->data[index];
}

template <typename T>
T *Array<T>::push()
{
    if (this->count >= this->capacity)
    {
        Int old_capacity = this->capacity;
        T *old_data = this->data;
        this->capacity *= 2;
        this->data = (T *)malloc(this->capacity * sizeof(T));
        memcpy(this->data, old_data, old_capacity * sizeof(T));
    }

    this->count++;
    return &this->data[this->count - 1];
}

Bool read_file(CStr filename, Str *contents);
Bool write_file(CStr filename, Str contents);
