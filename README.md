# Memory_Management
# Custom Memory Allocator

A from-scratch heap allocator built on top of `mmap`, implementing manual memory management with **first-fit allocation**, **block splitting**, **bidirectional coalescing**, and **automatic region return to the OS**.

This project reimplements the core mechanics of `malloc` / `free` at a low level, without relying on the C standard library's allocator.

---

## Features

- **Page-backed regions** — memory is requested from the OS in page-aligned chunks via `mmap`, not one syscall per allocation.
- **First-fit search** — walks the free block list and returns the first block large enough to satisfy a request.
- **Block splitting** — if a free block is significantly larger than requested, it's split so the leftover remains usable instead of being wasted.
- **Bidirectional coalescing** — on `free()`, adjacent free blocks (both previous and next) are merged back into a single larger block to fight fragmentation.
- **Region reclamation** — when an entire region collapses into one fully-free block, it is `munmap`'d and returned to the operating system rather than held indefinitely.
- **In-band metadata** — block size and allocation status are packed into a single `size_t` using the low bit as a flag, avoiding a separate "is_free" field.

---

## How It Works

### Memory layout

```
Region
├── Region header (start, size, first, next)
├── Block header (size | alloc-bit, next, prev)
├── payload...
├── Block header
├── payload...
└── ...
```

Each `Region` is one `mmap`-backed chunk of memory (rounded up to a whole number of pages). Each `Region` owns a linked list of `Block`s. Blocks are laid out contiguously in memory, so a block's neighbors can always be found via pointer arithmetic and the linked list pointers.

### Block size / allocation flag packing

```c
static inline size_t get_size(Block* b)   { return b->size & ~(size_t)1; }
static inline int    is_free(Block* b)    { return !(b->size & (size_t)1); }
static inline void   set_free(Block* b)   { b->size &= ~(size_t)1; }
static inline void   set_allocated(Block* b) { b->size |= (size_t)1; }
```

Because all sizes are 8-byte aligned (`ALIGNMENT = 8`), the lowest bit of `size` is always `0` and free for use as the allocation flag. This is the same trick used by real allocators like dlmalloc.

### Allocation (`alloc`)

1. Round the requested size up to the nearest 8-byte boundary (`align`).
2. Walk every existing `Region`, and within each, walk the block list looking for the first free block big enough (`find first fit`).
3. If found, hand it to `allocate_block`, which either:
   - **Splits** the block (if the leftover space is bigger than `sizeof(Block) + MIN_BLOCK_SIZE`), creating a new free block from the remainder, or
   - **Uses the whole block** as-is if splitting would leave an unusably small sliver.
4. If no existing block fits anywhere, request a brand new `Region` from the OS via `mmap` and allocate from it.

### Deallocation (`my_free`)

1. Locate the `Region` that owns the pointer (`find_region`).
2. Recover the `Block` header (it sits immediately before the user's pointer).
3. Mark the block free.
4. **Coalesce backward**: while the previous block is also free, merge it into the current block.
5. **Coalesce forward**: while the next block is also free, merge it into the current block.
6. If, after coalescing, the block is the *only* block in the region and it's free, the whole region is empty — `munmap` it and remove it from the region list.

---

## API

```c
void* alloc(size_t n);   // allocate n bytes, 8-byte aligned
void  my_free(void* ptr); // free a pointer previously returned by alloc()
```

---

## Build & Run

```bash
gcc -std=c11 -o allocator allocator.c
./allocator
```

The included `main()` demonstrates the allocator by allocating a 400,000-element `int` array, filling it with random values, and sorting it with a 3-way quicksort — a decent stress test since it exercises one large allocation with heavy read/write access.

---

## Project Structure

Single-file implementation (`allocator.c`) containing:

- Alignment & bit-packing helpers
- `Block` / `Region` structs
- `request_region` — mmap-backed region allocation
- `allocate_block` — first-fit + splitting logic
- `alloc` / `my_free` — public API
- `find_region` / `remove_region` — region bookkeeping and OS-level release
- A 3-way quicksort demo in `main()` to exercise the allocator
