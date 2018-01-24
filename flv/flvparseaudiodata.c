#include "flvparseaudiodata.h"
#include "flvparser.h"

typedef struct {
    unsigned int SoundFormat : 4;  // 0 = Linear PCM, platform endian; 1 = ADPCM; 2 = MP3; 3 = Linear PCM, little endian; 4 = Nellymoser 16 kHz mono; 5 = Nellymoser 8 kHz mono; 6 = Nellymoser; 7 = G.711 A-law logarithmic PCM; 8 = G.711 mu-law logarithmic PCM; 9 = reserved; 10 = AAC; 11 = Speex; 14 = MP3 8 kHz; 15 = Device-specific sound; Formats 7, 8, 14, and 15 are reserved.
    unsigned int SoundRate : 2;    // 0 = 5.5 kHz; 1 = 11 kHz; 2 = 22 kHz; 3 = 44 kHz
    unsigned int SoundSize : 1;    // 0 = 8-bit samples; 1 = 16-bit samples
    unsigned int SoundType : 1;    // 0 = Mono sound 1 = Stereo sound
    uint8_t AACPacketType;         // 0 = AAC sequence header; 1 = AAC raw
} FlvAudioTagHeader_t;

static void
printFlvAudioData(const FlvAudioTagHeader_t *p_audioHeader)
{
    printf("flv Tag Audio Header SoundFormat: %d (", (int)p_audioHeader->SoundFormat);
    switch (p_audioHeader->SoundFormat) {
    case 0:
        printf("Linear PCM, platform endian");
        break;
    case 1:
        printf("ADPCM");
        break;
    case 2:
        printf("MP3");
        break;
    case 3:
        printf("Linear PCM, little endian");
        break;
    case 4:
        printf("Nellymoser 16 kHz mono");
        break;
    case 5:
        printf("Nellymoser 8 kHz mono");
        break;
    case 6:
        printf("Nellymoser");
        break;
    case 7:
        printf("G.711 A-law logarithmic PCM");
        break;
    case 8:
        printf("G.711 mu-law logarithmic PCM");
        break;
    case 10:
        printf("AAC");
        break;
    case 11:
        printf("Speex");
        break;
    case 14:
        printf("MP3 8 kHz");
        break;
    case 15:
        printf("Device-specific sound");
        break;
    default:
        printf("reserved");
        break;
    }
    printf(")\n");
    printf("flv Tag Audio Header SoundRate: %d (", (int)p_audioHeader->SoundRate);
    switch (p_audioHeader->SoundRate) {
    case 0:
        printf("5.5 kHz");
        break;
    case 1:
        printf("11 kHz");
        break;
    case 2:
        printf("22 kHz");
        break;
    case 3:
        printf("44 kHz");
        break;
    }
    printf(")\n");
    printf("flv Tag Audio Header SoundSize: %d (", (int)p_audioHeader->SoundSize);
    switch (p_audioHeader->SoundSize) {
    case 0:
        printf("8-bit samples");
        break;
    case 1:
        printf("16-bit samples");
        break;
    }
    printf(")\n");
    printf("flv Tag Audio Header SoundType: %d (", (int)p_audioHeader->SoundType);
    switch (p_audioHeader->SoundType) {
    case 0:
        printf("AAC sequence header");
        break;
    case 1:
        printf("AAC sequence header");
        break;
    }
    printf(")\n");
    if (p_audioHeader->SoundFormat == 10) {
        printf("flv Tag Audio Header AACPacketType: %d (", (int)p_audioHeader->AACPacketType);
        switch (p_audioHeader->AACPacketType) {
        case 0:
            printf("AAC sequence header");
            break;
        case 1:
            printf("AAC raw");
            break;
        }
        printf(")\n");
    }
    printf("\n");
}

bool
parseFlvAudioData(const uint8_t *buf)
{
    FlvAudioTagHeader_t audio_header = {0};
    audio_header.SoundFormat = (buf[0] & 0xf0) >> 4;
    audio_header.SoundRate = (buf[0] && 0x0c) >> 2;
    audio_header.SoundSize = (buf[0] && 0x02) >> 1;
    audio_header.SoundType = buf[0] && 0x01;
    if (audio_header.SoundFormat == 10) {
        audio_header.AACPacketType = buf[1];
    }
    printFlvAudioData(&audio_header);
    return true;
}
