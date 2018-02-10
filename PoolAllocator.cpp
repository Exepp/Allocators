#include "PoolAllocator.h"

Pool::Pool(size_t size, size_t typeSize, uint8_t alignment)
{
	init(size, typeSize, alignment);
}

void Pool::init(size_t size, size_t typeSize, uint8_t alignment)
{
	ASSERT(!memory, "PoolAllocator second initialisation is not possible before reset function");
	ASSERT(size, "Tried to initialise PoolAllocator with size == 0, init failed");
	if (!memory && size)
	{
		allocator.init(size*typeSize + alignment);
		poolSize = size;
		this->typeSize = typeSize;
		memory = (uintptr_t)allocator.allocRawAligned(size, alignment);
		clear();
	}
}

void Pool::clear()
{
	ASSERT(memory, "Tried to set as free all allocated memory before initialisation (there is nothing to set as free)");
	if (memory)
	{
		for (size_t i = 0; i < poolSize; i++)
			*((uintptr_t*)(memory + (i * typeSize))) = i + 1;	// last one is set out of range on purpose (it says that there is no free index)
		inUseCount = 0;
		freeIndex = 0;
	}
}

void* Pool::allocRaw()
{
	ASSERT(memory, "PoolAllocator use before its initialisation");
	ASSERT((freeIndex < poolSize), "No free memory to allocate");

	if (!memory || freeIndex >= poolSize)
		return nullptr;

	void* allocatedObject = (void*)(memory + freeIndex * typeSize);
	ptrdiff_t currIndex = freeIndex;
	freeIndex = *((uintptr_t*)(memory + freeIndex * typeSize));
	inUseCount++;

	return allocatedObject;
}


void Pool::freeRaw(void* object)
{
	ASSERT(memory, "PoolAllocator use before its initialisation");
	ASSERT(contains(object), "Tried to free object that does not belong to pool");

	if (contains(object))
	{
		uintptr_t newIndex = ((uintptr_t)object - memory) / typeSize;
		*((uintptr_t*)(memory + newIndex * typeSize)) = freeIndex < poolSize ? freeIndex : poolSize;
		freeIndex = newIndex;
		inUseCount--;
	}
}

size_t Pool::freeCount() const
{
	return poolSize - inUseCount;
}


size_t Pool::size() const
{
	return poolSize;
}

void Pool::reset()
{
	if (!memory) // pool already reseted
		return;
	allocator.reset();

	poolSize = 0;
	typeSize = 0;
	inUseCount = 0;
	freeIndex = std::numeric_limits<uintptr_t>::max();
	memory = 0;
}

bool Pool::contains(void * ptr) const
{
	return (memory && (uintptr_t)ptr >= memory && (uintptr_t)ptr < memory + poolSize * typeSize);
}


Pool::~Pool()
{
	reset();
}