This is a very simple library to load and parse WAV files.

This library supports uncompressed PCM Uint8, Sint16, Sint32, Float32, Float64.

```c
    // Directly parse wave from a memory buffer
    extern int Wave_ParseBuffer(void* buff, size_t size, WAVE* out_wave);

    // Parse WAV from file (FULL FILE)
    extern int Wave_LoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator);
    extern int Wave_LoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    // Parse WAV from file (ONLY INFO, USEFUL FOR STREAMING)
    extern int Wave_LoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATOR* allocator);
    extern int Wave_LoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATOR* allocator);

    // Get various parsed WAV properties 
    extern WAVE_SAMPLE_FORMAT Wave_GetSampleFormat(WAVE* wave);
    extern float Wave_GetLengthInSeconds(WAVE* wave);
    extern int Wave_GetSampleFrequency(WAVE* wave);
    extern int Wave_GetChannelCount(WAVE* wave);
    extern int Wave_GetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size);
    extern int Wave_GetSampleCount(WAVE* wave);
    extern int Wave_Free(WAVE* wave, WAVE_ALLOCATOR* allocator);
    extern size_t Wave_GetSampleDataOffset(WAVE* wave);
```
