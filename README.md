This is a very simple library to load and parse WAV files.

This library supports uncompressed PCM Uint8, Sint16, Sint32, Float32, Float64.

    int                WaveParseBuffer(void* buff, size_t size, WAVE* out_wave);
    int                WaveLoadStream(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate);
    int                WaveLoadPath(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate);
    int                WaveLoadStreamOnlyInfo(FILE* file, long size, WAVE* out_wave, WAVE_ALLOCATE* allocate);
    int                WaveLoadPathOnlyInfo(const char* path, WAVE* out_wave, WAVE_ALLOCATE* allocate);
    WAVE_SAMPLE_FORMAT WaveGetSampleFormat(WAVE* wave);
    float              WaveGetLengthInSeconds(WAVE* wave);
    int                WaveGetSampleFrequency(WAVE* wave);
    int                WaveGetChannelCount(WAVE* wave);
    int                WaveGetSampleData(WAVE* wave, void** out_samples, size_t* out_samples_size);
    int                WaveGetSampleCount(WAVE* wave);
