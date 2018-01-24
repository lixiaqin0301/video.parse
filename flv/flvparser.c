#include "flvparser.h"
#include "flvparseaudiodata.h"
#include "flvparsescriptdata.h"
#include "flvparsevideodata.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_HEAD_LEN 11

typedef struct {
    uint8_t Signature[3];
    uint8_t Version;
    unsigned int TypeFlagsReserved1 : 5;
    unsigned int TypeFlagsAudio : 1;
    unsigned int TypeFlagsReserved2 : 1;
    unsigned int TypeFlagsVideo : 1;
    uint8_t DataOffset;
} FlvHeader_t;

typedef struct {
    uint32_t PreviousTagSize;
    struct {
        unsigned int Reserved : 2;
        unsigned int Filter : 1;
        unsigned int TagType : 5;
        uint32_t DataSize;
        uint32_t TimeStamp;
        uint32_t StreamID;
    } tagHeader;
    void *tagData;
} FlvTag_t;

// The FLV header
static void
printFlvHeader(long offset, const FlvHeader_t *p_flvHeader)
{
    printf("%ld:\n", offset);
    printf("flv Header Signature: %c%c%c\n", p_flvHeader->Signature[0], p_flvHeader->Signature[1], p_flvHeader->Signature[2]);
    printf("flv Header Version: %hhd\n", p_flvHeader->Version);
    printf("flv Header TypeFlagsAudio: %d\n", (int)p_flvHeader->TypeFlagsAudio);
    printf("flv Header TypeFlagsVideo: %d\n", (int)p_flvHeader->TypeFlagsVideo);
    printf("flv Header DataOffset: %u\n", p_flvHeader->DataOffset);
    printf("\n");
}

bool
parseFlvHeader(FILE *fp)
{
    long offset = ftell(fp);
    const int FLV_HEAD_LEN = 9;
    unsigned char buf[FLV_HEAD_LEN];
    size_t readBytes = fread(buf, sizeof(unsigned char), FLV_HEAD_LEN, fp);
    if (readBytes != FLV_HEAD_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu < %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, FLV_HEAD_LEN, strerror(errno));
        return false;
    }
    FlvHeader_t flv_header = {0};
    flv_header.Signature[0] = buf[0];
    flv_header.Signature[1] = buf[1];
    flv_header.Signature[2] = buf[2];
    flv_header.Version = buf[3];
    flv_header.TypeFlagsReserved1 = (buf[4] & 0xf8) >> 3;
    flv_header.TypeFlagsAudio = (buf[4] & 0x04) >> 2;
    flv_header.TypeFlagsReserved2 = (buf[4] & 0x02) >> 1;
    flv_header.TypeFlagsVideo = buf[4] & 0x01;
    flv_header.DataOffset = buf[8] | buf[7] << 8 | buf[6] << 16 | buf[5] << 24;

    if (flv_header.Signature[0] != 'F' || flv_header.Signature[1] != 'L' || flv_header.Signature[2] != 'V' || flv_header.TypeFlagsReserved1 != 0 || flv_header.TypeFlagsReserved2 != 0 || (flv_header.Version == 1 && flv_header.DataOffset != 9)) {
        fprintf(stderr, "%s:%d %s Not a FLV file!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }
    printFlvHeader(offset, &flv_header);
    return true;
}

// Previous Tag Size
static void
printFlvPreviousTagSize(long offset, FlvTag_t *p_flvTag)
{
    printf("%ld:\n", offset);
    printf("flv Previous Tag Size: %u\n", p_flvTag->PreviousTagSize);
    printf("\n");
}

// Previous Tag Size
static bool
parseFlvPreviousTagSize(FILE *fp, FlvTag_t *p_flvTag)
{
    const int PREVIOUS_TAG_SIZE_LEN = 4;
    long offset = ftell(fp);
    uint8_t previousTagSizeBuf[PREVIOUS_TAG_SIZE_LEN];
    size_t readBytes = fread(previousTagSizeBuf, sizeof(uint8_t), PREVIOUS_TAG_SIZE_LEN, fp);
    if (readBytes != PREVIOUS_TAG_SIZE_LEN && readBytes != 0) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, PREVIOUS_TAG_SIZE_LEN, strerror(errno));
        return false;
    } else if (readBytes == 0) {
        exit(EXIT_SUCCESS);
    }
    p_flvTag->PreviousTagSize = previousTagSizeBuf[3] | previousTagSizeBuf[2] << 8 | previousTagSizeBuf[1] << 16 | previousTagSizeBuf[0] << 24;
    printFlvPreviousTagSize(offset, p_flvTag);
    return true;
}

// Tag Header
static void
printFlvTagHeader(long offset, FlvTag_t *p_flvTag)
{
    printf("%ld:\n", offset);
    printf("flv Tag Header Filter: %u (", p_flvTag->tagHeader.Filter);
    switch (p_flvTag->tagHeader.Filter) {
    case 0:
        printf("No pre-processing required");
        break;
    case 1:
        printf("Pre-processing (such as decryption) of the packet is required before it can be rendered.");
        break;
    }
    printf(")\n");
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
    }
    printf(")\n");
    printf("flv Tag Header Datasize: %u\n", p_flvTag->tagHeader.DataSize);
    printf("flv Tag Header Timestamp(include timestamp_ex): %u\n", p_flvTag->tagHeader.TimeStamp);
    printf("flv Tag Header StreamID: %u\n", p_flvTag->tagHeader.StreamID);
}

// Tag Header
static bool
parseFlvTagHeader(FILE *fp, FlvTag_t *p_flvTag)
{
    long offset = ftell(fp);
    unsigned char tagHeader[TAG_HEAD_LEN] = {0};
    size_t readBytes = fread(tagHeader, sizeof(unsigned char), TAG_HEAD_LEN, fp);
    if (readBytes != TAG_HEAD_LEN && readBytes != 0) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, TAG_HEAD_LEN, strerror(errno));
        return false;
    } else if (readBytes == 0) {
        exit(EXIT_SUCCESS);
    }

    p_flvTag->tagHeader.Reserved = (tagHeader[0] & 0xc0) >> 6;
    p_flvTag->tagHeader.Filter = (tagHeader[0] & 0x20) >> 5;
    p_flvTag->tagHeader.TagType = tagHeader[0] & 0x1f;
    p_flvTag->tagHeader.DataSize = tagHeader[3] | tagHeader[2] << 8 | tagHeader[1] << 16;
    p_flvTag->tagHeader.TimeStamp = tagHeader[6] | tagHeader[5] << 8 | tagHeader[4] << 16 | tagHeader[7] << 24;
    p_flvTag->tagHeader.StreamID = tagHeader[8] | tagHeader[9] << 8 | tagHeader[10] << 16;

    if (p_flvTag->tagHeader.Reserved != 0) {
        fprintf(stderr, "%s:%d %s FLV Tag Header Reserved = %d not 0\n", __FILE__, __LINE__, __FUNCTION__, p_flvTag->tagHeader.Reserved);
        return false;
    }
    if (p_flvTag->tagHeader.TagType != 0x12 && p_flvTag->tagHeader.TagType != 0x09 && p_flvTag->tagHeader.TagType != 0x08) {
        fprintf(stderr, "%s:%d %s FLV Tag Header TagType = %d not 0x12 0x09 0x08\n", __FILE__, __LINE__, __FUNCTION__, p_flvTag->tagHeader.TagType);
        return false;
    }
    if (p_flvTag->tagHeader.StreamID != 0) {
        fprintf(stderr, "%s:%d %s FLV Tag Header StreamID = %d not 0\n", __FILE__, __LINE__, __FUNCTION__, p_flvTag->tagHeader.StreamID);
        return false;
    }

    printFlvTagHeader(offset, p_flvTag);
    return true;
}

bool
parseFlvTag(FILE *fp)
{
    FlvTag_t flv_tag = {0};
    bool success = parseFlvPreviousTagSize(fp, &flv_tag);
    if (!success) {
        fprintf(stderr, "%s:%d %s readFlvPreviousTagSize failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    success = parseFlvTagHeader(fp, &flv_tag);
    if (!success) {
        fprintf(stderr, "%s:%d %s readFlvTagHeader failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    flv_tag.tagData = calloc(flv_tag.tagHeader.DataSize, 1);
    size_t readBytes = fread(flv_tag.tagData, 1, flv_tag.tagHeader.DataSize, fp);
    if (readBytes != flv_tag.tagHeader.DataSize) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, flv_tag.tagHeader.DataSize, strerror(errno));
        free(flv_tag.tagData);
        return false;
    }
    switch (flv_tag.tagHeader.TagType) {
    case 0x08:
        parseFlvAudioData(flv_tag.tagData);
        break;
    case 0x09:
        parseFlvVideoData(flv_tag.tagData);
        break;
    case 0x12:
        parseFlvScriptData(flv_tag.tagData, flv_tag.tagHeader.DataSize);
        break;
    }
    free(flv_tag.tagData);
    return true;
}
