#include "flvparser.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define PREVIOUS_TAG_SIZE_LEN 4
#define FLV_HEAD_LEN 9
#define TAG_HEAD_LEN 11

// read Previous Tag Size
static bool
readFlvPreviousTagSize(FILE *fp, FlvTag_t *p_flv_tag)
{
    uint8_t previousTagSizeBuf[PREVIOUS_TAG_SIZE_LEN] = { 0 };
    size_t readBytes = fread(previousTagSizeBuf, sizeof(uint8_t), PREVIOUS_TAG_SIZE_LEN, fp);
    if (readBytes != PREVIOUS_TAG_SIZE_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, PREVIOUS_TAG_SIZE_LEN, strerror(errno));
        return false;
    }
    p_flv_tag->previousTagSize = previousTagSizeBuf[3] | previousTagSizeBuf[2] << 8 | previousTagSizeBuf[1] << 16 | previousTagSizeBuf[0] << 24;
    return true;
}

// read Tag Header
static bool
readFlvTagHeader(FILE *fp, FlvTag_t *p_flv_tag)
{
    unsigned char tagHeader[TAG_HEAD_LEN] = { 0 };
    size_t readBytes = fread(tagHeader, sizeof(unsigned char), TAG_HEAD_LEN, fp);
    if (readBytes != TAG_HEAD_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, TAG_HEAD_LEN, strerror(errno));
        return false;
    }
    p_flv_tag->tagHeader.type = tagHeader[0];
    p_flv_tag->tagHeader.dataSize = tagHeader[3] | tagHeader[2] << 8 | tagHeader[1] << 16;
    p_flv_tag->tagHeader.timeStamp = tagHeader[6] | tagHeader[5] << 8 | tagHeader[4] << 16 | tagHeader[7] << 24;
    p_flv_tag->tagHeader.streamID = tagHeader[8] | tagHeader[9] << 8 | tagHeader[10] << 16;

    switch (p_flv_tag->tagHeader.type) {
        case 0x12:
        case 0x09:
        case 0x08:
            return true;
        default:
            return false;
    }
}

static double
hexStr2double(uint8_t* hex)
{
    char hexstr[8 * 2] = {0};
    for (unsigned int i = 0; i < 8; i++) {
        sprintf(hexstr + i * 2, "%02x", hex[i]);
    }

    double ret = 0;
    sscanf(hexstr, "%llx", (unsigned long long*) &ret);
    return ret;
}

static uint32_t
parseFlvMetaDataScriptDataValue(uint8_t *p_meta, uint32_t offset, int depth)
{
    depth += 1;
    uint8_t amf_type = p_meta[offset];
    offset += 1;

    if (amf_type == 0x00) {        // Number
        double numValue = hexStr2double(p_meta + offset);
        offset += 8;
        printf("%f", numValue);
    } else if (amf_type == 0x01) { // Boolean
        uint8_t boolValue = p_meta[offset];
        offset += 1;
        printf("%hhu", boolValue);
    } else if (amf_type == 0x02) { // String
        uint16_t StringLength = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        char StringData[StringLength + 1];
        memset(StringData, 0, sizeof(StringData));
        memcpy(StringData, p_meta + offset, StringLength);
        offset += StringLength;
        printf("%s", StringData);
    } else if (amf_type == 0x03) { // Object
        for (;;) {
            // List terminator
            if (p_meta[offset] == 0 && p_meta[offset + 1] == 0 && p_meta[offset + 2] == 9) {
                break;
            }
            // List of object properties
            uint16_t PropertyNameLen = p_meta[offset + 1] | p_meta[offset] << 8;
            offset += 2;
            char PropertyName[PropertyNameLen + 1];
            memset(PropertyName, 0, sizeof(PropertyName));
            memcpy(PropertyName, p_meta + offset, PropertyNameLen);
            offset += PropertyNameLen;
            printf("\n");
            for (int i = 0; i < depth + 1; ++i) {
                printf("    ");
            }
            printf("%s = ", PropertyName);
            offset = parseFlvMetaDataScriptDataValue(p_meta, offset, depth);
            printf("\n");
        }
    } else if (amf_type == 0x04) { // MovieClip (reserved, not supported)
        printf("MovieClip");
    } else if (amf_type == 0x05) { // Null
        printf("Null");
    } else if (amf_type == 0x06) { // Undefined
        printf("Undefined");
    } else if (amf_type == 0x07) { // Reference
        uint16_t refValue = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        printf("%hu", refValue);
    } else if (amf_type == 0x08) { // ECMA array
        uint32_t ECMAArrayLength = p_meta[offset + 3] | p_meta[offset + 2] << 8 | p_meta[offset + 1] << 16 | p_meta[offset];
        offset += 4;
        printf("[(%u)", ECMAArrayLength);
        printf("\n");
        for (uint32_t i = 0; i < ECMAArrayLength; i++) {
            uint16_t PropertyNameLen = p_meta[offset + 1] | p_meta[offset] << 8;
            offset += 2;
            char PropertyName[PropertyNameLen + 1];
            memset(PropertyName, 0, sizeof(PropertyName));
            memcpy(PropertyName, p_meta + offset, PropertyNameLen);
            offset += PropertyNameLen;
            for (int i = 0; i < depth + 1; ++i) {
                printf("    ");
            }
            printf("%s = ", PropertyName);
            offset = parseFlvMetaDataScriptDataValue(p_meta, offset, depth);
            printf("\n");
        }
        for (int i = 0; i < depth; ++i) {
            printf("    ");
        }
        printf("]");
    } else if (amf_type == 0x09) { // Object end marker
        printf("Object end marker ");
    } else if (amf_type == 0x0a) { // Strict array
        uint32_t StrictArrayLength = p_meta[offset + 3] | p_meta[offset + 2] << 8 | p_meta[offset + 1] << 16 | p_meta[offset];
        offset += 4;
        printf("[(%u) ", StrictArrayLength);
        for (uint32_t i = 0; i < StrictArrayLength; i++) {
            offset = parseFlvMetaDataScriptDataValue(p_meta, offset, depth);
            printf(" ");
        }
        printf("]");
    } else if (amf_type == 0x0b) { // Date
        double dateTime = hexStr2double(p_meta + offset);
        offset += 8;
        int16_t localDateTimeOffset = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        printf("%f %hu ", dateTime, localDateTimeOffset);
    } else if (amf_type == 0x0c) { // Long string
        uint32_t strLen = p_meta[offset + 3] | p_meta[offset + 2] << 8 | p_meta[offset + 1] << 16 | p_meta[offset];
        offset += strLen;
        char strValue[strLen + 1];
        memset(strValue, 0, sizeof(strValue));
        memcpy(strValue, p_meta + offset, strLen);
        offset += strLen;
        printf("%s ", strValue);
    }
    return offset;
}

static bool
parseFlvMetaData(FILE *fp, FlvTag_t *p_flv_tag)
{
    printf("flv Tag Meta Data:\n");
    uint32_t offset = 0;
    while (offset < p_flv_tag->tagHeader.dataSize) {
        printf("    ");
        offset = parseFlvMetaDataScriptDataValue(p_flv_tag->tagData, offset, 0);
        printf("\n");
    }
    return true;
}

bool
parseFlvHeader(FILE *fp, FlvHeader_t *p_flvHeader)
{
    unsigned char buf[FLV_HEAD_LEN] = { 0 };
    size_t readBytes = fread(buf, sizeof(unsigned char), FLV_HEAD_LEN, fp);
    if (readBytes != FLV_HEAD_LEN) {
        fprintf(stderr, "%s:%d %s fread %p return %lu < %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, FLV_HEAD_LEN, strerror(errno));
        return false;
    }
    p_flvHeader->Signature[0] = buf[0];
    p_flvHeader->Signature[1] = buf[1];
    p_flvHeader->Signature[2] = buf[2];
    p_flvHeader->Version = buf[3];
    p_flvHeader->Flags = buf[4];
    p_flvHeader->Headersize = buf[8] | buf[7] << 8 | buf[6] << 16 | buf[5] << 24;

    if (p_flvHeader->Signature[0] != 'F' || p_flvHeader->Signature[1] != 'L' || p_flvHeader->Signature[2] != 'V') {
        fprintf(stderr, "%s:%d %s Not a FLV file!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }
    printf("flv Header Signature: %c%c%c\n", p_flvHeader->Signature[0], p_flvHeader->Signature[1], p_flvHeader->Signature[2]);
    printf("flv Header Version: %d\n", buf[3]);
    printf("flv Header Flags: %d (", buf[4]);
    if (buf[4] & 0x01) {
        printf("Video ");
    }
    if (buf[4] & 0x04) {
        printf("Audio ");
    }
    printf(")\n\n");
    return true;
}

bool
parseFlvTag(FILE *fp, FlvTag_t *p_flvTag)
{
    bool success = false;

    success = readFlvPreviousTagSize(fp, p_flvTag);
    if (success) {
        printf("flv Tag Previous Tag Size: %u\n", p_flvTag->previousTagSize);
    } else {
        fprintf(stderr, "%s:%d %s readFlvTagHeader failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    success = readFlvTagHeader(fp, p_flvTag);
    if (success) {
        printf("flv Tag Header Type: 0x%x (", p_flvTag->tagHeader.type);
        switch (p_flvTag->tagHeader.type) {
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
            fprintf(stderr, "%s:%d %s Tag Header Type Wrong 0x%x not in 0x12 0x09 0x08\n", __FILE__, __LINE__, __FUNCTION__, p_flvTag->tagHeader.type);
            return false;
        }
        printf(")\n");
        printf("flv Tag Header Datasize: %u\n", p_flvTag->tagHeader.dataSize);
        printf("flv Tag Header Timestamp(include timestamp_ex): %u\n",p_flvTag->tagHeader.timeStamp);
        printf("flv Tag Header StreamID: %u\n", p_flvTag->tagHeader.streamID);
    } else {
        fprintf(stderr, "%s:%d %s readFlvTagHeader failed!\n", __FILE__, __LINE__, __FUNCTION__);
        return false;
    }

    p_flvTag->tagData = calloc(p_flvTag->tagHeader.dataSize, 1);
    size_t readBytes = fread(p_flvTag->tagData, 1, p_flvTag->tagHeader.dataSize, fp);
    if (readBytes != p_flvTag->tagHeader.dataSize) {
        fprintf(stderr, "%s:%d %s fread %p return %lu != %d: %s\n", __FILE__, __LINE__, __FUNCTION__, fp, readBytes, p_flvTag->tagHeader.dataSize, strerror(errno));
        free(p_flvTag->tagData);
        p_flvTag->tagData = NULL;
        return false;
    }
    switch (p_flvTag->tagHeader.type) {
    case 0x08:
        printf("Audio");
        break;
    case 0x09:
        printf("Video");
        break;
    case 0x12:
        parseFlvMetaData(fp, p_flvTag);
        break;
    }
    return true;
}


