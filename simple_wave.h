#pragma once

/*
  SIMPLE WAVE
  Copyright (C) Christian Luppi

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  This is a simple wave library to parse and stream wave files.

  Do this:
      #define SIMPLE_WAVE_IMPLEMENTATION
   before you include this file in *one* C or C++ file to create the implementation.

   // i.e. it should look like this:
   #include ...
   #include ...
   #include ...
   #define SIMPLE_WAVE_IMPLEMENTATION
   #include "simple_wave.h"

   NOTE: This is not a full wav parser, only uncompressed PCM and FLOAT samples are supported.

*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(push, 1)

#define RIFF_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

    typedef struct RIFF_HEADER
    {
        uint32_t riff_id;
        uint32_t size;
        uint32_t filetype_id;
    } RIFF_HEADER;

    typedef struct RIFF_CHUNK
    {
        uint32_t id;
        uint32_t size;
    } RIFF_CHUNK;

    typedef enum WAVE_FORMAT_TAG
    {
        WAVE_FORMAT_TAG_PCM = 0x0001,
        WAVE_FORMAT_TAG_IEEE_FLOAT = 0x0003,
    } WAVE_FORMAT_TAG;

    typedef struct WAVE_FORMAT
    {
        uint16_t format_tag;
        uint16_t channels;
        uint32_t samples_per_sec;
        uint32_t avg_bytes_per_sec;
        uint16_t block_align;
        uint16_t bits_per_sample;
    } WAVE_FORMAT;

#pragma pack(pop)

    typedef enum WAVE_CHUNK
    {
        WAVE_CHUNK_FORMAT = RIFF_CODE('f', 'm', 't', ' '),
        WAVE_CHUNK_DATA = RIFF_CODE('d', 'a', 't', 'a'),
    } WAVE_CHUNK;

    typedef struct WAVE
    {
        void* free_ptr;
        size_t free_ptr_size;

        RIFF_HEADER* header;

        RIFF_CHUNK* format_chunk;
        size_t      format_chunk_offset;

        RIFF_CHUNK* data_chunk;
        size_t      data_chunk_offset;

        void* sample_data;
        size_t sample_data_size;
        size_t sample_data_offset;

        WAVE_FORMAT* format;
    } WAVE;

    typedef enum WAVE_SAMPLE_FORMAT
    {
        WAVE_SAMPLE_FORMAT_UNKNOWN,
        WAVE_SAMPLE_FORMAT_U8,
        WAVE_SAMPLE_FORMAT_S16,
        WAVE_SAMPLE_FORMAT_S32,
        WAVE_SAMPLE_FORMAT_F32,
        WAVE_SAMPLE_FORMAT_F64,
    } WAVE_SAMPLE_FORMAT;

    typedef void* WAVE_ALLOCATE(void* data, size_t size);
    typedef void WAVE_FREE(void* data, void* ptr, size_t size);

    typedef struct WAVE_ALLOCATOR
    {
        WAVE_ALLOCATE* allocate;
        WAVE_FREE*     free;
        void*          data;
    } WAVE_ALLOCATOR;
    
    /*
      Returns a parsed wave file if the result is not 0.
      This function does not allocate any memory,
      you must read the wave file yourself.
      The data stored in the wave structure is only valid if the given buffer is STILL allocated.
    */
    extern int Wave_ParseBuffer(void* buff, size_t size, WAVE* out_wave);

    /*
      Read a wav from the current file stream.
    */
    extern int Wave_LoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    /*
     Read a wav from a file path.
   */
    extern int Wave_LoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    /*
      Read a wav from the current file stream.
    */
    extern int Wave_LoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    /*
     Read a wav from a file path.
   */
    extern int Wave_LoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    /*
      Returns the sample format of the stored samples.
    */
    extern WAVE_SAMPLE_FORMAT Wave_GetSampleFormat(WAVE* wave);

    /*
      Returns the length of the stored samples in seconds.
    */
    extern float Wave_GetLengthInSeconds(WAVE* wave);

    /*
      Returns the number of samples per second.
    */
    extern int Wave_GetSampleFrequency(WAVE* wave);

    /*
      Returns the number of channels.
    */
    extern int Wave_GetChannelCount(WAVE* wave);

    /*
      Returns the sample data in the given pointers.
      Returns 0 if the function failed.
    */
    extern int Wave_GetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size);

    /*
      Returns the number of samples.
    */
    extern int Wave_GetSampleCount(WAVE* wave);

    /*
      Release internally allocated memory.
    */
    extern int Wave_Free(WAVE* wave, WAVE_ALLOCATOR* allocator);

    /*
      Returns the offset of the sample data in the parsed memory.
    */
    extern size_t Wave_GetSampleDataOffset(WAVE* wave);

    //
    //
    //
    //
    //
    //
    //
    //

#ifdef SIMPLE_WAVE_IMPLEMENTATION

    static int Wave_ValidateHeader(RIFF_HEADER* header)
    {
        // Check RIFF id
        if (header->riff_id != RIFF_CODE('R', 'I', 'F', 'F'))
            return 0;

        // Check WAV id
        if (header->filetype_id != RIFF_CODE('W', 'A', 'V', 'E'))
            return 0;

        return 1;
    }

    static int Wave_ValidateFormat(WAVE* out_wave)
    {
        if (!out_wave->format)
            return 0;

        if ((out_wave->format->format_tag != WAVE_FORMAT_TAG_PCM) && (out_wave->format->format_tag != WAVE_FORMAT_TAG_IEEE_FLOAT))
            return 0;

        if ((out_wave->format->format_tag == WAVE_FORMAT_TAG_PCM) && (out_wave->format->bits_per_sample != 8) && (out_wave->format->bits_per_sample != 16) && (out_wave->format->bits_per_sample != 32))
            return 0;

        if ((out_wave->format->format_tag == WAVE_FORMAT_TAG_IEEE_FLOAT) && (out_wave->format->bits_per_sample != 32) && (out_wave->format->bits_per_sample != 64))
            return 0;

        return 1;
    }

    static void* Wave_DefaultAlloc(void* data, size_t size)
    {
        return malloc(size);
    }

    static void Wave_DefaultFree(void* data, void* ptr, size_t size)
    {
        free(ptr);
    }

    static WAVE_ALLOCATOR* Wave_GetDefaultAllocator(void)
    {
        static WAVE_ALLOCATOR allocator = { 0 };
        allocator.allocate = Wave_DefaultAlloc;
        allocator.free     = Wave_DefaultFree;
        return &allocator;
    }

    int Wave_ParseBuffer(void* buff, size_t size, WAVE* out_wave)
    {
        // Validate params
        if ((!buff) || (!size) || (!out_wave))
            return 0;

        memset(out_wave, 0, sizeof(WAVE));

        out_wave->header = (RIFF_HEADER*)buff;
        if (!Wave_ValidateHeader(out_wave->header))
            return 0;

        // Parse RIFF chunks
        char* at = (char*)(out_wave->header + 1);
        char* max = (char*)(at + out_wave->header->size - 4);
        while (at < max)
        {
            RIFF_CHUNK* chunk = (RIFF_CHUNK*)at;

            switch (chunk->id)
            {
                case WAVE_CHUNK_DATA:
                    out_wave->data_chunk = chunk;
                    out_wave->data_chunk_offset = (size_t)((char*)chunk - (char*)buff);
                    break;
                case WAVE_CHUNK_FORMAT:
                    out_wave->format_chunk = chunk;
                    out_wave->format_chunk_offset = (size_t)((char*)chunk - (char*)buff);
                    break;
            }

            at += sizeof(RIFF_CHUNK) + ((chunk->size + 1) & ~1); // If the size is odd round it
        }

        // Get format
        if (!out_wave->format_chunk)
            return 0;
        out_wave->format = (WAVE_FORMAT*)(out_wave->format_chunk + 1);

        // Get sample data
        if (out_wave->data_chunk)
        {
            out_wave->sample_data = (void*)(out_wave->data_chunk + 1);
            out_wave->sample_data_size = out_wave->data_chunk->size;
            out_wave->sample_data_offset = (size_t)((char*)out_wave->sample_data - (char*)buff);
        }

        return Wave_ValidateFormat(out_wave);
    }

    int Wave_LoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator)
    {
        if (!out_wave)
            return 0;
        if (!file)
            return 0;

        if (!allocator)
            allocator = Wave_GetDefaultAllocator();

        void* buff = allocator->allocate(allocator->data, size);
        fread(buff, 1, size, file);
        out_wave->free_ptr = buff;
        out_wave->free_ptr_size = size;
        return Wave_ParseBuffer(buff, size, out_wave);
    }

    int Wave_LoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator)
    {
        if (!out_wave)
            return 0;
        if (!path)
            return 0;

        FILE* file = fopen(path, "rb");
        if (file)
        {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            Wave_LoadStream(file, fileSize, out_wave, allocator);
            fclose(file);
        }

        return 1;
    }

    int Wave_LoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator)
    {
        if (!file)
            return 0;
        if (!out_wave)
            return 0;
        if (!allocator)
            allocator = Wave_GetDefaultAllocator();

        memset(out_wave, 0, sizeof(WAVE));

        // Allocate space
        out_wave->free_ptr_size = sizeof(RIFF_HEADER) + sizeof(RIFF_CHUNK) * 2 + sizeof(WAVE_FORMAT);
        out_wave->free_ptr = allocator->allocate(allocator->data, out_wave->free_ptr_size);

        // Allocate space for the header
        out_wave->header = (RIFF_HEADER*)out_wave->free_ptr;
        fread(out_wave->header, sizeof(RIFF_HEADER), 1, file);

        // Validate the header
        if (!Wave_ValidateHeader(out_wave->header))
            return 0;

        // Initialize variables for chunk processing
        RIFF_CHUNK chunk;

        // Read chunks until the 'fmt ' chunk is found
        while ((ftell(file) + sizeof(RIFF_CHUNK)) <= size)
        {
            // Read the chunk header
            fread(&chunk, sizeof(RIFF_CHUNK), 1, file);

            if (chunk.id == WAVE_CHUNK_DATA)
            {
                out_wave->data_chunk = (RIFF_CHUNK*)(out_wave->header+1);
                memcpy(out_wave->data_chunk, &chunk, sizeof(RIFF_CHUNK));
                out_wave->data_chunk_offset = ftell(file) - sizeof(RIFF_CHUNK);
                out_wave->sample_data_offset = ftell(file);
                out_wave->sample_data_size = chunk.size;

                fseek(file, chunk.size, SEEK_CUR);
            }
            else if (chunk.id == WAVE_CHUNK_FORMAT)
            {
                out_wave->format_chunk = (RIFF_CHUNK*)((char*)(out_wave->header+1) + sizeof(RIFF_CHUNK));
                memcpy(out_wave->format_chunk, &chunk, sizeof(RIFF_CHUNK));
                out_wave->format_chunk_offset = ftell(file) - sizeof(RIFF_CHUNK);

                out_wave->format = (WAVE_FORMAT*)((char*)(out_wave->header + 1) + sizeof(RIFF_CHUNK) * 2);
                fread(out_wave->format, 1, chunk.size, file);
            }
            else
            {
                // Skip over this chunk
                fseek(file, chunk.size, SEEK_CUR);
            }

            if (chunk.size % 2 != 0)
                fseek(file, 1, SEEK_CUR);
        }

        // Validate the format chunk
        if (!Wave_ValidateFormat(out_wave))
            return 0;

        return 1;
    }

    int Wave_LoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator)
    {
        if (!path)
            return 0;
        if (!out_wave)
            return 0;

        FILE* file = fopen(path, "rb");
        if (file)
        {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            Wave_LoadStreamOnlyInfo(file, fileSize, out_wave, allocator);
            fclose(file);
        }

        return 1;
    }

    WAVE_SAMPLE_FORMAT Wave_GetSampleFormat(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return WAVE_SAMPLE_FORMAT_UNKNOWN;

        if (wave->format->format_tag == WAVE_FORMAT_TAG_PCM)
        {
            if (wave->format->bits_per_sample == 8)
                return WAVE_SAMPLE_FORMAT_U8;

            if (wave->format->bits_per_sample == 16)
                return WAVE_SAMPLE_FORMAT_S16;

            if (wave->format->bits_per_sample == 32)
                return WAVE_SAMPLE_FORMAT_S32;
        }

        if (wave->format->format_tag == WAVE_FORMAT_TAG_IEEE_FLOAT)
        {
            if (wave->format->bits_per_sample == 32)
                return WAVE_SAMPLE_FORMAT_F32;

            if (wave->format->bits_per_sample == 64)
                return WAVE_SAMPLE_FORMAT_F64;
        }

        return WAVE_SAMPLE_FORMAT_UNKNOWN;
    }

    float Wave_GetLengthInSeconds(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return ((float)wave->sample_data_size / (wave->format->bits_per_sample / 8)) / (float)wave->format->samples_per_sec;
    }

    int Wave_GetSampleFrequency(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return wave->format->samples_per_sec;
    }

    int Wave_GetChannelCount(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return wave->format->channels;
    }

    int Wave_GetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size)
    {
        if(!wave)
            return 0;
        if (out_samples)
            *out_samples = wave->sample_data;
        if (out_samples_size)
            *out_samples_size = wave->sample_data_size;
        return 1;
    }

    int Wave_GetSampleCount(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return (int)(wave->sample_data_size / (wave->format->bits_per_sample / 8));
    }

    int Wave_Free(WAVE* wave, WAVE_ALLOCATOR* allocator)
    {
        if(!wave)
            return 0;
        if(!allocator)
            allocator = Wave_GetDefaultAllocator();

        if (wave->free_ptr)
        {
            allocator->free(allocator->data, wave->free_ptr, wave->free_ptr_size);
            wave->free_ptr      = NULL;
            wave->free_ptr_size = 0;
            return 1;
        }

        return 0;
    }

    size_t Wave_GetSampleDataOffset(WAVE* wave)
    {
        if(!wave)
            return 0;

        return wave->sample_data_offset;
    }

#endif // SIMPLE_WAVE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif
