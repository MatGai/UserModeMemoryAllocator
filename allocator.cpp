#include "allocator.hpp"

Allocator::Allocator(
	uint64_t Capacity = DEFAULT_CAPACITY
)
: mSize(0), mCapacity(Capacity), VirtualArena(nullptr)
{
	VirtualArena = VirtualAlloc( nullptr, Capacity, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE );
}

Allocator::~Allocator(
	void
)
{
	if ( VirtualArena != nullptr )
	{
		VirtualFree( VirtualArena, 0, MEM_RELEASE );
	}
}

uint64_t
Allocator::AlignToQword(
	uint64_t Size
)
{
	return ( ( ( Size - 1 ) >> 3 ) << 3 ) + 8;
}