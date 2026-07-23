#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define MIN_BLOCK_SIZE 16
#define ALIGNMENT 8

size_t align(size_t n)
{
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

typedef struct Block {
    size_t size;
    struct Block* next;
    struct Block* prev;
} Block;

typedef struct Region {
    void* start;
    size_t size;
    Block* first;
    struct Region* next;
} Region;


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


Region* head = NULL;


Region* request_region(size_t min)
{
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t region_size =
        ((min + sizeof(Region) + sizeof(Block) + page_size - 1)/ page_size) * page_size;
    void* ptr = mmap(NULL,region_size,PROT_READ | PROT_WRITE,MAP_PRIVATE | MAP_ANONYMOUS,-1,0);
    if(ptr == MAP_FAILED)
        return NULL;
    Region* region = (Region*)ptr;
    region->start = ptr;
    region->size = region_size;
    region->next = NULL;
    Block* first = (Block*)((char*)ptr + sizeof(Region));
    first->size = region_size - sizeof(Region) - sizeof(Block);
    first->next = NULL;
    first->prev = NULL;
    set_free(first);
    region->first = first;
    return region;
}


void* allocate_block(size_t n, Block* block)
{
    if(!is_free(block) || get_size(block) < n)
        return NULL;


    size_t remaining = get_size(block) - n;


    if(remaining > sizeof(Block) + MIN_BLOCK_SIZE)
    {
        Block* new_block =
            (Block*)((char*)block + sizeof(Block) + n);


        new_block->size =
            remaining - sizeof(Block);

        set_free(new_block);


        new_block->next = block->next;
        new_block->prev = block;


        if(block->next)
            block->next->prev = new_block;


        block->next = new_block;


        block->size = n;
        set_allocated(block);
    }
    else
    {
        set_allocated(block);
    }


    return (char*)block + sizeof(Block);
}


void* alloc(size_t n)
{
    n = align(n);
    Region* region = head;
    while(region)
    {
        Block* block = region->first;


        while(block)
        {
            void* ptr = allocate_block(n, block);

            if(ptr)
                return ptr;
            block = block->next;
        }


        region = region->next;
    }
    Region* new_region = request_region(n);
    if(new_region == NULL)
        return NULL;
    if(head == NULL)
        head = new_region;
    else {
        region = head;

        while(region->next)
            region = region->next;

        region->next = new_region;
    }
    return allocate_block(n, new_region->first);
}
Region* find_region(void *addr) {
    Region *region = head;
    while(region) {
        if(addr >= region->start && addr < (char*)region->start + region->size) {
            return region;
        }

        region = region->next;
    }

    return NULL;
}
void remove_region(Region *target) {
    if(target == head) {
        head = head->next;
        munmap(target->start, target->size);
        return;
    }

    Region *prev = head;

    while(prev->next != target)
        prev = prev->next;


    prev->next = target->next;

    munmap(target->start, target->size);
}
void my_free(void *ptr) {
    if(ptr == NULL)
        return;
    Region *region = find_region(ptr);
    if(region == NULL) return;
    Block *block = (Block*)((char*)ptr - sizeof(Block));
    set_free(block);
    while(block->prev && is_free(block->prev)) {
        Block *prev = block->prev;

        prev->size =
            get_size(prev) +
            sizeof(Block) +
            get_size(block);

        prev->next = block->next;

        if(block->next)
            block->next->prev = prev;

        block = prev;
    }
    while(block->next && is_free(block->next)) {
        Block *next = block->next;

        block->size =
            get_size(block) +
            sizeof(Block) +
            get_size(next);

        block->next = next->next;

        if(next->next)
            next->next->prev = block;
    }
    if(region->first == block &&
       block->next == NULL &&
       is_free(block))
    {
        if(region != head)
        {
            remove_region(region);
        }
    }
}
void swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}
int medianOfThree(int arr[], int low, int high)
{
    int mid = low + (high-low)/2;


    if(arr[low] > arr[mid])
        swap(&arr[low], &arr[mid]);

    if(arr[low] > arr[high])
        swap(&arr[low], &arr[high]);

    if(arr[mid] > arr[high])
        swap(&arr[mid], &arr[high]);


    return arr[mid];
}
void quickSort3Way(int arr[], int low, int high)
{

    while(low < high)
    {

        int pivot = medianOfThree(arr, low, high);


        int lt = low;
        int i = low;
        int gt = high;


        while(i <= gt)
        {

            if(arr[i] < pivot)
            {
                swap(&arr[i], &arr[lt]);
                i++;
                lt++;
            }

            else if(arr[i] > pivot)
            {
                swap(&arr[i], &arr[gt]);
                gt--;
            }

            else
            {
                i++;
            }
        }
        if(lt-low < high-gt)
        {
            quickSort3Way(arr, low, lt-1);
            low = gt+1;
        }

        else
        {
            quickSort3Way(arr, gt+1, high);
            high = lt-1;
        }

    }
}
int main() {
    int* arr = (int*) alloc(400000*sizeof(int));
    for(int i = 0; i < 400000; i++) {
        arr[i] = rand();
    }
    quickSort3Way(arr, 0, 400000);
    for(int i = 0; i < 4; i++) {
        printf("%d, ", arr[i*100000]);
    } printf("\n");
    return 0;
}
