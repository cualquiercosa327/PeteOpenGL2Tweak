#pragma once

#include <windows.h>

template<typename T>
inline bool SafeWrite(void* addr, const T data)
{
    DWORD oldProtect;
    if (VirtualProtect(addr, sizeof(T), PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        *((T*)addr) = data;
        VirtualProtect(addr, sizeof(T), oldProtect, &oldProtect);
        return true;
    }
    return false;
}

inline bool SafeWriteBuf(void* addr, const unsigned char* data, size_t size)
{
	DWORD oldProtect;
	if (VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		memcpy(addr, data, size);
		VirtualProtect(addr, size, oldProtect, &oldProtect);
		return true;
	}
	return false;
}

