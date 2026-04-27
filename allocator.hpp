#pragma once

#define WIN32_LEAN_AND_MEAN 
#include <Windows.h>

#include <stdint.h>
#include <stdlib.h>

typedef enum _BUCKET_FLAGS
{
	BUCKET_FLAG_FREE = 0,
	BUCKET_FLAG_ALLOCATED
} BUCKET_FLAGS;

/**
* Used to store information about an allocated region of memory.
*/
typedef struct _ALLOCATED_BUCKET
{
	uint64_t          Offset; // Offset from the start of the arena.
	uint64_t          Size;	  // Size of the allocated region.
	ALLOCATED_BUCKET* Next;	  // Pointer to the next allocated bucket in the linked list.
	BUCKET_FLAGS      Flags;  // Flags for future use (e.g., to indicate if the bucket is free or allocated).
	char     Data[1];         //
} ALLOCATED_BUCKET;

/**
* Simple arena allocator. Gets a large contigous virtual region, then splits it up into smaller peices as requested. 
*/
class Allocator
{

public:

	// 
	// De/Constructors
	//

	Allocator( uint64_t Capacity = DEFAULT_CAPACITY );
	~Allocator( );

	//
	// Public routines
	//
		
	void* ReallocatePool( void* Address, uint64_t NewSize );
	void* AllocatePool( uint64_t Size );
	void  FreePool( void* Address );

	//
	// Internal routines
	//
private: 

	// Aligns the given size to an 8 byte boundary.
	uint64_t AlignToQword( uint64_t Size );

private:
	static constexpr uint64_t DEFAULT_CAPACITY = 1024 * 1024; // 1 MiB

	uint64_t mSize;     // Sum of total used buckets.
	uint64_t mCapacity;	// Total capacity of the arena.
	void* VirtualArena; // Pointer to the start of the virtual arena.
};

