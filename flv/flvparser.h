#ifndef _FLVPARSER_H_2018
#define _FLVPARSER_H_2018

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t Signature[3];
    uint8_t Version;
    uint8_t Flags;
    uint8_t Headersize;
} FlvHeader_t;

typedef struct {
    uint32_t previousTagSize;
    struct {
        uint8_t  type;
        uint32_t dataSize;
        uint32_t timeStamp;
        uint32_t streamID;
    } tagHeader;
    void *tagData;
} FlvTag_t;

bool parseFlvHeader(FILE *fp, FlvHeader_t *p_flvHeader);
bool parseFlvTag(FILE *fp, FlvTag_t *p_flvTag);

#endif  //_FLVPARSER_H_2018
