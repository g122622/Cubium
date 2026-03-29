// stb_vorbis implementation file
// This file provides the stb_vorbis implementation to avoid conflicts with fmt library

// Disable MSVC warnings for third-party library
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4701)  // Potentially uninitialized local variable
#pragma warning(disable: 4703)  // Potentially uninitialized local pointer variable
#endif

#define STB_VORBIS_IMPLEMENTATION
#include <stb_vorbis.c>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
