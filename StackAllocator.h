#pragma once
#include <stdint.h>

class StackAllocator
{
	typedef uintptr_t Marker;

public:
	explicit StackAllocator(size_t sizeInBytes);
	StackAllocator() = default;
	StackAllocator(const StackAllocator & lref) = delete;
	StackAllocator(StackAllocator&& rref) = delete;
	StackAllocator& operator=(const StackAllocator & lref) = delete;
	StackAllocator& operator=(StackAllocator&& lref) = delete;

	~StackAllocator();

	void init(size_t sizeInBytes);

	void* allocRawUnaligned(size_t sizeInBytes);
	//  "alignment" must be a power of 2
	void* allocRawAligned(size_t sizeInBytes, uint8_t alignment);

	// allocates aligned memory and calls default constructor of given type using placement new, must be freed by freeToObject, 
	// or call destructor by yourself when you dont need this object anymore and stack's data fragmentation is not a problem
	template<class T, class ...Args>
	T* allocObject(Args... args);

	Marker getMarker() const;

	template<class T>
	void freeToRawUnaligned(T* marker);
	template<class T>
	void freeToRawAligned(T* marker);

	// sets stack top to marker and calls destructor on given object
	template<class T>
	void freeToObject(T* marker);

	// sets stack's top marker to bottom
	void clear();
	
	// resets stack size to 0, deletes allocated memory
	void reset();

	bool isValid(void* ptr) const;

private:
	Marker topMarker = 0;
	Marker bottomMarker = 0;
	size_t sizeInBytes = 0;
};

template<class T, class ...Args>
inline T * StackAllocator::allocObject(Args ...args)
{
	T* ptr = (T*)allocRawAligned(sizeof(T), alignof(T));
	if (ptr)
		new ((void*)ptr) T(std::forward<Args>(args)...);
	ASSERT(ptr, "No memory was allocated to construct object, returned nullptr");
	return ptr;
}

template<class T>
inline void StackAllocator::freeToRawUnaligned(T * marker)
{
	ASSERT(marker, "Tried to free memory to a nullptr");
	if (marker)
	{
		uintptr_t mark = (uintptr_t)marker;
		ASSERT((mark < topMarker), "Tried to free memory to a marker that lies further than current stack's top marker");

		if (mark < topMarker)
			topMarker = mark;
	}
}

template<class T>
inline void StackAllocator::freeToRawAligned(T * marker)
{
	ASSERT(marker, "Tried to free memory to a nullptr");
	if (marker)
	{
		uintptr_t mark = (uintptr_t)marker;
		uint8_t adjustment = *((uint8_t*)(mark - 1));
		freeToRawUnaligned((T*)(mark - adjustment));
	}
}

template<class T>
inline void StackAllocator::freeToObject(T * marker)
{
	if (marker)
	{
		marker->~T();
		freeToRawAligned(marker);
	}
}