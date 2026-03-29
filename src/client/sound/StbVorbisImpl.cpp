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

extern "C" {

stb_vorbis* mc_stb_vorbis_open_memory(const unsigned char* data, int len, int* error)
{
	return stb_vorbis_open_memory(data, len, error, nullptr);
}

void mc_stb_vorbis_close(stb_vorbis* v)
{
	stb_vorbis_close(v);
}

int mc_stb_vorbis_get_info(stb_vorbis* v, unsigned int* sampleRate, int* channels)
{
	if (v == nullptr || sampleRate == nullptr || channels == nullptr) {
		return 0;
	}

	const stb_vorbis_info info = stb_vorbis_get_info(v);
	*sampleRate = info.sample_rate;
	*channels = info.channels;
	return 1;
}

int mc_stb_vorbis_stream_length_in_samples(stb_vorbis* v)
{
	return stb_vorbis_stream_length_in_samples(v);
}

int mc_stb_vorbis_get_samples_short_interleaved(
	stb_vorbis* v,
	int channels,
	short* output,
	int numSamples
)
{
	return stb_vorbis_get_samples_short_interleaved(v, channels, output, numSamples);
}

} // extern "C"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
