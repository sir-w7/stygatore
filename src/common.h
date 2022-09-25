#ifndef __LAYER_H_
#define __LAYER_H_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#if defined(__clang__)
# define STYX_COMPILER_CLANG 1

# if defined(_WIN32)
#  define STYX_OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define STYX_OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define STYX_OS_MAC 1
# else
#  error "Unknown OS."
# endif

# if defined(__amd64__)
#  define STYX_ARCH_X64 1
# elif defined(__i386__)
#  define STYX_ARCH_X86 1
# elif defined(__arm__) || defined(__arm64__)
#  define STYX_ARCH_ARM 1
# else
#  error "Unknown arch."
# endif

#elif defined(__GNUC__)
# define STYX_COMPILER_GCC 1

# if defined(_WIN32)
#  define STYX_OS_WINDOWS 1
# elif defined(__gnu_linux__)
#  define STYX_OS_LINUX 1
# elif defined(__APPLE__) && defined(__MACH__)
#  define STYX_OS_MAC 1
# else
#  error "Unknown OS."
# endif

# if defined(__amd64__)
#  define STYX_ARCH_X64 1
# elif defined(__i386__)
#  define STYX_ARCH_X86 1
# elif defined(__arm__)
#  define STYX_ARCH_ARM
# else
#  error "Unknown arch."
# endif

#elif defined(_MSC_VER)
# define STYX_COMPILER_MSVC 1

# if defined(_WIN32)
#  define STYX_OS_WINDOWS 1
# else
#  error "Unknown OS."
# endif

# if defined(_M_AMD64)
#  define STYX_ARCH_X64 1
# elif defined(_M_I86)
#  define STYX_ARCH_X86 1
# elif defined(_M_AMD64)
#  define STYX_ARCH_ARM
# else
#  error "Unknown arch."
# endif

#else
# error "Unknown compiler."
#endif

#if defined(__cplusplus)
# define STYX_LANG_CPP 1
#else
# define STYX_LANG_C 1
#endif

#if !defined(STYX_COMPILER_MSVC)
# define STYX_COMPILER_MSVC 0
#endif

#if !defined(STYX_COMPILER_GCC)
# define STYX_COMPILER_GCC 0
#endif

#if !defined(STYX_COMPILER_CLANG)
# define STYX_COMPILER_CLANG 0
#endif

#if !defined(STYX_OS_WINDOWS)
# define STYX_OS_WINDOWS 0
#endif

#if !defined(STYX_OS_LINUX)
# define STYX_OS_LINUX 0
#endif

#if !defined(STYX_OS_MAC)
# define STYX_OS_MAC 0
#endif

#if !defined(STYX_ARCH_X64)
# define STYX_ARCH_X64 0
#endif

#if !defined(STYX_ARCH_X86)
# define STYX_ARCH_X86 0
#endif

#if !defined(STYX_ARCH_ARM)
# define STYX_ARCH_ARM 0
#endif

#if !defined(STYX_RELEASE)
# define STYX_RELEASE 0
#endif

#if !defined(STYX_DEBUG)
# define STYX_DEBUG 0
#endif 

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  f32;
typedef double f64;

typedef u8  b8;
typedef u16 b16;
typedef u32 b32;
typedef u64 b64;

#define FALSE 0
#define TRUE  1

#define styx_function static
#define styx_persist  static
#define styx_global   static

#define concat_internal(x, y) x##y
#define concat(x, y) concat_internal(x, y)

#if STYX_COMPILER_MSVC
# define styx_inline inline
#else
// TODO(sir->w7): Look into this, along with function attributes to find how to get GCC and Clang to inline functions.
# define styx_inline static inline
#endif

#if STYX_COMPILER_CLANG
# define STYX_FUNCTION_NAME __PRETTY_FUNCTION__
#else
# define STYX_FUNCTION_NAME __func__
#endif

// NOTE(sir->w7): Stolen from Jon Blow.
template <typename T>
struct ExitScope {
    T lambda;
    ExitScope(T lambda) : lambda(lambda) {}
    ~ExitScope() { lambda(); }
    //ExitScope(const ExitScope&);
    
private:
    ExitScope& operator=(const ExitScope&);
};

struct ExitScopeHelp {
    template <typename T>
    ExitScope<T> operator+(T t){ return t; }
};

#define defer const auto& concat(defer__, __COUNTER__) = ExitScopeHelp() + [&]()

#define kilobytes(count) (1024ull * (u64)count)
#define megabytes(count) (1024ull * (u64)kilobytes(count))
#define gigabytes(count) (1024ull * (u64)megabytes(count))

#define cast(variable_identifier, type) ((type)variable_identifier)

#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))

#define _println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define println(fmt, ...) _println(fmt, ##__VA_ARGS__)

#define _fprintln(stream, fmt, ...) fprintf(stream, fmt "\n", ##__VA_ARGS__)
#define fprintln(stream, fmt, ...) _fprintln(stream, fmt, ##__VA_ARGS__)

#define printnl() printf("\n")

#define debugln_str8var(var) println(#var ": " str8_fmt, str8_exp(var))

//--------------------------------------------------------------------
//--------------------------Basic Utilities---------------------------
//--------------------------------------------------------------------

#ifndef max
#define max(a, b) (a > b ? a : b)
#endif

#ifndef min
#define min(a, b) (a > b ? b : a)
#endif 

#define defer_block(start, end)                                         \
    for (int _i_##__LINE__ = ((start), 0);  _i_##__LINE__ == 0; _i_##__LINE__ += 1, (end))

//-------------------------------Memory-------------------------------
#define DEF_ALIGN (2 * sizeof(void *))

uintptr_t align_forth(uintptr_t ptr, u32 align);

void memory_copy(void* dest, void* src, u64 size);
void memory_set(void* ptr, u32 value, u64 size);

// NOTE(sir->w7): prev_offset isn't really used in this codebase since we never really have to resize memory or anything.
struct MemoryArena
{
    u8 *mem;
    u64 size;
    
    u64 offset;

    MemoryArena() {}
    MemoryArena(u64 size);
    MemoryArena(MemoryArena *parent_arena, u64 size);
    
    ~MemoryArena();

    void reset();
    void *push_align(u64 size, u32 align);
    void *push_initialize_align(u64 size, void *init_data, u32 align);
    void *push_pack(u64 size);

    void *push(u64 size) { return push_align(size, DEF_ALIGN); }
    void *push_initialize(u64 size, void *init_data) { return push_initialize_align(size, init_data, DEF_ALIGN); }

    void *push_array(u64 size, u64 count) { return push(size * count); }

private:
    inline void *push_ptr(u64 size, u32 align);
};

struct TempArena
{ 
    MemoryArena *parent_arena;
    u64 offset;

    TempArena(MemoryArena *parent) :
        parent_arena(parent), offset(parent->offset) {}
    ~TempArena() { parent_arena->offset = offset; }
};

//-------------------------------String-------------------------------
struct Str8
{
    char *str;
    u64 len;

    Str8() : str(nullptr), len(0) {}
    Str8(char *cstr, u64 len) : str(cstr), len(len) {}
    Str8(MemoryArena *allocator, char *cstr, u64 len);
    Str8(MemoryArena *allocator, Str8 str);
};

u64 cstr_len(char *cstr);

// make this a method. 
Str8 push_str8_concat(MemoryArena *allocator, Str8 init, Str8 add);

// Returns true if the strings are the same, false if they are different.
b32 str8_compare(Str8 str1, Str8 str2);

struct Str8Node
{
    Str8 data;
    Str8Node *next = nullptr;
};

struct Str8List
{
    Str8Node *head = nullptr;
    Str8Node *tail = nullptr;
    
    u64 count = 0;

    void push(MemoryArena *arena, Str8 str);
    void push(Str8List list);
};

#define str8list_it(var, list)                          \
    for (auto var = list.head; var; var = var->next)

#define str8_lit(string) (Str8((char *)string, sizeof(string) - 1))
#define str8_exp(string) string.len ? (int)string.len : 4, string.len ? string.str : "null"
#define str8_fmt "%.*s"

#define str8_from_cstr(cstr) (Str8(cstr, cstr_len(cstr)))
#define str8_is_nil(string) (string.len == 0)

// File and string utilities.
Str8 file_working_dir(Str8 filename);
Str8 file_name(Str8 file_path);
Str8 file_base_name(Str8 filename);
Str8 file_ext(Str8 filename);
Str8 read_file(MemoryArena *allocator, Str8 filename);

Str8List arg_list(MemoryArena *allocator, int argc, char **argv);

// djb2 hash function for string hashing.
u64 djb2_hash(Str8 str);

#endif
