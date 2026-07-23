#include <stdio.h>
#define HEAP_SIZE 1024*1024
#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 8

size_t align(size_t n)
{
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

struct Block {
    size_t size;
    struct Block* next;
    struct Block* prev;
} typedef Block;

static inline size_t get_size(Block* b)
{
    return b->size & ~(size_t)1;
}

static inline int is_free(Block* b)
{
    return !(b->size & (size_t)1);
}

static inline void set_free(Block* b)
{
    b->size &= ~(size_t)1;
}

static inline void set_allocated(Block* b)
{
    b->size |= (size_t)1;
}

unsigned char heap[HEAP_SIZE];
Block* head = (Block*) heap;

void *alloc(size_t n)
{
    n = align(n);

    if (head->next == NULL && is_free(head))
    {
        head->size = n;
        set_allocated(head);
        head->prev = NULL;

        Block *next = (Block *)((char *)head + sizeof(Block) + n);

        if ((char *)next >= (heap + HEAP_SIZE))
            return NULL;

        head->next = next;
        next->next = NULL;
        next->prev = head;
        next->size = 0;
        set_free(next);

        return (void *)(head + 1);
    }

    Block *block = head;

    while (block->next->next != NULL) {
        if(is_free(block) && get_size(block) >= n) {
            size_t s = get_size(block) - n;

            if(s > MIN_BLOCK_SIZE) {
                block->size = n;
                set_allocated(block);

                Block* new = (Block*) ((char*)block + sizeof(Block) + n);

                new->size = s - sizeof(Block);
                set_free(new);

                new->next = block->next;
                new->prev = block;
                block->next = new;

                if (new->next != NULL)
                    new->next->prev = new;

                return (void *)(block + 1);
            }
            else {
                set_allocated(block);
                return (void *)((char*)block + sizeof(Block));
            }
        }

        block = block->next;
    }

    Block *curr = block->next;

    curr->size = n;
    set_allocated(curr);
    curr->prev = block;

    Block *next = (Block *)((char *)curr + sizeof(Block) + n);

    if ((char *)next >= heap + HEAP_SIZE)
        return NULL;

    curr->next = next;
    next->next = NULL;
    next->prev = curr;
    next->size = 0;
    set_free(next);

    return (void *)(curr + 1);
}

void my_free(void* addr)
{
    if(addr == NULL)
        return;

    if((unsigned char*)addr < heap ||
       (unsigned char*)addr >= heap + HEAP_SIZE)
        return;

    char* address = (char*) addr;
    Block* b = (Block*) (address - sizeof(Block));

    set_free(b);

    while(b->prev && is_free(b->prev)) {
        b = b->prev;
        b->size = get_size(b) + get_size(b->next) + sizeof(Block);
        b->next = b->next->next;

        if(b->next != NULL)
            b->next->prev = b;
    }

    while(b->next && is_free(b->next)) {
        b->size = get_size(b) + get_size(b->next) + sizeof(Block);
        b->next = b->next->next;

        if(b->next != NULL)
            b->next->prev = b;
    }
}

void init_allocator(void)
{
    head = (Block *)heap;
    head->size = 0;
    set_free(head);
    head->next = NULL;
    head->prev = NULL;
}

void printf_heap_info()
{
    Block* block = head;
    int i = 0;

    while(block != 0) {
        printf("Block %d:\n", i++);
        printf("Address : %p\n", block);
        printf("Size : %lu\n", get_size(block));
        printf("Free : %d\n", is_free(block));
        block = block->next;
    }
}
void pause_test()
{
    printf("\nPress Enter to continue...");
    getchar();
}
int main()
{
    init_allocator();

    printf("========== TEST 1 ==========\n");

    int *a = alloc(sizeof(int));
    int *b = alloc(sizeof(int));
    double *c = alloc(sizeof(double));
    char *d = alloc(100);

    *a = 10;
    *b = 20;
    *c = 3.14159;

    for(int i = 0; i < 100; i++)
        d[i] = 'A' + (i % 26);

    printf("%d\n", *a);
    printf("%d\n", *b);
    printf("%.5lf\n", *c);
    printf("%c %c %c\n", d[0], d[1], d[2]);

    printf_heap_info();

    pause_test();

    printf("\n========== TEST 2 ==========\n");

    my_free(b);
    my_free(d);

    printf_heap_info();

    pause_test();

    printf("\n========== TEST 3 ==========\n");

    int *e = alloc(sizeof(int));
    char *f = alloc(50);

    *e = 500;

    for(int i = 0; i < 50; i++)
        f[i] = 'Z';

    printf("%d\n", *e);
    printf("%c\n", f[0]);

    printf_heap_info();

    pause_test();

    printf("\n========== TEST 4 ==========\n");

    void *ptrs[100];

    for(int i = 0; i < 100; i++)
    {
        ptrs[i] = alloc(32);

        if(ptrs[i] == NULL)
        {
            printf("Allocation failed at %d\n", i);
            break;
        }
    }

    pause_test();

    printf("\n========== TEST 5 ==========\n");

    for(int i = 0; i < 100; i += 2)
        my_free(ptrs[i]);

    printf_heap_info();

    pause_test();

    printf("\n========== TEST 6 ==========\n");

    void *newptrs[50];

    for(int i = 0; i < 50; i++)
    {
        newptrs[i] = alloc(16);

        if(newptrs[i] == NULL)
        {
            printf("Allocation failed\n");
            break;
        }
    }

    printf_heap_info();

    pause_test();

    printf("\n========== TEST 7 ==========\n");

    char *big = alloc(50000);

    if(big)
    {
        for(int i = 0; i < 50000; i++)
            big[i] = i % 256;

        printf("Large allocation successful\n");
    }
    else
    {
        printf("Large allocation failed\n");
    }


    pause_test();

    printf("\n========== TEST 8 ==========\n");

    while(alloc(1024) != NULL);

    printf("Allocator exhausted.\n");


    return 0;
}