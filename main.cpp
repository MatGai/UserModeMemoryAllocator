#include "allocator.hpp"
#include <stdio.h>

struct TEST_STRUCT
{
	std::uint64_t v1;
	std::uint32_t v2;
	std::uint8_t pad[0x700];
};

static constexpr std::size_t TEST_ARR_COUNT = 10;
TEST_STRUCT* t1[TEST_ARR_COUNT];

int 
main(
	void
)
{
	Allocator& Mem = GetAllocator();

	for ( std::uint32_t Index = 0; Index < TEST_ARR_COUNT; ++Index )
	{
		t1[Index] = static_cast<TEST_STRUCT*>(
			Mem.AllocatePool( sizeof( *t1[Index] ) )
		);
		t1[Index]->v1 = Index * 10;
		t1[Index]->v2 = Index * 5;
	}

	//
	// Free everything, blocks should be re-usable.
	//

	for ( std::uint32_t Index = 0; Index < TEST_ARR_COUNT; ++Index )
	{
		Mem.FreePool( t1[Index] );
	}

	//
	// Should re-use the same memory as the first allocation
	//

	for ( std::uint32_t Index = 0; Index < TEST_ARR_COUNT; ++Index )
	{
		t1[Index] = static_cast<TEST_STRUCT*>(
			Mem.AllocatePool(sizeof(*t1[Index]))
		);
		t1[Index]->v1 = Index * 10;
		t1[Index]->v2 = Index * 5;
	}

	std::printf( "Allication and freeing did not fail!\n" );

	return 0;
}