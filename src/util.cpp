#include <cstdint>
#include <cstdlib>
#include <cstdio>

#define OUT

typedef int8_t Int1;
typedef int16_t Int2;
typedef int32_t Int4;
typedef int64_t Int8;
typedef Int8 Int;
typedef Int1 Byte;

typedef uint8_t UInt1;
typedef uint16_t UInt2;
typedef uint32_t UInt4;
typedef uint64_t UInt8;
typedef UInt8 UInt;

typedef float Real4;
typedef double Real8;
typedef Real8 Real;

typedef bool Bool;

void __builtin_trap();
void assert(Bool predicate)
{
    if (!predicate)
    {
        __builtin_trap();
    }
}

typedef char *RawStr;

struct Str
{
    Int length;
    Byte *data;
};

constexpr Str wrap_str(const char *raw_str)
{
    if (raw_str != NULL)
    {
        Str result = {0};

        result.data = (Byte *)raw_str;
        while (*raw_str)
        {
            raw_str++;
            result.length++;
        }

        return result;
    }
    else
    {
        return {0};
    }
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

Bool read_file(Str filename, Str *file)
{
    FILE *file_handle = fopen((RawStr)filename.data, "rb");
    if (!file_handle)
    {
        return false;
    }

    Int result = fseek(file_handle, 0, SEEK_END);
    assert(result == 0);
    Int length = ftell(file_handle);
    assert(length >= 0);
    rewind(file_handle);

    Byte *data = (Byte *)malloc(length + 1);
    Int read_length = fread(data, 1, length, file_handle);
    assert(read_length == length);
    data[length] = 0;

    file->data = data;
    file->length = length;
    return true;
}
