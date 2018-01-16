#include "flvparser.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "%s:%d %s open %s failed: %s\n", __FILE__, __LINE__, __FUNCTION__, argv[1], strerror(errno));
        return false;
    }
    printf("flv file path: %s\n\n", argv[1]);

    bool success = false;

    FlvHeader_t flvHeader = { 0 };
    success = parseFlvHeader(fp, &flvHeader);
    if (!success) {
        fprintf(stderr, "%s:%d %s parseFlvHeader %s failed\n", __FILE__, __LINE__, __FUNCTION__, argv[0]);
        fclose(fp);
        return 1;
    }

    FlvTag_t flvTag = { 0 };
    success = parseFlvTag(fp, &flvTag);
    if (!success) {
        fprintf(stderr, "%s:%d %s parseFlvHeader %s failed\n", __FILE__, __LINE__, __FUNCTION__, argv[0]);
        fclose(fp);
        return 1;
    }

//    FlvReader flvReader(argv[1]);
//
//    FlvMetaData *flvMetaData = flvReader.readMeta();
//
//    cout << "duration: " << flvMetaData->getDuration() << "s" << endl;
//    cout << "width: " << flvMetaData->getWidth() << endl;
//    cout << "height: " << flvMetaData->getHeight() << endl;
//    cout << "framerate: " << flvMetaData->getFramerate() << endl;
//    cout << "videodatarate: " << flvMetaData->getVideoDatarate() << endl;
//    cout << "audiodatarate: " << flvMetaData->getAudioDatarate() << endl;
//    cout << "videocodecid: " << flvMetaData->getVideoCodecId() << endl;
//    cout << "audiosamplerate: " << flvMetaData->getAudioSamplerate() << endl;
//    cout << "audiosamplesize: " << flvMetaData->getAudioSamplesize() << endl;
//    cout << "audiocodecid: " << flvMetaData->getAudioCodecId() << endl;
//    cout << "stereo: " << flvMetaData->getStereo() << endl;
//
//    delete flvMetaData;
//    flvMetaData = NULL;

    return 0;
}
