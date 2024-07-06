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

#ifdef __cplusplus
extern "C" {
#endif

#define RIFF_CODE(a, b, c, d) (((uint32_t)(a) << 0) | ((uint32_t)(b) << 8) | ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#pragma pack(push, 1)

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

    // This is a custom allocator to acquire memory.
    // Free is not available because this was designed with an arena allocator in mind.
    // If you need to individually free the allocations, create an internal list and free them yourself.
    typedef void* WAVE_ALLOCATE(size_t size);

    /*
      Returns a parsed wave file if the result is not 0.
      This function does not allocate any memory,
      you must read the wave file yourself.
      The data stored in the wave structure is only valid if the given buffer is STILL allocated.
    */
    extern int WaveParseBuffer(void* buff, size_t size, WAVE* out_wave);

    /*
      Read a wav from the current file stream.
      All allocations use WAVE_ALLOCATE().
      If WAVE_ALLOCATE is NULL, malloc will be used.
    */
    extern int WaveLoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate);

    /*
     Read a wav from a file path.
     All allocations use WAVE_ALLOCATE().
      If WAVE_ALLOCATE is NULL, malloc will be used.
   */
    extern int WaveLoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate);

    /*
      Read a wav from the current file stream.
      All allocations use WAVE_ALLOCATE().
      If WAVE_ALLOCATE is NULL, malloc will be used.
    */
    extern int WaveLoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate);

    /*
     Read a wav from a file path.
     All allocations use WAVE_ALLOCATE().
      If WAVE_ALLOCATE is NULL, malloc will be used.
   */
    extern int WaveLoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate);

    /*
      Returns the sample format of the stored samples.
    */
    extern WAVE_SAMPLE_FORMAT WaveGetSampleFormat(WAVE* wave);

    /*
      Returns the length of the stored samples in seconds.
    */
    extern float WaveGetLengthInSeconds(WAVE* wave);

    /*
      Returns the number of samples per second.
    */
    extern int WaveGetSampleFrequency(WAVE* wave);

    /*
      Returns the number of channels.
    */
    extern int WaveGetChannelCount(WAVE* wave);

    /*
      Returns the sample data in the given pointers.
      Returns 0 if the function failed.
    */
    extern int WaveGetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size);

    /*
      Returns the number of samples.
    */
    extern int WaveGetSampleCount(WAVE* wave);

    //
    //
    //
    //
    //
    //
    //
    //

#ifdef SIMPLE_WAVE_IMPLEMENTATION

    static int WaveValidateHeader(RIFF_HEADER* header)
    {
        // Check RIFF id
        if (header->riff_id != RIFF_CODE('R', 'I', 'F', 'F'))
            return 0;

        // Check WAV id
        if (header->filetype_id != RIFF_CODE('W', 'A', 'V', 'E'))
            return 0;

        return 1;
    }

    static int WaveValidateFormat(WAVE* out_wave)
    {
        if(!out_wave->format)
            return 0;

        if ((out_wave->format->format_tag != WAVE_FORMAT_TAG_PCM) && (out_wave->format->format_tag != WAVE_FORMAT_TAG_IEEE_FLOAT))
            return 0;

        if((out_wave->format->format_tag == WAVE_FORMAT_TAG_PCM) && (out_wave->format->bits_per_sample != 8) && (out_wave->format->bits_per_sample != 16) && (out_wave->format->bits_per_sample != 32))
            return 0;
        
        if ((out_wave->format->format_tag == WAVE_FORMAT_TAG_IEEE_FLOAT) && (out_wave->format->bits_per_sample != 32) && (out_wave->format->bits_per_sample != 64))
            return 0;

        return 1;
    }

    int WaveParseBuffer(void* buff, size_t size, WAVE* out_wave)
    {
        // Validate params
        if ((!buff) || (!size) || (!out_wave))
            return 0;

        memset(out_wave, 0, sizeof(WAVE));

        out_wave->header = (RIFF_HEADER*)buff;
        if(!WaveValidateHeader(out_wave->header))
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

        return WaveValidateFormat(out_wave);
    }

    int WaveLoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate)
    {
        if (!out_wave)
            return 0;
        if(!file)
            return 0;
        
        if(!allocate)
            allocate = malloc;

        void* buff = allocate(size);
        fread(buff, 1, size, file);
        return WaveParseBuffer(buff, size, out_wave);
    }

    int WaveLoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate)
    {
        if (!out_wave)
            return 0;
        if(!path)
            return 0;

        FILE* file = fopen(path, "rb");
        if (file)
        {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            WaveLoadStream(file, fileSize, out_wave, allocate);
            fclose(file);
        }

        return 1;
    }

    int WaveLoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate)
    {
        if (!file)
            return 0;
        if (!out_wave)
            return 0;
        if (!allocate)
            allocate = malloc;

        memset(out_wave, 0, sizeof(WAVE));

        // Allocate space for the header
        out_wave->header = (RIFF_HEADER*)allocate(sizeof(RIFF_HEADER));
        fread(out_wave->header, sizeof(RIFF_HEADER), 1, file);

        // Validate the header
        if (!WaveValidateHeader(out_wave->header))
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
                out_wave->data_chunk = (RIFF_CHUNK*)allocate(sizeof(RIFF_CHUNK));
                memcpy(out_wave->data_chunk, &chunk, sizeof(RIFF_CHUNK));
                out_wave->data_chunk_offset = ftell(file) - sizeof(RIFF_CHUNK);
                out_wave->sample_data_offset = ftell(file);
                out_wave->sample_data_size = chunk.size;
                
                fseek(file, chunk.size, SEEK_CUR);
            }
            else if (chunk.id == WAVE_CHUNK_FORMAT)
            {
                out_wave->format_chunk = (RIFF_CHUNK*)allocate(sizeof(RIFF_CHUNK));
                memcpy(out_wave->format_chunk, &chunk, sizeof(RIFF_CHUNK));
                out_wave->format_chunk_offset = ftell(file) - sizeof(RIFF_CHUNK);

                out_wave->format = (WAVE_FORMAT*)allocate(chunk.size);
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
        if (!WaveValidateFormat(out_wave))
            return 0;

        return 1;
    }

    int WaveLoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate)
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

            WaveLoadStreamOnlyInfo(file, fileSize, out_wave, allocate);
            fclose(file);
        }

        return 1;
    }

    WAVE_SAMPLE_FORMAT WaveGetSampleFormat(WAVE* wave)
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

    float WaveGetLengthInSeconds(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return ((float)wave->sample_data_size / (wave->format->bits_per_sample / 8)) / (float)wave->format->samples_per_sec;
    }

    int WaveGetSampleFrequency(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return wave->format->samples_per_sec;
    }

    int WaveGetChannelCount(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return wave->format->channels;
    }

    int WaveGetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size)
    {
        if (out_samples)
            *out_samples = wave->sample_data;
        if (out_samples_size)
            *out_samples_size = wave->sample_data_size;
        return 1;
    }

    int WaveGetSampleCount(WAVE* wave)
    {
        if ((!wave) || (!wave->format))
            return 0;

        return (int)(wave->sample_data_size / (wave->format->bits_per_sample / 8));
    }

#endif // SIMPLE_WAVE_IMPLEMENTATION

#ifdef __cplusplus
}
#endif