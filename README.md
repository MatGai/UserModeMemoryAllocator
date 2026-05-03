# UserModeMemoryAllocator

A simple arena-based memory allocator for Windows user-mode apps. Tried my best writing it in modern C++ 23. 
Allocator initially reserves a large contigous region, default to 1 TiB, and commits pages as needed via VirtualAlloc. 

Allocator currently uses a linked list to store meta data at the head of each allocation. 
The links store the offset to the next block, making it position independent and allowing relocations. 

### TODO

- Implement `ReallocatePool`.
- Merge adjacent free blocks to reduce fragmentation.
- Split free blocks if best/close fit not found.
- Make it **thread-safe**. Multiple threads calling `AllocatePool` or `FreePool` will race.
- Move away from a linked list, as it is slow at scale. Maybe implement hash map?

## License

MIT, see [LICENSE.txt](LICENSE.txt).