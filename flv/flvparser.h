#ifndef _FLVPARSER_H
#define _FLVPARSER_H

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

struct FlvHeader_s {
    uint8_t Signature[3];
    uint8_t Version;
    uint8_t Flags;
    uint8_t Headersize;
};

typedef struct FlvHeader_s FlvHeader_t;

struct FlvTag_s {
    uint32_t previousTagSize;
    struct {
        uint8_t type;
        uint32_t dataSize;
        uint32_t timeStamp;
        uint32_t streamID;
    } tagHeader;
    void *tagData;
};

typedef struct FlvTag_s FlvTag_t;

bool parseFlvHeader(FILE *fp, FlvHeader_t *p_flvHeader);
bool parseFlvTag(FILE *fp, FlvTag_t *p_flvTag);

#endif //_FLVPARSER_H
