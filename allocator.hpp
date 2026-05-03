#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include <system_error>

/**
* Simple arena allocator. Reserves a large contiguous virtual region, then
* splits it into smaller pieces as requested.
*/
class Allocator
{
public:

	// 
	// De/Constructors
	//
	
	/**
	* Constructor for the Allocator class, reserves a virtually contigious region.
	* Throws std::system_error if reservation fails.
	* 
	* @param Capacity The total capacity of the arena. Defaults to 1 TiB.
	*/
	explicit Allocator( std::size_t Capacity = DEFAULT_CAPACITY );

	~Allocator( );


	//
	// Public routines
	//
		
	// TODO: void* ReallocatePool( void* Address, uint64_t NewSize );
	void* AllocatePool  ( std::size_t Size ) noexcept;
	void  FreePool	    ( void* Address );


private: 

	static constexpr std::size_t DEFAULT_CAPACITY = 0x10000000000; // 1 TiB
	static constexpr std::size_t BLOCK_ALIGNMENT  = 16;

	//
	// Should get page size from system info, if we want this portable. 
	//

	static constexpr std::size_t PAGE_SIZE        = 0x1000;

	//
	// Block meta stored at the head of every allocation, the user poitner is immediatly after the header. 
	// Aligned to 16 bytes for shadow stack. 
	//

	struct alignas(BLOCK_ALIGNMENT) ARENA_BLOCK
	{
		std::size_t NextOffset; // bytes from this header to the next; 0 = tail
		std::size_t Size;       // total bytes including this header
		bool        Free;
	};

	static_assert( sizeof(ARENA_BLOCK) % BLOCK_ALIGNMENT == 0, "Arena header is not aligned to 16 bytes" );

	//
    // Internal routines
    //

	ARENA_BLOCK* FindFreeBlock( std::size_t Size  ) noexcept;
	ARENA_BLOCK* FindUserBlock( void* UserAddress ) noexcept;

	bool UpdateCommited( std::size_t Size ) noexcept;

	static 
	constexpr 
	std::size_t 
	AlignUp(
		std::size_t Value, 
		std::size_t Alignment = 8
	) noexcept
	{
		return (Value + (Alignment - 1)) & ~(Alignment - 1);
	}

	static
	std::byte* 
	GetBlockUserRegion(
		ARENA_BLOCK* Block
	) noexcept
	{
		return reinterpret_cast<std::byte*>(Block) + sizeof(ARENA_BLOCK);
	}

	static 
	ARENA_BLOCK* 
	GetNextBlock(
		ARENA_BLOCK* Block
	) noexcept
	{
		if ( Block->NextOffset == 0 )
		{ 
			return nullptr;
		}
		return reinterpret_cast<ARENA_BLOCK*>(
			reinterpret_cast<std::byte*>(Block) + Block->NextOffset
		);
	}

	//
	// Internal variables
	//

	std::size_t mSize         = 0; // Sum of total used buckets.
	std::size_t mCommited     = 0; // The amount of virtual memory we have commited via virtual alloc.
	std::size_t mCapacity     = 0; // Total capacity of the arena.
	std::byte*  mVirtualArena = nullptr; // Pointer to the start of the virtual arena.

	//
	// Store head and tail of block regions
	//

	ARENA_BLOCK* mBlockListHead  = nullptr;
	ARENA_BLOCK* mBlockListTail = nullptr;
};


//
// Singleton constructor for allocator.
//

Allocator& 
GetAllocator( 
	void 
);

