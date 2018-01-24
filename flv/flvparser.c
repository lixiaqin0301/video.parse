#include "flvparser.h"
#include "flvparsescriptdata.h"
#include "flvparseaudiodata.h"
#include "flvparsevideodata.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIOUS_TAG_SIZE_LEN 4
#define FLV_HEAD_LEN 9
#define TAG_HEAD_LEN 11

// read Previous Tag Size
static bool
readFlvPreviousTagSize(FILE *fp, FlvTag_t *p_flv_tag)
{
    uint8_t previousTagSizeBuf[PREVIOUS_TAG_SIZE_LEN] = {0};
    size_t readBytes = fread(previousTagSizeBuf, sizeof(uint8_t), PREVIOUS_TAG_SIZE_LEN, fp);
    if (readBytes != PREVIOUS_TAG_SIZE_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, PREVIOUS_TAG_SIZE_LEN, strerror(errno));
        return false;
    }
    p_flv_tag->PreviousTagSize = previousTagSizeBuf[3] | previousTagSizeBuf[2] << 8 | previousTagSizeBuf[1] << 16 | previousTagSizeBuf[0] << 24;
    return true;
}

// read Tag Header
static bool
readFlvTagHeader(FILE *fp, FlvTag_t *p_flv_tag)
{
    unsigned char tagHeader[TAG_HEAD_LEN] = {0};
    size_t readBytes = fread(tagHeader, sizeof(unsigned char), TAG_HEAD_LEN, fp);
    if (readBytes != TAG_HEAD_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, TAG_HEAD_LEN, strerror(errno));
        return false;
    }
    p_flv_tag->tagHeader.Reserved = (tagHeader[0] & 0xc0) >> 6;
    p_flv_tag->tagHeader.Filter = (tagHeader[0] & 0x20) >> 5;
    p_flv_tag->tagHeader.TagType = tagHeader[0] & 0x1f;
    p_flv_tag->tagHeader.DataSize = tagHeader[3] | tagHeader[2] << 8 | tagHeader[1] << 16;
    p_flv_tag->tagHeader.TimeStamp = tagHeader[6] | tagHeader[5] << 8 | tagHeader[4] << 16 | tagHeader[7] << 24;
    p_flv_tag->tagHeader.StreamID = tagHeader[8] | tagHeader[9] << 8 | tagHeader[10] << 16;

    switch (p_flv_tag->tagHeader.TagType) {
    case 0x12:  // scripit data
    case 0x09:  // video
    case 0x08:  // audeo
        return true;
    default:
        return false;
    }
}

bool
parseFlvHeader(FILE *fp, FlvHeader_t *p_flvHeader)
{
    unsigned char buf[FLV_HEAD_LEN] = {0};
    size_t readBytes = fread(buf, sizeof(unsigned char), FLV_HEAD_LEN, fp);
    if (readBytes != FLV_HEAD_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu < %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, FLV_HEAD_LEN, strerror(errno));
        return false;
    }
    memset(p_flvHeader, 0, sizeof(*p_flvHeader));
    p_flvHeader->Signature[0] = buf[0];
    p_flvHeader->Signature[1] = buf[1];
    p_flvHeader->Signature[2] = buf[2];
    p_flvHeader->Version = buf[3];
    p_flvHeader->TypeFlagsReserved1 = (buf[4] & 0xf8) >> 3;
    p_flvHeader->TypeFlagsAudio = (buf[4] & 0x04) >> 2;
    p_flvHeader->TypeFlagsReserved2 = (buf[4] & 0x02) >> 1;
    p_flvHeader->TypeFlagsVideo = buf[4] & 0x01;
    p_flvHeader->DataOffset = buf[8] | buf[7] << 8 | buf[6] << 16 | buf[5] << 24;

    if (p_flvHeader->Signature[0] != 'F' || p_flvHeader->Signature[1] != 'L' || p_flvHeader->Signature[2] != 'V' || p_flvHeader->TypeFlagsReserved1 != 0 || p_flvHeader->TypeFlagsReserved2 != 0) {
        fprintf(stderr, "%s:%d %s Not a FLV file!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }
    printf("flv Header Signature: %c%c%c\n", p_flvHeader->Signature[0], p_flvHeader->Signature[1], p_flvHeader->Signature[2]);
    printf("flv Header Version: %hhd\n", p_flvHeader->Version);
    printf("flv Header TypeFlagsAudio: %d\n", (int)p_flvHeader->TypeFlagsAudio);
    printf("flv Header TypeFlagsVideo: %d\n", (int)p_flvHeader->TypeFlagsVideo);
    printf("flv Header DataOffset: %u\n", p_flvHeader->DataOffset);
    printf("\n");
    return true;
}

bool
parseFlvTag(FILE *fp, FlvTag_t *p_flvTag)
{
    bool success = false;

    printf("%ld:\n", ftell(fp));
    success = readFlvPreviousTagSize(fp, p_flvTag);
    if (success) {
        printf("flv Previous Tag Size: %u\n", p_flvTag->PreviousTagSize);
        printf("\n");
    } else {
        fprintf(stderr, "%s:%d %s readFlvTagHeader failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    printf("%ld:\n", ftell(fp));
    success = readFlvTagHeader(fp, p_flvTag);
    if (success) {
        printf("flv Tag Header Filter: %u\n", p_flvTag->tagHeader.Filter);
        printf("flv Tag Header Type: 0x%x (", p_flvTag->tagHeader.TagType);
        switch (p_flvTag->tagHeader.TagType) {
        case 0x08:
            printf("Audio");
            break;
        case 0x09:
            printf("Video");
            break;
        case 0x12:
            printf("script data");
            break;
        default:
            fprintf(stderr, "%s:%d %s Tag Header Type Wrong 0x%x not in 0x12 0x09 0x08\n", __FILE__, __LINE__, __FUNCTION__, p_flvTag->tagHeader.TagType);
            return false;
        }
        printf(")\n");
        printf("flv Tag Header Datasize: %u\n", p_flvTag->tagHeader.DataSize);
        printf("flv Tag Header Timestamp(include timestamp_ex): %u\n", p_flvTag->tagHeader.TimeStamp);
        printf("flv Tag Header StreamID: %u\n", p_flvTag->tagHeader.StreamID);
    } else {
        fprintf(stderr, "%s:%d %s readFlvTagHeader failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    p_flvTag->tagData = calloc(p_flvTag->tagHeader.DataSize, 1);
    size_t readBytes = fread(p_flvTag->tagData, 1, p_flvTag->tagHeader.DataSize, fp);
    if (readBytes != p_flvTag->tagHeader.DataSize) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, p_flvTag->tagHeader.DataSize, strerror(errno));
        free(p_flvTag->tagData);
        p_flvTag->tagData = NULL;
        return false;
    }
    switch (p_flvTag->tagHeader.TagType) {
    case 0x08:
    	parseFlvAudioData(p_flvTag);
        break;
    case 0x09:
        parseFlvVideoData(p_flvTag);
        break;
    case 0x12:
        parseFlvScriptData(p_flvTag);
        break;
    }
    return true;
}
