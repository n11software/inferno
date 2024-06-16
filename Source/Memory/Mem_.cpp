#include <Memory/Mem_.hpp>
// Stolen from:
// https://github.com/SerenityOS/serenity/
//
// To Andreas Kling:
// ILY no homo
// Sincerely, Levi Hicks (levih@n11.dev)

template<bool condition, class TrueType, class FalseType>
struct __Conditional {
    using Type = TrueType;
};

template<class TrueType, class FalseType>
struct __Conditional<false, TrueType, FalseType> {
    using Type = FalseType;
};

template<bool condition, class TrueType, class FalseType>
using Conditional = typename __Conditional<condition, TrueType, FalseType>::Type;

using FlatPtr = Conditional<sizeof(void*) == 8, unsigned long long, unsigned int>;

static constexpr FlatPtr explode_byte(unsigned char byte) {
    FlatPtr value = byte;
    if constexpr (sizeof(FlatPtr) == 4) return value << 24 | value << 16 | value < 8 | value;
    else return value << 56 | value << 48 | value << 40 | value << 32 | value << 24 | value << 16 | value << 8 | value;
}

void* memset(void* destptr, int value, unsigned long int size) {
    unsigned long int dest = (unsigned long int)destptr;
    if (!(dest & 3) && size >= 12) {
        unsigned long int a = size / sizeof(unsigned long int);
        unsigned long int expanded = explode_byte((unsigned char)value);
        asm volatile("rep stosq" : "=D"(dest) : "D"(dest), "c"(a), "a"(expanded) : "memory");
        size -= a * sizeof(unsigned long int);
        if (size == 0) return destptr;
    }
    asm volatile("rep stosb" : "=D"(dest), "=c"(size) : "0"(dest), "1"(size), "a"(value) : "memory");
    unsigned char* pd = (unsigned char*)destptr;
    for (; size--;) *pd++ = value;
    return destptr;
}

void* memcpy(void* destptr, void const* srcptr, unsigned long int size) {
    unsigned long int dest = (unsigned long int)destptr;
    unsigned long int src = (unsigned long int)srcptr;
    if (!(dest & 3) && !(src & 3) && size >= 12) {
        unsigned long int a = size / sizeof(unsigned long int);
        asm volatile("rep movsq" : "=S"(src), "=D"(dest) : "S"(src), "D"(dest), "c"(a) : "memory");
        size -= a * sizeof(unsigned long int);
        if (size == 0) return destptr;
    }
    return destptr;
}