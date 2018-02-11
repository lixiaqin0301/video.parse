#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static struct {
    uint32_t mdhd_time_scale;
    uint32_t stss_entry_num;
    struct {
        uint32_t sync_sample;
    } * stss_entry_data;
    uint32_t stts_entry_num;
    struct {
        uint32_t sample_count;
        uint32_t sample_duration;
    } * stts_entry_data;
    uint32_t stsc_entry_num;
    struct {
        uint32_t first_chunk;
        uint32_t samples_per_chunk;
    } * stsc_entry_data;
    uint32_t stco_entry_num;
    struct {
        uint32_t chunk_offset;
    } * stco_entry_data;
    uint32_t keyframes_num;
    struct {
        double sync_sample_time;
        uint32_t chunk_offset;
        uint32_t sync_sample;
        uint32_t sync_sample_duration;
        uint32_t sync_chunk;
    } * keyframes_data;
    bool need_parse;
    uint8_t *content_buf;
    FILE *fp;
} g_keyframe;

static void mp4_box(const uint8_t *p, size_t len, int depth);
static void free_keyframe_data(void);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    atexit(free_keyframe_data);

    const char *filename = argv[1];

    struct stat sb = {0};
    if (stat(filename, &sb) < 0) {
        fprintf(stderr, "%s:%d %s stat(\"%s\", &sb) error: %s", __FILE__, __LINE__, __FUNCTION__, filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    g_keyframe.fp = fopen(filename, "rb");
    if (!g_keyframe.fp) {
        fprintf(stderr, "%s:%d %s fopen(\"%s\", \"rb\") error: %s", __FILE__, __LINE__, __FUNCTION__, filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    g_keyframe.need_parse = true;
    g_keyframe.content_buf = malloc(sb.st_size);
    if (!g_keyframe.content_buf) {
        fprintf(stderr, "%s:%d %s calloc(%ld) error: %s", __FILE__, __LINE__, __FUNCTION__, (long)sb.st_size, strerror(errno));
        exit(EXIT_FAILURE);
    }
    if (fread(g_keyframe.content_buf, 1, sb.st_size, g_keyframe.fp) != sb.st_size) {
        fprintf(stderr, "%s:%d %s fread error: %s", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
        exit(EXIT_FAILURE);
    }
    mp4_box(g_keyframe.content_buf, sb.st_size, 0);
    // stss sync sample table (keyframs table)
    g_keyframe.keyframes_num = g_keyframe.stss_entry_num;
    g_keyframe.keyframes_data = calloc(g_keyframe.keyframes_num, sizeof(*g_keyframe.keyframes_data));
    for (int i = 0; i < g_keyframe.keyframes_num; i++) {
        g_keyframe.keyframes_data[i].sync_sample = g_keyframe.stss_entry_data[i].sync_sample;
    }
    // stts time-to-sample
    for (int i = 0; i < g_keyframe.keyframes_num; i++) {
        int total_count = 0;
        int total_duration = 0;
        for (int j = 0; j < g_keyframe.stts_entry_num; j++) {
            if (total_count + g_keyframe.stts_entry_data[j].sample_count >= g_keyframe.keyframes_data[i].sync_sample - 1) {
                int extra_count = g_keyframe.keyframes_data[i].sync_sample - 1 - total_count;
                if (extra_count < 0) {
                    extra_count = 0;
                }
                g_keyframe.keyframes_data[i].sync_sample_duration = total_duration + extra_count * g_keyframe.stts_entry_data[j].sample_duration;
                break;
            }
            total_count += g_keyframe.stts_entry_data[j].sample_count;
            total_duration += g_keyframe.stts_entry_data[j].sample_count * g_keyframe.stts_entry_data[j].sample_duration;
        }
        g_keyframe.keyframes_data[i].sync_sample_time = (double)g_keyframe.keyframes_data[i].sync_sample_duration / g_keyframe.mdhd_time_scale;
    }
    // stsc sample-to-chunk
    for (int i = 0; i < g_keyframe.keyframes_num; i++) {
        int total_sample = 0;
        for (int j = 0; j < g_keyframe.stsc_entry_num; j++) {
            uint32_t next_stsc_first_chunk = g_keyframe.stco_entry_num + 1;
            if (j < g_keyframe.stsc_entry_num - 1) {
                next_stsc_first_chunk = g_keyframe.stsc_entry_data[j + 1].first_chunk;
            }

            int cur_sample = g_keyframe.stsc_entry_data[j].samples_per_chunk * (next_stsc_first_chunk - g_keyframe.stsc_entry_data[j].first_chunk);
            if (total_sample + cur_sample >= g_keyframe.keyframes_data[i].sync_sample) {
                g_keyframe.keyframes_data[i].sync_chunk = g_keyframe.stsc_entry_data[j].first_chunk + (g_keyframe.keyframes_data[i].sync_sample - total_sample - 1) / g_keyframe.stsc_entry_data[j].samples_per_chunk;
                break;
            }
            total_sample += cur_sample;
        }
    }
    // stco chunk offsete
    for (int i = 0; i < g_keyframe.keyframes_num; i++) {
        g_keyframe.keyframes_data[i].chunk_offset = g_keyframe.stco_entry_data[g_keyframe.keyframes_data[i].sync_chunk - 1].chunk_offset;
    }

    for (int i = 0; i < g_keyframe.keyframes_num; i++) {
        printf("%-7g %u\n", g_keyframe.keyframes_data[i].sync_sample_time, g_keyframe.keyframes_data[i].chunk_offset);
    }
    exit(EXIT_SUCCESS);
}

static inline uint32_t
get_u32(const uint8_t *p)
{
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

static inline uint64_t
get_u64(const uint8_t *p)
{
    return ((uint64_t)p[0] << 56) | ((uint64_t)p[1] << 48) | ((uint64_t)p[2] << 40) | ((uint64_t)p[3] << 32) | ((uint64_t)p[4] << 24) | ((uint64_t)p[5] << 16) | ((uint64_t)p[6] << 8) | (uint64_t)p[7];
}

typedef void (*mp4_box_func)(const uint8_t *p, size_t len, int depth);
static void mp4_box_mdhd(const uint8_t *p, size_t len, int depth);
static void mp4_box_stss(const uint8_t *p, size_t len, int depth);
static void mp4_box_stts(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsc(const uint8_t *p, size_t len, int depth);
static void mp4_box_stco(const uint8_t *p, size_t len, int depth);

static void
mp4_box(const uint8_t *buf, size_t len, int depth)
{
    const uint8_t *p = buf;
    const uint8_t *end = buf + len;

    while (p < end) {
        uint64_t box_size = get_u32(p);
        char box_type[5] = {0};
        strncpy(box_type, (const char *)p + 4, 4);
        const uint8_t *box_data = p + 8;

        if (box_size == 1) {
            box_size = get_u64(box_data);
            box_data = p + 16;
        }

        mp4_box_func func = NULL;
        if (strncmp(box_type, "moov", 4) == 0
            || strncmp(box_type, "trak", 4) == 0
            || strncmp(box_type, "mdia", 4) == 0
            || strncmp(box_type, "minf", 4) == 0
            || strncmp(box_type, "stbl", 4) == 0) {
            func = mp4_box;
        } else if (strncmp(box_type, "mdhd", 4) == 0) {
            func = mp4_box_mdhd;
        } else if (strncmp(box_type, "stss", 4) == 0) {
            func = mp4_box_stss;
        } else if (strncmp(box_type, "stts", 4) == 0) {
            func = mp4_box_stts;
        } else if (strncmp(box_type, "stsc", 4) == 0) {
            func = mp4_box_stsc;
        } else if (strncmp(box_type, "stco", 4) == 0) {
            func = mp4_box_stco;
        }
        if (func) {
            func(box_data, box_size - (box_data - p), depth + 1);
        }
        p += box_size;
    }
}

static void
mp4_box_mdhd(const uint8_t *p, size_t len, int depth)
{
    if (g_keyframe.stss_entry_num > 0) {
        g_keyframe.need_parse = false;
    }

    if (g_keyframe.need_parse) {
        g_keyframe.mdhd_time_scale = get_u32(p + 12);
    }
}

static void
mp4_box_stss(const uint8_t *p, size_t len, int depth)
{
    if (!g_keyframe.need_parse) {
        return;
    }

    g_keyframe.stss_entry_num = get_u32(p + 4);
    g_keyframe.stss_entry_data = calloc(g_keyframe.stss_entry_num, sizeof(*g_keyframe.stss_entry_data));
    for (uint32_t i = 0; i < g_keyframe.stss_entry_num; i++) {
        g_keyframe.stss_entry_data[i].sync_sample = get_u32(p + 8 + i * 4);
    }
}

static void
mp4_box_stts(const uint8_t *p, size_t len, int depth)
{
    if (!g_keyframe.need_parse) {
        return;
    }

    g_keyframe.stts_entry_num = get_u32(p + 4);
    g_keyframe.stts_entry_data = calloc(g_keyframe.stts_entry_num, sizeof(*g_keyframe.stts_entry_data));
    for (uint32_t i = 0; i < g_keyframe.stts_entry_num; i++) {
        g_keyframe.stts_entry_data[i].sample_count = get_u32(p + 8 + i * 8);
        g_keyframe.stts_entry_data[i].sample_duration = get_u32(p + 8 + i * 8 + 4);
    }
}

static void
mp4_box_stsc(const uint8_t *p, size_t len, int depth)
{
    if (!g_keyframe.need_parse) {
        return;
    }

    g_keyframe.stsc_entry_num = get_u32(p + 4);
    g_keyframe.stsc_entry_data = calloc(g_keyframe.stsc_entry_num, sizeof(*g_keyframe.stsc_entry_data));
    for (uint32_t i = 0; i < g_keyframe.stsc_entry_num; i++) {
        g_keyframe.stsc_entry_data[i].first_chunk = get_u32(p + 8 + 12 * i);
        g_keyframe.stsc_entry_data[i].samples_per_chunk = get_u32(p + 8 + 12 * i + 4);
    }
}

static void
mp4_box_stco(const uint8_t *p, size_t len, int depth)
{
    if (!g_keyframe.need_parse) {
        return;
    }

    g_keyframe.stco_entry_num = get_u32(p + 4);
    g_keyframe.stco_entry_data = calloc(g_keyframe.stco_entry_num, sizeof(*g_keyframe.stco_entry_data));
    for (uint32_t i = 0; i < g_keyframe.stco_entry_num; i++) {
        g_keyframe.stco_entry_data[i].chunk_offset = get_u32(p + 8 + i * 4);
    }
}

static void
free_keyframe_data(void)
{
    free(g_keyframe.stss_entry_data);
    free(g_keyframe.stts_entry_data);
    free(g_keyframe.stsc_entry_data);
    free(g_keyframe.stco_entry_data);
    free(g_keyframe.keyframes_data);
    free(g_keyframe.content_buf);
    if (g_keyframe.fp) {
        fclose(g_keyframe.fp);
    }
    memset(&g_keyframe, 0, sizeof(g_keyframe));
}
