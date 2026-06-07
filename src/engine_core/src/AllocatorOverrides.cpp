#if defined(HUSH_USE_MIMALLOC)

#include <mimalloc.h>

#include <cstddef>
#include <new>

#include "Profiling.hpp"

void *operator new(std::size_t n)
{
	void *p = mi_malloc(n);
	if (p == nullptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(p, n);
	return p;
}

void *operator new[](std::size_t n)
{
	void *p = mi_malloc(n);
	if (p == nullptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(p, n);
	return p;
}

void *operator new(std::size_t n, std::align_val_t a)
{
	void *p = mi_malloc_aligned(n, static_cast<std::size_t>(a));
	if (p == nullptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(p, n);
	return p;
}

void *operator new[](std::size_t n, std::align_val_t a)
{
	void *p = mi_malloc_aligned(n, static_cast<std::size_t>(a));
	if (p == nullptr)
	{
		throw std::bad_alloc();
	}
	TracyAlloc(p, n);
	return p;
}

void *operator new(std::size_t n, const std::nothrow_t &) noexcept
{
	void *p = mi_malloc(n);
	if (p != nullptr)
	{
		TracyAlloc(p, n);
	}
	return p;
}

void *operator new[](std::size_t n, const std::nothrow_t &) noexcept
{
	void *p = mi_malloc(n);
	if (p != nullptr)
	{
		TracyAlloc(p, n);
	}
	return p;
}

void *operator new(std::size_t n, std::align_val_t a, const std::nothrow_t &) noexcept
{
	void *p = mi_malloc_aligned(n, static_cast<std::size_t>(a));
	if (p != nullptr)
	{
		TracyAlloc(p, n);
	}
	return p;
}

void *operator new[](std::size_t n, std::align_val_t a, const std::nothrow_t &) noexcept
{
	void *p = mi_malloc_aligned(n, static_cast<std::size_t>(a));
	if (p != nullptr)
	{
		TracyAlloc(p, n);
	}
	return p;
}

void operator delete(void *p) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete(void *p, std::size_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p, std::size_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete(void *p, std::align_val_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p, std::align_val_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete(void *p, std::size_t, std::align_val_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p, std::size_t, std::align_val_t) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete(void *p, const std::nothrow_t &) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p, const std::nothrow_t &) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete(void *p, std::align_val_t, const std::nothrow_t &) noexcept
{
	TracyFree(p);
	mi_free(p);
}

void operator delete[](void *p, std::align_val_t, const std::nothrow_t &) noexcept
{
	TracyFree(p);
	mi_free(p);
}

// Anchor: forces MSVC's linker to include this TU when linking HushEngine.lib
// into an exe. The CRT already provides a default operator new, so ours is a
// replacement (not a resolution) and the linker would otherwise skip this obj.
extern "C" void HushForceLinkAllocatorOverrides() noexcept
{
}

#endif
