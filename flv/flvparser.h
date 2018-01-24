#ifndef _FLVPARSER_H_2018
#define _FLVPARSER_H_2018

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool parseFlvHeader(FILE *fp);
bool parseFlvTag(FILE *fp);

#endif  //_FLVPARSER_H_2018
