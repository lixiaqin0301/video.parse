#ifndef _FLVPARSER_H_2018
#define _FLVPARSER_H_2018

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint8_t Signature[3];
    uint8_t Version;
    unsigned int TypeFlagsReserved1:5;
    unsigned int TypeFlagsAudio:1;
    unsigned int TypeFlagsReserved2:1;
    unsigned int TypeFlagsVideo:1;
    uint8_t DataOffset;
} FlvHeader_t;

typedef struct {
    uint32_t PreviousTagSize;
    struct {
    	unsigned int Reserved:2;
    	unsigned int Filter:1;
    	unsigned int TagType:5;
        uint32_t DataSize;
        uint32_t TimeStamp;
        uint32_t StreamID;
    } tagHeader;
    void *tagData;
} FlvTag_t;

bool parseFlvHeader(FILE *fp, FlvHeader_t *p_flvHeader);
bool parseFlvTag(FILE *fp, FlvTag_t *p_flvTag);

#endif  //_FLVPARSER_H_2018
