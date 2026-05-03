#include "allocator.hpp"

Allocator::Allocator(
	std::size_t Capacity
)
: mCapacity(Capacity)
{
	// 
	// Per msdn -- https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc
	// We reserve memory so we can have a contigous virtual backing. When needed we will commit the reserved memory. 
	// 

	void* Arena = ::VirtualAlloc( nullptr, Capacity, MEM_RESERVE, PAGE_READWRITE );
	if ( Arena == nullptr )
	{
		throw std::system_error(
			static_cast<int>(::GetLastError()),
			std::system_category(),
			"VirtualAlloc failed MEM_RESERVE"
		);
	}
	mVirtualArena = static_cast<std::byte*>(Arena);
}

Allocator::~Allocator(
	void
)
{
	if ( mVirtualArena != nullptr )
	{
		::VirtualFree( mVirtualArena, 0, MEM_RELEASE );
	}
}

void* 
Allocator::AllocatePool(
	std::size_t Size
) noexcept
{
	if ( Size == 0 || mVirtualArena == nullptr )
	{
		return nullptr;
	}

	std::size_t AlignedSize = AlignUp(Size + sizeof(ARENA_BLOCK), BLOCK_ALIGNMENT);
	
	//
	// Try find free block with correct size. We don't modify anything, just unset the free flag
	// and return it. 
	//

	ARENA_BLOCK* FreeBlock = FindFreeBlock( AlignedSize );
	if ( FreeBlock != nullptr )
	{
		FreeBlock->Free = false;
		return GetBlockUserRegion(FreeBlock);
	}

	//
	// Could not find free block, first check if we can commit more memory. 
	//

	bool CanCommit = UpdateCommited( AlignedSize );
	if ( CanCommit == false )
	{
		return nullptr;
	}

	//
	// If this is first call, arena is clean. No need to append to tail.
	//

 	if ( mBlockListHead == nullptr )
	{
		mBlockListHead		       = reinterpret_cast<ARENA_BLOCK*>(mVirtualArena);
		mBlockListHead->Size       = AlignedSize;
		mBlockListHead->NextOffset = 0;
		mBlockListHead->Free       = false;
		mBlockListTail             = mBlockListHead;
		mSize					   += AlignedSize;
		return GetBlockUserRegion(mBlockListHead);
	}

	//
	// Append a block after current tail, and set new tail.
	//

	ARENA_BLOCK* NewBlock = reinterpret_cast<ARENA_BLOCK*>( 
		reinterpret_cast<std::byte*>(mBlockListTail) + mBlockListTail->Size 
	);

	NewBlock->NextOffset = 0;
	NewBlock->Size = AlignedSize;
	NewBlock->Free = false;

	mBlockListTail->NextOffset = static_cast<std::size_t>( 
		reinterpret_cast<std::byte*>(NewBlock) - reinterpret_cast<std::byte*>(mBlockListTail) 
		);
	mBlockListTail = NewBlock;

	mSize += AlignedSize;
	return GetBlockUserRegion(NewBlock);
}

void 
Allocator::FreePool(
	void* Address
)
{
	if ( Address == nullptr || mVirtualArena == nullptr )
	{
		return; 
	}

	ARENA_BLOCK* FoundBlock = FindUserBlock( Address );
	if ( FoundBlock == nullptr || FoundBlock->Free )
	{
		return;
	}

	//
	// This is rudimentary, we just set block to free. Will be reused if <= memory is needed. 
	// Zeroing the memory is pointless, but for debugging we will.
	// 

	FoundBlock->Free = true;

#ifdef _DEBUG
	std::memset( 
		GetBlockUserRegion(FoundBlock), 
		0xAA, 
		FoundBlock->Size - sizeof(ARENA_BLOCK)
	);
#endif
}

Allocator::ARENA_BLOCK* 
Allocator::FindFreeBlock(
	std::size_t Size
) noexcept
{
	ARENA_BLOCK* BestFit = nullptr;
	for ( ARENA_BLOCK* CurrentBlock = mBlockListHead; CurrentBlock != nullptr; CurrentBlock = GetNextBlock(CurrentBlock) )
	{
		if ( 
			( CurrentBlock->Free && CurrentBlock->Size >= Size ) && 
			( BestFit == nullptr || CurrentBlock->Size < BestFit->Size ) 
		)
		{
			BestFit = CurrentBlock;
		}
	}

	return BestFit;
}

Allocator::ARENA_BLOCK* 
Allocator::FindUserBlock(
	void* UserAddress
) noexcept
{
	std::byte* TargetAddress = static_cast<std::byte*>(UserAddress);
	for (ARENA_BLOCK* CurrentBlock = mBlockListHead; CurrentBlock != nullptr; CurrentBlock = GetNextBlock(CurrentBlock))
	{
		if ( GetBlockUserRegion(CurrentBlock) == TargetAddress )
		{
			return CurrentBlock;
		}
	}
	
	return nullptr;
}

bool
Allocator::UpdateCommited(
	std::size_t Size
) noexcept
{
	if ( mSize + Size <= mCommited )
	{
		return true;
	}

	std::size_t NewCommitedSize = mCommited * 2;
	if ( mCommited == 0 )
	{
		NewCommitedSize = PAGE_SIZE;
	}

	NewCommitedSize = std::max( AlignUp(NewCommitedSize, PAGE_SIZE), AlignUp(mSize + Size, PAGE_SIZE) );
	if ( NewCommitedSize > mCapacity )
	{
		return false;
	}
	
	std::byte*  CommitHead = mVirtualArena + mCommited;
	std::size_t CommitSize = NewCommitedSize - mCommited;

	if ( ::VirtualAlloc( CommitHead, CommitSize, MEM_COMMIT, PAGE_READWRITE ) == nullptr )
	{
		return false;
	}

	mCommited = NewCommitedSize;
	return true;
}

Allocator& 
GetAllocator(
	void
)
{
	static Allocator* Instance = new Allocator();
	return *Instance;
}
