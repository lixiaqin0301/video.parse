#include "flvparseaudiodata.h"
#include "flvparser.h"

typedef struct {
    unsigned int FrameType : 4;  // 1 = key frame (for AVC, a seekable frame); 2 = inter frame (for AVC, a non-seekable frame); 3 = disposable inter frame (H.263 only); 4 = generated key frame (reserved for server use only); 5 = video info/command frame
    unsigned int CodecID : 4;    // 2 = Sorenson H.263; 3 = Screen video 4 = On2 VP6; 5 = On2 VP6 with alpha channel 6 = Screen video version 2; 7 = AVC
    uint8_t AVCPacketType;       // IF CodecID == 7: 0 = AVC sequence header; 1 = AVC NALU 2 = AVC end of sequence (lower level NALU sequence ender is not required or supported)
    int32_t CompositionTime;     // IF CodecID == 7: IF AVCPacketType == 1 Composition time offset ELSE 0
} FlvVideoTagHeader_t;

bool
parseFlvVideoData(FlvTag_t *p_flv_tag)
{
    uint8_t *buf = p_flv_tag->tagData;
    FlvVideoTagHeader_t video_header = { 0 };
    video_header.FrameType = (buf[0] & 0xf0) >> 4;
    video_header.CodecID = buf[0] && 0x0f;
    if (video_header.CodecID == 7) {
        video_header.AVCPacketType = buf[1];
        video_header.CompositionTime = buf[2] << 16 | buf[3] << 8 | buf[4];
    }
    printf("flv Tag Video Header Frame Type: %d ", (int) video_header.FrameType);
    switch (video_header.FrameType) {
    case 1:
        printf("key frame (for AVC, a seekable frame)");
        break;
    case 2:
        printf("inter frame (for AVC, a non-seekable frame)");
        break;
    case 3:
        printf("disposable inter frame (H.263 only)");
        break;
    case 4:
        printf("generated key frame (reserved for server use only)");
        break;
    case 5:
        printf("video info/command frame");
        break;
    }
    printf("\n");
    printf("flv Tag Video Header CodecID: %d (", (int) video_header.CodecID);
    switch (video_header.CodecID) {
    case 2:
        printf("Sorenson H.263");
        break;
    case 3:
        printf("Screen video");
        break;
    case 4:
        printf("On2 VP6");
        break;
    case 5:
        printf("On2 VP6 with alpha channel");
        break;
    case 6:
        printf("Screen video version 2");
        break;
    case 7:
        printf("AVC");
        break;
    }
    printf(")\n");
    if (video_header.CodecID == 7) {
        printf("flv Tag Video Header AVCPacketType: %d ", (int) video_header.AVCPacketType);
        switch (video_header.AVCPacketType) {
        case 0:
            printf("AVC sequence header");
            break;
        case 1:
            printf("AVC NALU");
            break;
        case 2:
            printf("AVC end of sequence (lower level NALU sequence ender is not required or supported)");
            break;
        }
        printf("\n");
        printf("flv Tag Video Header CompositionTime: %d\n", (int) video_header.CompositionTime);
    }
    printf("\n");
    return true;
}
