#include "../lib/hw.h"
#include "../h/syscall_c.h"

// PDF p.10: the global operator new/delete must be implemented as wrappers
// around mem_alloc/mem_free, so all C++ dynamic allocation in user code
// flows through the syscall.
//
// We define every overload the linker is likely to want (scalar, array,
// sized C++14 forms) so that any user-code shape compiles.
//
// NOTE: kernel code must NEVER use `new` — that would mean the kernel
// calling its own syscall. Kernel internals use MemoryAllocator::alloc
// directly + placement-new for construction.

void* operator new(size_t n)                       { return mem_alloc(n); }
void* operator new[](size_t n)                     { return mem_alloc(n); }

void  operator delete(void* p) noexcept            { mem_free(p); }
void  operator delete[](void* p) noexcept          { mem_free(p); }

// C++14 sized-delete forms. Required when classes have non-trivial dtors.
void  operator delete(void* p, size_t) noexcept    { mem_free(p); }
void  operator delete[](void* p, size_t) noexcept  { mem_free(p); }
