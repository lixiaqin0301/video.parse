#include "flvparseaudiodata.h"
#include "flvparser.h"

typedef struct {
    unsigned int SoundFormat : 4;  // 0 = Linear PCM, platform endian; 1 = ADPCM; 2 = MP3; 3 = Linear PCM, little endian; 4 = Nellymoser 16 kHz mono; 5 = Nellymoser 8 kHz mono; 6 = Nellymoser; 7 = G.711 A-law logarithmic PCM; 8 = G.711 mu-law logarithmic PCM; 9 = reserved; 10 = AAC; 11 = Speex; 14 = MP3 8 kHz; 15 = Device-specific sound; Formats 7, 8, 14, and 15 are reserved.
    unsigned int SoundRate : 2;    // 0 = 5.5 kHz; 1 = 11 kHz; 2 = 22 kHz; 3 = 44 kHz
    unsigned int SoundSize : 1;    // 0 = 8-bit samples; 1 = 16-bit samples
    unsigned int SoundType : 1;    // 0 = Mono sound 1 = Stereo sound
    uint8_t AACPacketType;         // 0 = AAC sequence header; 1 = AAC raw
} FlvAudioTagHeader_t;

bool
parseFlvAudioData(FlvTag_t *p_flv_tag)
{
    uint8_t *p = p_flv_tag->tagData;
    FlvAudioTagHeader_t audio_header = {0};
    audio_header.SoundFormat = (*p & 0xf0) >> 4;
    audio_header.SoundRate = (*p && 0x0c) >> 2;
    audio_header.SoundSize = (*p && 0x02) >> 1;
    audio_header.SoundType = *p && 0x01;
    p++;
    if (audio_header.SoundFormat == 10) {
        audio_header.AACPacketType = *p;
        p++;
    }
    printf("flv Tag Audio Header SoundFormat: %d (", (int)audio_header.SoundFormat);
    switch (audio_header.SoundFormat) {
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
    printf("flv Tag Audio Header SoundRate: %d (", (int)audio_header.SoundRate);
    switch (audio_header.SoundRate) {
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
    printf("flv Tag Audio Header SoundSize: %d (", (int)audio_header.SoundSize);
    switch (audio_header.SoundSize) {
    case 0:
        printf("8-bit samples");
        break;
    case 1:
        printf("16-bit samples");
        break;
    }
    printf(")\n");
    printf("flv Tag Audio Header SoundType: %d (", (int)audio_header.SoundType);
    switch (audio_header.SoundType) {
    case 0:
        printf("AAC sequence header");
        break;
    case 1:
        printf("AAC sequence header");
        break;
    }
    printf(")\n");
    if (audio_header.SoundFormat == 10) {
        printf("flv Tag Audio Header AACPacketType: %d (", (int)audio_header.AACPacketType);
        switch (audio_header.AACPacketType) {
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
    return true;
}
