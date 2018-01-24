#include "flvparsescriptdata.h"
#include "flvparser.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PREVIOUS_TAG_SIZE_LEN 4
#define FLV_HEAD_LEN 9
#define TAG_HEAD_LEN 11

static double
hexStr2double(const uint8_t *hex)
{
    char hexstr[8 * 2] = {0};
    for (unsigned int i = 0; i < 8; i++) {
        sprintf(hexstr + i * 2, "%02x", hex[i]);
    }
    double ret = 0;
    sscanf(hexstr, "%llx", (unsigned long long *)&ret);
    return ret;
}

static uint32_t
parseFlvScriptDataScriptDataValue(const uint8_t *p_meta, uint32_t offset, int depth)
{
    depth += 1;
    uint8_t amf_type = p_meta[offset];
    offset += 1;

    if (amf_type == 0x00) {  // Number
        double numValue = hexStr2double(p_meta + offset);
        offset += 8;
        printf("%f", numValue);
    } else if (amf_type == 0x01) {  // Boolean
        uint8_t boolValue = p_meta[offset];
        offset += 1;
        printf("%hhu", boolValue);
    } else if (amf_type == 0x02) {  // String
        uint16_t StringLength = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        char StringData[StringLength + 1];
        memset(StringData, 0, sizeof(StringData));
        memcpy(StringData, p_meta + offset, StringLength);
        offset += StringLength;
        printf("%s", StringData);
    } else if (amf_type == 0x03) {  // Object
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
            offset = parseFlvScriptDataScriptDataValue(p_meta, offset, depth);
            printf("\n");
        }
    } else if (amf_type == 0x04) {  // MovieClip (reserved, not supported)
        printf("MovieClip");
    } else if (amf_type == 0x05) {  // Null
        printf("Null");
    } else if (amf_type == 0x06) {  // Undefined
        printf("Undefined");
    } else if (amf_type == 0x07) {  // Reference
        uint16_t refValue = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        printf("%hu", refValue);
    } else if (amf_type == 0x08) {  // ECMA array
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
            offset = parseFlvScriptDataScriptDataValue(p_meta, offset, depth);
            printf("\n");
        }
        for (int i = 0; i < depth; ++i) {
            printf("    ");
        }
        printf("]");
    } else if (amf_type == 0x09) {  // Object end marker
        printf("Object end marker ");
    } else if (amf_type == 0x0a) {  // Strict array
        uint32_t StrictArrayLength = p_meta[offset + 3] | p_meta[offset + 2] << 8 | p_meta[offset + 1] << 16 | p_meta[offset];
        offset += 4;
        printf("[(%u) ", StrictArrayLength);
        for (uint32_t i = 0; i < StrictArrayLength; i++) {
            offset = parseFlvScriptDataScriptDataValue(p_meta, offset, depth);
            printf(" ");
        }
        printf("]");
    } else if (amf_type == 0x0b) {  // Date
        double dateTime = hexStr2double(p_meta + offset);
        offset += 8;
        int16_t localDateTimeOffset = p_meta[offset + 1] | p_meta[offset] << 8;
        offset += 2;
        printf("%f %hu ", dateTime, localDateTimeOffset);
    } else if (amf_type == 0x0c) {  // Long string
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

bool
parseFlvScriptData(const uint8_t *buf, uint32_t buflen)
{
    printf("flv Tag Script Data:\n");
    uint32_t offset = 0;
    while (offset < buflen) {
        printf("    ");
        offset = parseFlvScriptDataScriptDataValue(buf, offset, 0);
        printf("\n");
    }
    printf("\n");
    return true;
}
