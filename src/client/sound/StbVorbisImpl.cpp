/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

// stb_vorbis implementation file
// This file provides the stb_vorbis implementation to avoid conflicts with fmt library

// Disable MSVC warnings for third-party library
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4701) // Potentially uninitialized local variable
#pragma warning(disable : 4703) // Potentially uninitialized local pointer variable
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

int mc_stb_vorbis_get_samples_short_interleaved(stb_vorbis* v, int channels, short* output, int numSamples)
{
    return stb_vorbis_get_samples_short_interleaved(v, channels, output, numSamples);
}

} // extern "C"

#ifdef _MSC_VER
#pragma warning(pop)
#endif
