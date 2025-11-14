#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include "flutter_meter.h"

/**
 * @brief RIFF/WAV file main header.
 */
typedef struct
{
    char riff[4];        // Should contain "RIFF"
    uint32_t chunkSize;
    char wave[4];        // Should contain "WAVE"
} RIFFHeader;

/**
 * @brief "fmt " subchunk describing audio format.
 */
typedef struct
{
    char fmt[4];         // Should contain "fmt "
    uint32_t subChunk1Size;
    uint16_t audioFormat;     // PCM = 1
    uint16_t numChannels;     // 1 = mono, 2 = stereo
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;   // Typically 16 for PCM
} FMTSubchunk;

/**
 * @brief "data" subchunk header describing raw sample data.
 */
typedef struct
{
    char data[4];        // Should contain "data"
    uint32_t dataSize;   // Size of the sample data in bytes
} DataSubchunk;

int main(void)
{
    const char *filename = "test1.wav";
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        perror("Failed to open file");
        return 1;
    }

    // Read RIFF header
    RIFFHeader riff;
    fread(&riff, sizeof(RIFFHeader), 1, fp);

    // Read FMT header
    FMTSubchunk fmt;
    fread(&fmt, sizeof(FMTSubchunk), 1, fp);

    // Basic format validation
    if (fmt.audioFormat != 1)
    {
        printf("File is not PCM format!\n");
        return 1;
    }

    if (fmt.bitsPerSample != 16)
    {
        printf("Only 16-bit PCM is supported!\n");
        return 1;
    }

    // Search for the "data" subchunk
    DataSubchunk dataChunk;

    while (fread(&dataChunk, sizeof(DataSubchunk), 1, fp) == 1)
    {
        if (memcmp(dataChunk.data, "data", 4) == 0)
        {
            break;   // "data" subchunk found
        }
        else
        {
            // Skip unknown chunk payload
            fseek(fp, dataChunk.dataSize, SEEK_CUR);
        }
    }

    // Calculate number of samples (only first channel is read later)
    int numSamples =
        dataChunk.dataSize /
        (fmt.bitsPerSample / 8) /
        fmt.numChannels;

    int *samples = malloc(sizeof(int) * numSamples);
    if (!samples)
    {
        perror("Memory allocation error");
        return 1;
    }

    // Read audio samples (only channel 0 is used)
    for (int i = 0; i < numSamples; i++)
    {
        int16_t val;
        fread(&val, sizeof(int16_t), 1, fp);  // Read first channel
        samples[i] = (int)val;

        // Skip second channel in stereo files
        if (fmt.numChannels == 2)
        {
            fseek(fp, sizeof(int16_t), SEEK_CUR);
        }
    }

    fclose(fp);

    // Initialize flutter meter
    flutterMeter_init(fmt.sampleRate, 3150);

    // Process data
    int ret = process_samples(samples, numSamples, 1);
    if (ret != 0)
    {
        printf("process_samples returned an error: %d\n", ret);
    }

    // Retrieve results
    double peak, rms, freq;
    get_results(&peak, &rms, &freq);

    printf("\nRMS:  %.4f\nPeak: %.4f\nFreq: %.2f Hz\n",
           rms, peak, freq);

    free(samples);
    return 0;
}
