// ========================================================================
// heap.c - reduceOS kernel's heap.
// This file handles managing the heap and works in tandem with paging.c
// ========================================================================
// This file is a part of the reduceOS C kernel. If you use this code, please credit the implementation's original owner (if you directly copy and paste this, also add me please)
// This implementation is not by me - credits for the implementation are present in heap.h

#include "include/heap.h" // Main header file

// heap.h defines end - but we need to get a uint32_t pointer to end.
uint32_t placement_address = (uint32_t)&end;
heap_t *kernelHeap = 0; // This shows kmalloc_int not to immediately use paging functions yet.
extern page_directory *kernelDir; // Defined in paging.c


// These aren't the actual heap functions - that's after.

// kmalloc_heap(uint32_t size, int align, uint32_t *phys) - To fix a circular dependency, we use this function, which is called by stdlib.c whenever kheap is not 0.
uint32_t kmalloc_heap(uint32_t size, int align, uint32_t *phys) {
    // stdlib.c has handed control to us.
    void *address = alloc(size, (uint8_t)align, kernelHeap); // Get the address of what we are allocating.
    if (phys != 0) {
        // Set the physical address.
        page *pg = getPage((uint32_t)address, 0, kernelDir); // Get the page on our allocation address.
        *phys = pg->frame*PLACEMENT_ALIGN + (uint32_t)address & 0xFFF; // Get the physical address.
    }
    return (uint32_t)address;
}


// Moving on to the actual heap functions...

// The point of this function is mainly for convienience, it automatically uses kheap.
void kfree(void *p) { free(p, kernelHeap); }


// (static) expand(uint32_t newSize, heap_t *heap) - Expand the heap to newSize.
static void expand(uint32_t newSize, heap_t *heap) {
    // Sanity check - make sure newSize and size are valid!
    ASSERT(newSize > heap->endAddress - heap->startAddress);

    // Get the nearest following page boundary
    if (newSize & 0xFFFFF000 != 0) {
        // phys. address alignment
        newSize &= 0xFFFFF000;
        newSize += PLACEMENT_ALIGN;
    }

    // Check we aren't overreaching.
    ASSERT(heap->startAddress+newSize <= heap->maxAddress);

    // oldSize is always on a page boundary
    uint32_t oldSize = heap->endAddress - heap->startAddress; // previous size

    // Now expand the heap.
    uint32_t i = oldSize;
    while (i < newSize) {
        allocateFrame(getPage(heap->startAddress+i, 1, kernelDir),
                            (heap->supervisor) ? 1 : 0, (heap->readonly) ? 0 : 1);
        i += PAGE_SIZE;
    }

    heap->endAddress = heap->startAddress + newSize;
}


// (static) contract(uint32_t newSize, heap_t *heap) - Shrink heap to newSize.
static uint32_t contract(uint32_t newSize, heap_t *heap) {
    // Same thing but different 
    ASSERT(newSize < heap->endAddress - heap->startAddress)

    // Now, get the nearest following page boundary
    if (newSize & PAGE_SIZE) {
        newSize &= PAGE_SIZE;
        newSize += PAGE_SIZE;
    }
    

    // Make sure those pesky users aren't trying to put newSize below HEAP_MINIMUM_SIZE
    if (newSize < HEAP_MINIMUM_SIZE) newSize = HEAP_MINIMUM_SIZE;

    // Shrink the heap.
    uint32_t oldSize = heap->endAddress - heap->startAddress;
    uint32_t i = oldSize - PAGE_SIZE;
    while (newSize < i ) {
        freeFrame(getPage(heap->startAddress+i, 0, kernelDir));
        i -= PAGE_SIZE;
    }

    heap->endAddress = heap->startAddress + newSize;
    return newSize;
}


// (static) findSmallestHole(uint32_t size, uint8_t pageAlign, heap_t *heap) - Find the smallest hole that will fit.
static int32_t findSmallestHole(uint32_t size, uint8_t pageAlign, heap_t *heap) {
    uint32_t iterator = 0;
    while (iterator < heap->index.size) {
        header_t *header = (header_t *)lookupOrderedArray(iterator, &heap->index);
        // Check if the user has requested to page align this
        if (pageAlign > 0) {
            // Page align the starting point of the header
            uint32_t location = (uint32_t)header;
            int32_t offset = 0;
            if ((location + sizeof(header_t)) & 0xFFFFF000 != 0) offset = PAGE_SIZE - (location + sizeof (header_t)) % 0x1000;
            int32_t holeSize = (int32_t)header->size - offset;
            // Is there enough space?
            if (holeSize >= (int32_t)size) break;
        } else if (header->size >= size) break;
        iterator++;
    }

    // Check if the while loop exited due to error or not.

    if (iterator == heap->index.size) return -1; // We didn't find anything.
    return iterator; // We found something!
}


// (static) header_less_than(void *a, void *b) - Check which header is bigger.
static int8_t header_less_than(void *a, void *b) {
    return (((header_t*)a)->size < ((header_t*)b)->size) ? 1 : 0;
}

// No more static functions!

// createHeap(uint32_t startAddress, uint32_t endAddress, uint32_t maxAddress, uint8_t supervisor, uint8_t readonly) - Create a heap!
heap_t *createHeap(uint32_t startAddress, uint32_t endAddress, uint32_t maxAddress, uint8_t supervisor, uint8_t readonly) {
    heap_t *heap = (heap_t*)kmalloc(sizeof(heap_t));

    // Double check startAddress and endAddress are both page aligned.
    ASSERT(startAddress % PAGE_ALIGN == 0);
    ASSERT(endAddress % PAGE_ALIGN == 0);

    // Great! Initailize the index:
    heap->index = placeOrderedArray((void*)startAddress, HEAP_INDEX_SIZE, &header_less_than);

    // Now shift the start address forward to show where we can start putting data.
    startAddress += sizeof(type_t)*HEAP_INDEX_SIZE;

    // Make sure the start address is page aligned.
    if (startAddress & 0xFFFFF000 != 0) {
        startAddress &= 0xFFFFF000;
        startAddress += PAGE_ALIGN;
    }

    // Now setup the values for our heap.
    heap->startAddress = startAddress;
    heap->endAddress = endAddress;
    heap->maxAddress = maxAddress;
    heap->supervisor = supervisor;
    heap->readonly = readonly;

    // Start off with one large hole in the index.
    header_t *hole = (header_t*)startAddress;
    hole->size = endAddress - startAddress;
    hole->magic = HEAP_MAGIC;
    hole->isHole = 1;
    insertOrderedArray((void*)hole, &heap->index);

    // Return the heap!
    return heap;
}


// alloc(uint32_t size, uint8_t pageAlign, heap_t *heap) - Allocate to a heap (of size "size" and page align if pageAlign != 0)
void *alloc(uint32_t size, uint8_t pageAlign, heap_t *heap) {
    // We take the size of header/footer into account.
    uint32_t newSize = size + sizeof(header_t) + sizeof(footer_t);

    // Find the smallest hole that will fit.
    int32_t iterator = findSmallestHole(newSize, pageAlign, heap);


    // Check if findSmallestHole() returned a header or not
    if (iterator == -1) {
        // It didn't - we have work to do.

        // Save some previous data first.
        uint32_t oldLength = heap->endAddress - heap->startAddress;
        uint32_t oldEndAddress = heap->endAddress;

        // Expand heap to newSize.
        expand(oldLength+newSize, heap);
        uint32_t newLength = heap->endAddress - heap->startAddress;

        // Find the endmost header (in location)
        iterator = 0;
        
        // Declare variables to hold index of and value of the endmost header so far.
        uint32_t index = -1; uint32_t value = 0x0;

        // Loop through to find it.
        while (iterator < heap->index.size) {
            uint32_t tmp = (uint32_t)lookupOrderedArray(iterator, &heap->index);
            if (tmp > value) {
                value = tmp;
                index = iterator;
            }

            iterator++;
        }

        // Add a header if we didn't find any
        if (index == -1) {
            // Get a header from the old end address.
            header_t *header = (header_t *)oldEndAddress;
            
            // Fill in the header data
            header->magic = HEAP_MAGIC;
            header->size = newLength - oldLength;
            header->isHole = 1;

            // Create a footer.
            footer_t *footer = (footer_t*) (oldEndAddress + header->size - sizeof(footer_t));
            
            // Fill in the footer data.
            footer->magic = HEAP_MAGIC;
            footer->header = header;

            // Insert into ordered array.
            insertOrderedArray((void*)header, &heap->index);
        } else {
            // Last header needs adjusting
            header_t *header = lookupOrderedArray(index, &heap->index);
            header->size += newLength - oldLength;
            
            // Rewrite the footer
            footer_t *footer = (footer_t*) ((uint32_t)header + header->size - sizeof(footer_t));
            footer->magic = HEAP_MAGIC;
            footer->header = header;
        }
        
        // We know there is enough space. Recurse and call function again
        return alloc(size, pageAlign, heap);
    }

    header_t *originalHoleHeader = (header_t *)lookupOrderedArray(iterator, &heap->index);
    uint32_t originalHolePosition = (uint32_t) originalHoleHeader;
    uint32_t originalHoleSize = originalHoleHeader->size;

    
    // Work out if we should split thehole we found into two parts (check if the OG size minus the requested size is less than the overhead for adding a new hole)
    if (originalHoleSize - newSize < sizeof(header_t) + sizeof(footer_t)) {
        // If so, increase the requested size to the size of the hole we found.
        size += originalHoleSize - newSize;
        newSize = originalHoleSize;
    }


    // Check if we need to page align the data. If so, do it now and make a new hole in front of our block.
    if (pageAlign && originalHolePosition & 0xFFFFF000) {
        uint32_t newLocation = originalHolePosition + PAGE_SIZE - (originalHolePosition & 0xFFF) - sizeof(header_t);
        header_t *holeHeader = (header_t*)originalHolePosition;
        
        // Fill in the data for the hole header
        holeHeader->size = PAGE_SIZE - (originalHolePosition & 0xFFF) - sizeof(header_t);
        holeHeader->magic = HEAP_MAGIC;
        holeHeader->isHole = 1;
        
        // Update original positions
        originalHolePosition = newLocation;
        originalHoleSize = originalHoleSize - holeHeader->size;
    } else {
        // We don't need this hole any more. Delete it from the index.
        removeOrderedArray(iterator, &heap->index);
    }

    // Now overwrite the original header.
    header_t *blockHeader = (header_t *)originalHolePosition;
    
    // Fill in the data for blockHeader...
    blockHeader->size = newSize;
    blockHeader->magic = HEAP_MAGIC;
    blockHeader->isHole = 0;

    // Overwrite the original footer.
    footer_t *blockFooter = (footer_t *) (originalHolePosition + sizeof(header_t) + size);

    // Fill in the data for blockFooter...
    blockFooter->magic = HEAP_MAGIC;
    blockFooter->header = blockHeader;

    // We might need to write a new hole after the allocated block. Do this only if the new hole would've been positive size
    if (originalHoleSize - newSize > 0) {
        header_t *holeHeader = (header_t *) (originalHolePosition + sizeof(header_t) + size + sizeof(footer_t));
        holeHeader->magic = HEAP_MAGIC;
        holeHeader->isHole = 1;
        holeHeader->size = originalHoleSize - newSize;

        footer_t *holeFooter = (footer_t*) ((uint32_t)holeHeader + originalHoleSize - newSize - sizeof(footer_t));
        if ((uint32_t)holeFooter < heap->endAddress) {
            holeFooter->magic = HEAP_MAGIC;
            holeFooter->header = holeHeader;
        }

        // Put the new hole in the index.
        insertOrderedArray((void*)holeHeader, &heap->index);
    }

    // Finally, we're done!
    return (void*) ((uint32_t)blockHeader + sizeof(header_t));
}

// free(void *p, heap_t *heap) - Free *p from *heap.
void free(void *p, heap_t *heap) {
    // Again, we have to check if pesky users are mucking up OUR input.
    if (p == 0) return;

    // Get header and footer associated with this pointer
    header_t *header = (header_t*) ((uint32_t)p - sizeof(header_t));
    footer_t *footer = (footer_t*) ((uint32_t)header + header->size - sizeof(footer_t));

    // Sanity checks - check if magics are both HEAP_MAGIC
    ASSERT(header->magic == HEAP_MAGIC);
    ASSERT(header->magic == HEAP_MAGIC);

    // Make the header a hole
    header->isHole = 1;

    // Add this header to the "free holes" index?
    char doAdd = 1;

    // Unify left (if the thing immediately to the left of us is a footer)
    footer_t *testFooter = (footer_t*) ((uint32_t)header - sizeof(footer_t));

    if (testFooter->magic == HEAP_MAGIC && testFooter->header->isHole == 1) {
        uint32_t cacheSize = header->size; // Cache the current size.
        header = testFooter->header; // Rewrite the header with the new one.
        footer->header = header; // Rewrite footer's header address.
        header->size += cacheSize; // Update the size.
        doAdd = 0; // Header is already in the index - don't add.
    }

    // Unify right (if the thing immediately to the right of us is a header)
    header_t *testHeader = (header_t*) ((uint32_t)footer + sizeof(footer_t));
    if (testHeader->magic == HEAP_MAGIC && testHeader->isHole) {
        header->size = testHeader->size; // Increase size.
        testFooter = (footer_t*) ((uint32_t)testHeader + testHeader->size + sizeof(footer_t)); // Rewrite footer to point to our header
        footer = testFooter;

        // Find and remove the header from the index.
        uint32_t iterator = 0;
        while ((iterator < heap->index.size) && (lookupOrderedArray(iterator, &heap->index) != (void*)testHeader)) iterator++;

        // Sanity check - make sure we found the item.
        ASSERT(iterator < heap->index.size);
        // Remove it.
        removeOrderedArray(iterator, &heap->index);
    }

    // Contract if the footer location is the end address.
    if ((uint32_t)footer + sizeof(footer_t) == heap->endAddress) { 
        uint32_t oldLength = heap->endAddress - heap->startAddress;
        uint32_t newLength = contract((uint32_t)header - heap->startAddress, heap);

        // Check how big we will be after resizing
        if (header->size - (oldLength - newLength) > 0) {
            // We will still exist, yay! Resize us.
            header->size -= oldLength-newLength;
            footer = (footer_t*) ((uint32_t)header + header->size - sizeof(footer_t));
            footer->magic = HEAP_MAGIC;
            footer->header = header;
        } else {
            // we will no longer exist :( - remove us from index
            // (rip header)
            uint32_t iterator = 0;
            while ((iterator < heap->index.size) && (lookupOrderedArray(iterator, &heap->index) != (void*)testHeader)) iterator++;

            // Check if we didn't find ourselves. If we didn't, nothing to remove.
            if (iterator < heap->index.size) {
                removeOrderedArray(iterator, &heap->index);
            }
        }
    }
    
    // Add us to index if required.
    if (doAdd == 1) insertOrderedArray((void*)header, &heap->index);
}