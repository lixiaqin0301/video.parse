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

static uint8_t *g_content_buf = NULL;

static void mp4_print(const uint8_t *p, size_t len, int depth);

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *filename = argv[1];
    printf("Reading file %s\n", filename);

    struct stat sb = {0};
    if (stat(filename, &sb) < 0) {
        fprintf(stderr, "%s:%d %s stat(\"%s\", &sb) error: %s", __FILE__, __LINE__, __FUNCTION__, filename, strerror(errno));
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "%s:%d %s fopen(\"%s\", \"rb\") error: %s", __FILE__, __LINE__, __FUNCTION__, filename, strerror(errno));
        exit(EXIT_FAILURE);
    }
    g_content_buf = malloc(sb.st_size);
    if (!g_content_buf) {
        fprintf(stderr, "%s:%d %s calloc(%ld) error: %s", __FILE__, __LINE__, __FUNCTION__, (long)sb.st_size, strerror(errno));
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    if (fread(g_content_buf, 1, sb.st_size, fp) != sb.st_size) {
        fprintf(stderr, "%s:%d %s fread error: %s", __FILE__, __LINE__, __FUNCTION__, strerror(errno));
        fclose(fp);
        exit(EXIT_FAILURE);
    }
    printf("File Content:\n");
    mp4_print(g_content_buf, sb.st_size, 0);
    free(g_content_buf);
    g_content_buf = 0;
    fclose(fp);
    return EXIT_SUCCESS;
}

static inline uint16_t
get_u16(const uint8_t *p)
{
    return (p[0] << 8) | p[1];
}

static inline uint32_t
get_u24(const uint8_t *p)
{
    return (p[0] << 16) | (p[1] << 8) | p[2];
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

static const char *
indent(int depth, int header)
{
    static char buf[64];
    memset(buf, 0, sizeof(buf));

    for (int i = 0; i < depth; i++) {
        strcat(buf, "|  ");
    }

    if (header) {
        strcat(buf, "+");
    } else {
        strcat(buf, "");
    }

    return buf;
}

const char *
get_box_desc(const uint8_t *name)
{
    if (!name) {
        return "";
    }
    static const struct {
        const char *name;
        const char *desc;
        const char *source;
    } atoms[] = {
        {"ainf", "Asset information to identify, license and play", "DECE"},
        {"avcn", "AVC NAL Unit Storage Box", "DECE"},
        {"bloc", "Base location and purchase location for license acquisition", "DECE"},
        {"bpcc", "Bits per component", "JPEG2000"},
        {"buff", "Buffering information", "NALu Video"},
        {"bxml", "binary XML container", "ISO"},
        {"ccid", "OMA DRM Content ID", "OMA DRM 2.1"},
        {"cdef", "type and ordering of the components within the codestream", "JPEG2000"},
        {"clip", "Reserved", "ISO"},
        {"cmap", "mapping between a palette and codestream components", "JPEG2000"},
        {"co64", "64-bit chunk offset", "ISO"},
        {"coin", "Content Information Box", "DECE"},
        {"colr", "specifies the colourspace of the image", "JPEG2000"},
        {"crgn", "Reserved", "ISO"},
        {"crhd", "reserved for ClockReferenceStream header", "MP4v1"},
        {"cslg", "composition to decode timeline mapping", "ISO"},
        {"ctab", "Reserved", "ISO"},
        {"ctts", "(composition) time to sample", "ISO"},
        {"cvru", "OMA DRM Cover URI", "OMA DRM 2.1"},
        {"dinf", "data information box, container", "ISO"},
        {"dref", "data reference box, declares source(s) of media data in track", "ISO"},
        {"dsgd", "DVB Sample Group Description Box", "DVB"},
        {"dstg", "DVB Sample to Group Box", "DVB"},
        {"edts", "edit list container", "ISO"},
        {"elst", "an edit list", "ISO"},
        {"emsg", "event message", "DASH"},
        {"fdel", "File delivery information (item info extension)", "ISO"},
        {"feci", "FEC Informatiom", "ISO"},
        {"fecr", "FEC Reservoir", "ISO"},
        {"fiin", "FD Item Information", "ISO"},
        {"fire", "File Reservoir", "ISO"},
        {"fpar", "File Partition", "ISO"},
        {"free", "free space", "ISO"},
        {"frma", "original format box", "ISO"},
        {"ftyp", "file type and compatibility", "ISO"},
        {"gitn", "Group ID to name", "ISO"},
        {"grpi", "OMA DRM Group ID", "OMA DRM 2.0"},
        {"hdlr", "handler, declares the media (handler) type", "ISO"},
        {"hmhd", "hint media header, overall information (hint track only)", "ISO"},
        {"hpix", "Hipix Rich Picture (user-data or meta-data)", "Hipix"},
        {"icnu", "OMA DRM Icon URI", "OMA DRM 2.0"},
        {"ID32", "ID3 version 2 container", "id3v2"},
        {"idat", "Item data", "ISO"},
        {"ihdr", "Image Header", "JPEG2000"},
        {"iinf", "item information", "ISO"},
        {"iloc", "item location", "ISO"},
        {"imap", "Reserved", "ISO"},
        {"imif", "IPMP Information box", "ISO"},
        {"infe", "Item information entry", "ISO"},
        {"infu", "OMA DRM Info URL", "OMA DRM 2.0"},
        {"iods", "Object Descriptor container box", "MP4v1"},
        {"iphd", "reserved for IPMP Stream header", "MP4v1"},
        {"ipmc", "IPMP Control Box", "ISO"},
        {"ipro", "item protection", "ISO"},
        {"iref", "Item reference", "ISO"},
        {"jP  ", "JPEG 2000 Signature", "JPEG2000"},
        {"jp2c", "JPEG 2000 contiguous codestream", "JPEG2000"},
        {"jp2h", "Header", "JPEG2000"},
        {"jp2i", "intellectual property information", "JPEG2000"},
        {"kmat", "Reserved", "ISO"},
        {"leva", "Leval assignment", "ISO"},
        {"load", "Reserved", "ISO"},
        {"lrcu", "OMA DRM Lyrics URI", "OMA DRM 2.1"},
        {"m7hd", "reserved for MPEG7Stream header", "MP4v1"},
        {"matt", "Reserved", "ISO"},
        {"mdat", "media data container", "ISO"},
        {"mdhd", "media header, overall information about the media", "ISO"},
        {"mdia", "container for the media information in a track", "ISO"},
        {"mdri", "Mutable DRM information", "OMA DRM 2.0"},
        {"meco", "additional metadata container", "ISO"},
        {"mehd", "movie extends header box", "ISO"},
        {"mere", "metabox relation", "ISO"},
        {"meta", "Metadata container", "ISO"},
        {"mfhd", "movie fragment header", "ISO"},
        {"mfra", "Movie fragment random access", "ISO"},
        {"mfro", "Movie fragment random access offset", "ISO"},
        {"minf", "media information container", "ISO"},
        {"mjhd", "reserved for MPEG-J Stream header", "MP4v1"},
        {"moof", "movie fragment", "ISO"},
        {"moov", "container for all the meta-data", "ISO"},
        {"mvcg", "Multiview group", "NALu Video"},
        {"mvci", "Multiview Information", "NALu Video"},
        {"mvex", "movie extends box", "ISO"},
        {"mvhd", "movie header, overall declarations", "ISO"},
        {"mvra", "Multiview Relation Attribute", "NALu Video"},
        {"nmhd", "Null media header, overall information (some tracks only)", "ISO"},
        {"ochd", "reserved for ObjectContentInfoStream header", "MP4v1"},
        {"odaf", "OMA DRM Access Unit Format", "OMA DRM 2.0"},
        {"odda", "OMA DRM Content Object", "OMA DRM 2.0"},
        {"odhd", "reserved for ObjectDescriptorStream header", "MP4v1"},
        {"odhe", "OMA DRM Discrete Media Headers", "OMA DRM 2.0"},
        {"odrb", "OMA DRM Rights Object", "OMA DRM 2.0"},
        {"odrm", "OMA DRM Container", "OMA DRM 2.0"},
        {"odtt", "OMA DRM Transaction Tracking", "OMA DRM 2.0"},
        {"ohdr", "OMA DRM Common headers", "OMA DRM 2.0"},
        {"padb", "sample padding bits", "ISO"},
        {"paen", "Partition Entry", "ISO"},
        {"pclr", "palette which maps a single component in index space to a multiple- component image", "JPEG2000"},
        {"pdin", "Progressive download information", "ISO"},
        {"pitm", "primary item reference", "ISO"},
        {"pnot", "Reserved", "ISO"},
        {"prft", "Producer reference time", "ISO"},
        {"pssh", "Protection system specific header", "ISO Common Encryption"},
        {"res ", "grid resolution", "JPEG2000"},
        {"resc", "grid resolution at which the image was captured", "JPEG2000"},
        {"resd", "default grid resolution at which the image should be displayed", "JPEG2000"},
        {"rinf", "restricted scheme information box", "ISO"},
        {"saio", "Sample auxiliary information offsets", "ISO"},
        {"saiz", "Sample auxiliary information sizes", "ISO"},
        {"sbgp", "Sample to Group box", "ISO"},
        {"schi", "scheme information box", "ISO"},
        {"schm", "scheme type box", "ISO"},
        {"sdep", "Sample dependency", "NALu Video"},
        {"sdhd", "reserved for SceneDescriptionStream header", "MP4v1"},
        {"sdtp", "Independent and Disposable Samples Box", "ISO"},
        {"sdvp", "SD Profile Box", "SDV"},
        {"segr", "file delivery session group", "ISO"},
        {"senc", "Sample specific encryption data", "ISO Common Encryption"},
        {"sgpd", "Sample group definition box", "ISO"},
        {"sidx", "Segment Index Box", "3GPP"},
        {"sinf", "protection scheme information box", "ISO"},
        {"skip", "free space", "ISO"},
        {"smhd", "sound media header, overall information (sound track only)", "ISO"},
        {"srmb", "System Renewability Message", "DVB"},
        {"srmc", "System Renewability Message container", "DVB"},
        {"srpp", "STRP Process", "ISO"},
        {"ssix", "Sub-sample index", "ISO"},
        {"stbl", "sample table box, container for the time/space map", "ISO"},
        {"stco", "chunk offset, partial data-offset information", "ISO"},
        {"stdp", "sample degradation priority", "ISO"},
        {"sthd", "Subtitle Media Header Box", "ISO"},
        {"stpp", "XML Subtitle Sample Entry", "ISO 14496-12 14496-30"},
        {"strd", "Sub-track definition", "ISO"},
        {"stri", "Sub-track information", "ISO"},
        {"stsc", "sample-to-chunk, partial data-offset information", "ISO"},
        {"stsd", "sample descriptions (codec types, initialization etc.)", "ISO"},
        {"stsg", "Sub-track sample grouping", "ISO"},
        {"stsh", "shadow sync sample table", "ISO"},
        {"stss", "sync sample table (random access points)", "ISO"},
        {"stsz", "sample sizes (framing)", "ISO"},
        {"stts", "(decoding) time-to-sample", "ISO"},
        {"styp", "Segment Type Box", "3GPP"},
        {"stz2", "compact sample sizes (framing)", "ISO"},
        {"subs", "Sub-sample information", "ISO"},
        {"swtc", "Multiview Group Relation", "NALu Video"},
        {"tfad", "Track fragment adjustment box", "3GPP"},
        {"tfdt", "Track fragment decode time", "ISO"},
        {"tfhd", "Track fragment header", "ISO"},
        {"tfma", "Track fragment media adjustment box", "3GPP"},
        {"tfra", "Track fragment radom access", "ISO"},
        {"tibr", "Tier Bit rate", "NALu Video"},
        {"tiri", "Tier Information", "NALu Video"},
        {"tkhd", "Track header, overall information about the track", "ISO"},
        {"traf", "Track fragment", "ISO"},
        {"trak", "container for an individual track or stream", "ISO"},
        {"tref", "track reference container", "ISO"},
        {"trex", "track extends defaults", "ISO"},
        {"trgr", "Track grouping information", "ISO"},
        {"trik", "Facilitates random access and trick play modes", "DECE"},
        {"trun", "track fragment run", "ISO"},
        {"udta", "user-data", "ISO"},
        {"uinf", "a tool by which a vendor may provide access to additional information associated with a UUID", "JPEG2000"},
        {"UITS", "Unique Identifier Technology Solution", "Universal Music Group"},
        {"ulst", "a list of UUID’s", "JPEG2000"},
        {"url ", "a URL", "JPEG2000"},
        {"uuid", "user-extension box", "ISO"},
        {"vmhd", "video media header, overall information (video track only)", "ISO"},
        {"vwdi", "Multiview Scene Information", "NALu Video"},
        {"xml ", "XML container", "ISO"},
    };

    for (int i = 0; i < sizeof(atoms) / sizeof(atoms[0]); i++) {
        if (memcmp(name, atoms[i].name, 4) == 0) {
            return atoms[i].desc;
        }
    }

    return "";
}

typedef void (*mp4_box_func)(const uint8_t *p, size_t len, int depth);
static void mp4_box_btrt_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsd_sample_audio_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsd_sample_video_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsd_avcC_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_frma_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_ftyp_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_mfhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_mvhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_iods_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_mdhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_hdlr_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_tfhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_tkhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_trun_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_ctab_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_schm_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_senc_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stpp_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_mime_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsd_hvcC_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stts_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_ctts_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_saio_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_saiz_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsc_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stsz_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stco_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_stss_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_subs_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_tenc_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_uuid_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_vmhd_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_mdat_print(const uint8_t *p, size_t len, int depth);
static void mp4_box_trex_print(const uint8_t *p, size_t len, int depth);
static void mp4_hexdump(const uint8_t *p, size_t len, int depth);

static mp4_box_func
mp4_box_printer_get(const uint8_t *p)
{
    static struct {
        const char *type;
        mp4_box_func func;
    } box_map[] = {
        {"btrt", mp4_box_btrt_print},
        {"ctab", mp4_print},
        {"enca", mp4_box_stsd_sample_audio_print},
        {"encv", mp4_box_stsd_sample_video_print},
        {"frma", mp4_box_frma_print},
        {"ftyp", mp4_box_ftyp_print},
        {"styp", mp4_box_ftyp_print},
        {"mfhd", mp4_box_mfhd_print},
        {"moof", mp4_print},
        {"moov", mp4_print},
        {"mp4a", mp4_box_stsd_sample_audio_print},
        {"mvhd", mp4_box_mvhd_print},
        {"iods", mp4_box_iods_print},
        {"mdhd", mp4_box_mdhd_print},
        {"hdlr", mp4_box_hdlr_print},
        {"tfhd", mp4_box_tfhd_print},
        {"trak", mp4_print},
        {"tkhd", mp4_box_tkhd_print},
        {"trun", mp4_box_trun_print},
        {"ctab", mp4_box_ctab_print},
        {"mdia", mp4_print},
        {"minf", mp4_print},
        {"stbl", mp4_print},
        {"traf", mp4_print},
        {"schi", mp4_print},
        {"schm", mp4_box_schm_print},
        {"senc", mp4_box_senc_print},
        {"stsd", mp4_box_stsd_print},
        {"stpp", mp4_box_stpp_print},
        {"mime", mp4_box_mime_print},
        {"avc1", mp4_box_stsd_sample_video_print},
        {"avcC", mp4_box_stsd_avcC_print},
        {"hev1", mp4_box_stsd_sample_video_print},
        {"hvcC", mp4_box_stsd_hvcC_print},
        {"stts", mp4_box_stts_print},
        {"ctts", mp4_box_ctts_print},
        {"sinf", mp4_print},
        {"saio", mp4_box_saio_print},
        {"saiz", mp4_box_saiz_print},
        {"stsc", mp4_box_stsc_print},
        {"stsz", mp4_box_stsz_print},
        {"stco", mp4_box_stco_print},
        {"stss", mp4_box_stss_print},
        {"subs", mp4_box_subs_print},
        {"tenc", mp4_box_tenc_print},
        {"uuid", mp4_box_uuid_print},
        {"vmhd", mp4_box_vmhd_print},
        {"mdat", mp4_box_mdat_print},
        {"mvex", mp4_print},
        {"trex", mp4_box_trex_print},
    };

    for (int i = 0; i < sizeof(box_map) / sizeof(box_map[0]); i++) {
        if (memcmp(p, box_map[i].type, 4) == 0) {
            return box_map[i].func;
        }
    }

    return mp4_hexdump;
}

static void
mp4_print(const uint8_t *buf, size_t len, int depth)
{
    const uint8_t *p = buf;
    const uint8_t *end = buf + len;
    mp4_box_func func = NULL;

    while (p < end) {
        uint64_t box_size = get_u32(p);
        uint8_t box_type[5] = {0};
        strncpy((char *)box_type, (const char *)p + 4, 4);
        const uint8_t *box_data = p + 8;

        if (box_size == 1) {
            box_size = get_u64(box_data);
            box_data = p + 16;
        }

        printf("%s--- Offset: %zu Length: %zu Type: %s\n", indent(depth, 1), p - g_content_buf, box_size, box_type);
        printf("%s  Description: %s\n", indent(depth + 1, 0), get_box_desc(box_type));
        func = mp4_box_printer_get(box_type);
        func(box_data, box_size - (box_data - p), depth + 1);

        p += box_size;
    }
}

static void
mp4_box_btrt_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:                 %u\n", indent(depth, 0), p[0]);
    mp4_hexdump(p, len, depth);
}

static void
mp4_box_stsd_sample_audio_print(const uint8_t *p, size_t len, int depth)
{
    // General sample decription
    printf("%s  Reserved:             %.2x%.2x%.2x%.2x%.2x%.2x\n", indent(depth, 0), p[0], p[1], p[2], p[3], p[4], p[5]);
    printf("%s  Data reference index: %u\n", indent(depth, 0), get_u16(p + 6));

    p += 8;

    // Version
    // A 16-bit integer that holds the sample description version (currently 0 or 1).
    //
    // Revision level
    // A 16-bit integer that must be set to 0.
    //
    // Vendor
    // A 32-bit integer that must be set to 0.
    //
    // Number of channels
    // A 16-bit integer that indicates the number of sound channels used by the sound sample.
    // Set to 1 for monaural sounds, 2 for stereo sounds.
    // Higher numbers of channels are not supported.
    //
    // Sample size (bits)
    // A 16-bit integer that specifies the number of bits in each uncompressed sound sample.
    // Allowable values are 8 or 16. Formats using more than 16 bits per sample set this field to 16 and use sound description version 1.
    //
    // Compression ID
    // A 16-bit integer that must be set to 0 for version 0 sound descriptions.
    // This may be set to –2 for some version 1 sound descriptions; see Redefined Sample Tables.
    //
    // Packet size
    // A 16-bit integer that must be set to 0.
    //
    // Sample rate
    // A 32-bit unsigned fixed-point number (16.16) that indicates the rate at which the sound samples were obtained.
    // The integer portion of this number should match the media’s time scale.
    // Many older version 0 files have values of 22254.5454 or 11127.2727, but most files have integer values, such as 44100.
    // Sample rates greater than 2^16 are not supported.
    //
    // Version 0 of the sound description format assumes uncompressed audio in 'raw ' or 'twos' format, 1 or 2 channels, 8 or 16 bits per sample, and a compression ID of 0.
    uint16_t version = get_u16(p);
    printf("%s  Version:              %u\n", indent(depth, 0), version);
    printf("%s  Revision level:       %u\n", indent(depth, 0), get_u16(p + 2));
    printf("%s  Vendor:               %x\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Number of Channels:   %u\n", indent(depth, 0), get_u16(p + 8));
    printf("%s  Sample Size:          %u\n", indent(depth, 0), get_u16(p + 10));
    printf("%s  Compression ID:       %u\n", indent(depth, 0), get_u16(p + 12));
    printf("%s  Packet Size:          %u\n", indent(depth, 0), get_u16(p + 14));
    printf("%s  Sample Rate:          %u\n", indent(depth, 0), get_u32(p + 16));

    if (version == 0) {
        mp4_print(p + 20, len - 28, depth);
    }
}

static void
mp4_box_stsd_sample_video_print(const uint8_t *p, size_t len, int depth)
{
    // General sample decription
    printf("%s  Reserved:             %.2x%.2x%.2x%.2x%.2x%.2x\n", indent(depth, 0), p[0], p[1], p[2], p[3], p[4], p[5]);
    printf("%s  Data reference index: %u\n", indent(depth, 0), get_u16(p + 6));
    p += 8;

    // Video sample description
    // Version
    // A 16-bit integer indicating the version number of the compressed data.
    // This is set to 0, unless a compressor has changed its data format.
    printf("%s  Version:          %u\n", indent(depth, 0), get_u16(p));

    // Revision level
    // A 16-bit integer that must be set to 0.
    printf("%s  Revision level:   %u\n", indent(depth, 0), get_u16(p + 2));

    // Vendor
    // A 32-bit integer that specifies the developer of the compressor that generated the compressed data.
    // Often this field contains 'appl' to indicate Apple, Inc.
    printf("%s  Vendor:           %x\n", indent(depth, 0), get_u32(p + 4));

    // Temporal quality
    // A 32-bit integer containing a value from 0 to 1023 indicating the degree of temporal compression.
    printf("%s  Temporal Quality: %u\n", indent(depth, 0), get_u32(p + 8));

    // Spatial quality
    // A 32-bit integer containing a value from 0 to 1024 indicating the degree of spatial compression.
    printf("%s  Spatial Quality:  %u\n", indent(depth, 0), get_u32(p + 12));

    // Width
    // A 16-bit integer that specifies the width of the source image in pixels.
    printf("%s  Width:            %u\n", indent(depth, 0), get_u16(p + 16));

    // Height
    // A 16-bit integer that specifies the height of the source image in pixels.
    printf("%s  Heigth:           %u\n", indent(depth, 0), get_u16(p + 18));

    // Horizontal resolution
    // A 32-bit fixed-point number containing the horizontal resolution of the image in pixels per inch.
    printf("%s  Horizontal PPI:   %u\n", indent(depth, 0), get_u32(p + 20));

    // Vertical resolution
    // A 32-bit fixed-point number containing the vertical resolution of the image in pixels per inch.
    printf("%s  Vertical PPI:     %u\n", indent(depth, 0), get_u32(p + 24));

    // Data size
    // A 32-bit integer that must be set to 0.
    printf("%s  Data Size:        %u\n", indent(depth, 0), get_u32(p + 28));

    // Frame count
    // A 16-bit integer that indicates how many frames of compressed data are stored in each sample. Usually set to 1.
    printf("%s  Frame Count:      %u\n", indent(depth, 0), get_u16(p + 32));

    // Compressor name
    // A 32-byte Pascal string containing the name of the compressor that created the image, such as "jpeg".
    printf("%s  Compressor:       %s\n", indent(depth, 0), p + 34 + 1);

    // Depth
    // A 16-bit integer that indicates the pixel depth of the compressed image.
    // Values of 1, 2, 4, 8 ,16, 24, and 32 indicate the depth of color images.
    // The value 32 should be used only if the image contains an alpha channel.
    // Values of 34, 36, and 40 indicate 2-, 4-, and 8-bit grayscale,
    // respectively, for grayscale images.
    printf("%s  Depth:            %x\n", indent(depth, 0), get_u16(p + 68));

    // Color table ID
    // A 16-bit integer that identifies which color table to use.
    // If this field is set to -1, the default color table should be used for the specified depth.
    // For all depths below 16 bits per pixel, this indicates a standard Macintosh color table for the specified depth.
    // Depths of 16, 24, and 32 have no color table.
    // If the color table ID is set to 0, a color table is contained within the sample description itself.
    // The color table immediately follows the color table ID field in the sample description.
    // See Color Table Atoms for a complete description of a color table.
    printf("%s  Color Table ID:   %x\n", indent(depth, 0), get_u16(p + 70));

    mp4_print(p + 70, len - 78, depth);
    // mp4_hexdump(p+72, len - 78, depth);
}

static mp4_box_func mdat_printer = NULL;
static void
mp4_box_mdat_h264_nal_print(const uint8_t *p, size_t len, int depth)
{
    // nal_unit( NumBytesInNALunit ) {
    //   forbidden_zero_bit   f(1)
    //   nal_ref_idc          u(2)
    //   nal_unit_type        u(5)

    const uint8_t nal_ref_idc = (p[0] >> 5) & 0x03;
    const uint8_t nal_unit_type = p[0] & 0x1f;
    char *typestr = NULL;
    bool hexdump = false;

    switch (nal_unit_type) {
    case 1:
        typestr = "Non-IDR";
        break;
    case 2:
        typestr = "DPA";
        break;
    case 3:
        typestr = "DPB";
        break;
    case 4:
        typestr = "DPC";
        break;
    case 5:
        typestr = "IDR";
        break;
    case 6:
        typestr = "SEI";
        hexdump = true;
        break;
    case 7:
        typestr = "SPS";
        hexdump = true;
        break;
    case 8:
        typestr = "PPS";
        hexdump = true;
        break;
    case 9:
        typestr = "AUD";
        break;
    case 10:
        typestr = "End Of Sequence";
        break;
    case 11:
        typestr = "End Of Stream";
        break;
    case 12:
        typestr = "Filler";
        break;
    case 13:
        typestr = "SPS Ext";
        break;
    case 19:
        typestr = "Aux Slice";
        break;
    default:
        typestr = "Unknown";
        break;
    }

    printf("%s  nal_ref_idc:    %u\n", indent(depth, 0), nal_ref_idc);
    printf("%s  nal_unit_type:  %u (%s)\n", indent(depth, 0), nal_unit_type, typestr);
    if (hexdump) {
        mp4_hexdump(p, len, depth);
    }
}

static void
mp4_box_mdat_h264_print(const uint8_t *p, size_t len, int depth)
{
    const uint8_t *p_end = p + len;
    while (p < p_end) {
        uint32_t nal_length = get_u32(p);

        printf("%s--- Offset: %zu Length %u Type: H264 NAL\n", indent(depth, 1), p - g_content_buf, nal_length);
        mp4_box_mdat_h264_nal_print(p + 4, nal_length, depth + 1);
        p += nal_length + 4;
    }
}

static void
mp4_box_stsd_avcC_print(const uint8_t *p, size_t len, int depth)
{
    mp4_hexdump(p, len, depth);
    mdat_printer = mp4_box_mdat_h264_print;
}

static void
mp4_box_frma_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Data Format: %c%c%c%c\n", indent(depth, 0), p[0], p[1], p[2], p[3]);
}

static void
mp4_box_ftyp_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Major brand:   %c%c%c%c\n", indent(depth, 0), p[0], p[1], p[2], p[3]);
    printf("%s  Minor version: %u\n", indent(depth, 0), get_u32(p + 4));
    for (const uint8_t *pp = p + 8; pp < p + len; pp += 4) {
        printf("%s  Compability brand: %c%c%c%c\n", indent(depth, 0), pp[0], pp[1], pp[2], pp[3]);
    }
}

static void
mp4_box_mfhd_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Sequence Number: %u\n", indent(depth, 0), get_u32(p + 4));
}

static void
mp4_box_mvhd_print(const uint8_t *p, size_t len, int depth)
{
    mp4_hexdump(p, 128, depth);

    printf("%s  Version:            %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:              0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Creation time:      %u\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Modification time:  %u\n", indent(depth, 0), get_u32(p + 8));
    printf("%s  Time scale:         %u\n", indent(depth, 0), get_u32(p + 12));
    printf("%s  Duration:           %u\n", indent(depth, 0), get_u32(p + 16));
    printf("%s  Preferred rate:     %u\n", indent(depth, 0), get_u32(p + 20));
    printf("%s  Preferred volume:   %u\n", indent(depth, 0), get_u16(p + 24));
    // printf("%s  Matrix structure:   %u\n",indent(depth, 0), get_u32(p+2));
    printf("%s  Preview time:       %u\n", indent(depth, 0), get_u32(p + 72));
    printf("%s  Preview duration:   %u\n", indent(depth, 0), get_u32(p + 76));
    printf("%s  Poster time:        %u\n", indent(depth, 0), get_u32(p + 80));
    printf("%s  Selection time:     %u\n", indent(depth, 0), get_u32(p + 84));
    printf("%s  Selection duration: %u\n", indent(depth, 0), get_u32(p + 88));
    printf("%s  Current Time:       %u\n", indent(depth, 0), get_u32(p + 92));
    printf("%s  Next track ID       %u\n", indent(depth, 0), get_u32(p + 96));
}

static void
mp4_box_iods_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:            %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:              0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    mp4_hexdump(p, len, depth);
}

static void
mp4_box_mdhd_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:            %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:              0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Creation time:      %u\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Modification time:  %u\n", indent(depth, 0), get_u32(p + 8));
    printf("%s  Time scale:         %u\n", indent(depth, 0), get_u32(p + 12));
    printf("%s  Duration:           %u\n", indent(depth, 0), get_u32(p + 16));
    printf("%s  Language:           %u\n", indent(depth, 0), get_u16(p + 20));
    printf("%s  Quality:            %u\n", indent(depth, 0), get_u16(p + 22));
}

static void
mp4_box_hdlr_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:                %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:                  0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Component type:         %u\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Component subtype:      %.*s\n", indent(depth, 0), 4, p + 8);
    printf("%s  Component manufacturer: %.*s\n", indent(depth, 0), 4, p + 8);
    printf("%s  Component flags:        %u\n", indent(depth, 0), get_u32(p + 12));
    printf("%s  Component flags mask:   %u\n", indent(depth, 0), get_u32(p + 16));
    printf("%s  Component name:         %u\n", indent(depth, 0), get_u32(p + 12));
}

static void
mp4_box_tfhd_optional_print(const uint8_t *p, const uint8_t *end, int depth, uint32_t flags)
{
    if (flags & 1) {
        if (p + 8 > end) {
            return;
        }
        printf("%s  Base Data Offset:        %lu\n", indent(depth, 0), get_u64(p));
        p += 8;
    }

    if (flags & 2) {
        if (p + 4 > end) {
            return;
        }
        printf("%s  Sample Desc Index:       %d\n", indent(depth, 0), get_u32(p));
        p += 4;
    }

    if (flags & 8) {
        if (p + 4 > end) {
            return;
        }
        printf("%s  Default Sample Duration: %d\n", indent(depth, 0), get_u32(p));
        p += 4;
    }

    if (flags & 0x10) {
        if (p + 4 > end) {
            return;
        }
        printf("%s  Default Sample Size:     %d\n", indent(depth, 0), get_u32(p));
        p += 4;
    }

    if (flags & 0x20) {
        if (p + 4 > end) {
            return;
        }
        printf("%s  Default Sample Flags:    0x%x\n", indent(depth, 0), get_u32(p));
        p += 4;
    }
}

static void
mp4_box_tfhd_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags = get_u24(p + 1);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.6x\n", indent(depth, 0), flags);
    printf("%s  Track ID:   0x%d\n", indent(depth, 0), get_u32(p + 4));
    mp4_box_tfhd_optional_print(p + 8, p + len, depth, flags);
}

static void
mp4_box_tkhd_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:            %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:              0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Creation time:      %u\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Modification time:  %u\n", indent(depth, 0), get_u32(p + 8));
    printf("%s  Track ID:           %u\n", indent(depth, 0), get_u32(p + 12));
    // Reserved 4 bytes
    printf("%s  Duration:           %u\n", indent(depth, 0), get_u32(p + 20));
    // Reserved 8 bytes
    printf("%s  Layer:              %u\n", indent(depth, 0), get_u16(p + 32));
    printf("%s  Alternate groupe:   %u\n", indent(depth, 0), get_u16(p + 34));
    printf("%s  Volume:             %u\n", indent(depth, 0), get_u16(p + 36));
    // Reserved 2 bytes

    // printf("%s  Matrix structure:   %u\n",indent(depth, 0), get_u32(p+2));
    printf("%s  Track width:        %u\n", indent(depth, 0), get_u32(p + 76));
    printf("%s  Track height:       %u\n", indent(depth, 0), get_u32(p + 80));
}

static void
mp4_table_print(const char *name, const char *header, const uint8_t *p, int esize, int width, int num, int depth)
{
    printf("%s  %s:\n", indent(depth, 0), name);
    printf("%s             %s\n", indent(depth, 0), header);
    int offset = 0;
    for (int i = 0; i < num; i++) {
        printf("%s      %3d:", indent(depth, 0), i + 1);
        for (int j = 0; j < width; j++) {
            printf("   %6u", get_u32(p + offset));
            offset += esize;
        }
        printf("\n");
    }
}

static void
mp4_box_trun_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags = get_u24(p + 1);
    const uint32_t samples = get_u32(p + 4);
    char table_hdr[128] = {0};
    int table_fields = 0;

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.6x\n", indent(depth, 0), flags);
    printf("%s  Samples:     %u\n", indent(depth, 0), samples);

    p += 8;

    if (flags & 1) {
        printf("%s  Data Offset: %u\n", indent(depth, 0), get_u32(p));
        p += 4;
    }

    if (flags & 0x100) {
        strcat(table_hdr, "Duration    ");
        table_fields++;
    }

    if (flags & 0x200) {
        strcat(table_hdr, "Size    ");
        table_fields++;
    }

    if (flags & 0x400) {
        strcat(table_hdr, "Flags    ");
        table_fields++;
    }

    if (flags & 0x800) {
        strcat(table_hdr, "Composition-Time-Offset (CTS)");
        table_fields++;
    }

    mp4_table_print("Sample Table", table_hdr, p, 4, table_fields, samples, depth);
}

static void
mp4_box_ctab_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Color Table Seed   %x\n", indent(depth, 0), get_u32(p));
    printf("%s  Color Table Flags  %u\n", indent(depth, 0), get_u16(p + 4));
    printf("%s  Color Table Size   %u\n", indent(depth, 0), get_u16(p + 6));
}

static void
mp4_box_schm_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags = get_u24(p + 1);

    printf("%s  Version:       %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:         0x%.6x\n", indent(depth, 0), flags);

    printf("%s  Scheme Type:    %c%c%c%c\n", indent(depth, 0), p[4], p[5], p[6], p[7]);
    printf("%s  Scheme Version: %u\n", indent(depth, 0), get_u32(p + 8));

    if (flags & 0x1) {
        printf("%s  Scheme URI: TODO\n", indent(depth, 0));
    }
}

static const char *
mp4_hexstr(const uint8_t *p, size_t len)
{
    static char buffer[128];

    int n = 0;
    for (size_t i = 0; i < len; i++) {
        n += snprintf(buffer + n, sizeof(buffer) - n, " %.2x", p[i]);
    }

    return buffer;
}

static void
mp4_box_senc_print(const uint8_t *p, size_t len, int depth)
{
    const uint32_t flags = get_u24(p + 1);
    uint32_t sample_count = get_u32(p + 4);
    uint32_t i = 0;
    uint32_t j = 0;

    //aligned(8) class SampleEncryptionBox
    //extends FullBox(‘senc’, version=0, flags)
    //{
    //    unsigned int(32) sample_count;
    //    {
    //        unsigned int(Per_Sample_IV_Size*8) InitializationVector;
    //        if (flags & 0x000002)
    //        {
    //            unsigned int(16) subsample_count;
    //            {
    //                unsigned int(16) BytesOfClearData;
    //                unsigned int(32) BytesOfProtectedData;
    //            } [ subsample_count ]
    //        }
    //    }[ sample_count ]
    //}

    printf("%s  Version:      %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:        0x%.6x\n", indent(depth, 0), flags);
    printf("%s  Sample Count: %u\n", indent(depth, 0), sample_count);

    p += 8;
    //  printf("%s  Sample\n", indent(depth, 0), j);
    for (i = 0; i < sample_count; i++) {
        // Print 8 bytes IV
        printf("%s Sample: %3u\n", indent(depth, 1), i);
        printf("%s  IV:     %s\n", indent(depth + 1, 0), mp4_hexstr(p, 8));
        p += 8;

        if (flags & 0x000002) {
            uint32_t sub_sample_count = get_u16(p);
            printf("%s  Subsample Count: %u\n", indent(depth + 1, 1), sub_sample_count);
            p += 2;
            printf("%s  Subsample  BytesOfClear  BytesOfProtectedData\n", indent(depth + 2, 0));
            for (j = 0; j < sub_sample_count; j++) {
                printf("%s  %9d  %12u  %20u\n", indent(depth + 2, 0), j, get_u16(p), get_u32(p + 2));
                p += 6;
            }
        }
    }
}

static void
mp4_box_stsd_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags = get_u24(p + 1);
    const uint32_t num_entries = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.6x\n", indent(depth, 0), flags);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num_entries);

    // Print recursive boxes
    mp4_print(p + 8, len - 8, depth);
}

// 14496-12:2015 12.6.3.2
static void
mp4_box_stpp_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Reference Index: %u\n", indent(depth, 0), get_u16(p + 6));

    do {
        const uint8_t *pp = p;
        const uint8_t *end = p + len;

        pp += 8;
        printf("%s  Namespace:       %s\n", indent(depth, 0), (const char *)pp);
        pp += strlen((const char *)pp) + 1;
        if (pp >= end) {
            break;
        }
        printf("%s  Scheme Location: %s\n", indent(depth, 0), (const char *)pp);
        pp += strlen((const char *)pp) + 1;
        if (pp >= end) {
            break;
        }
        printf("%s  Aux Mime Type:   %s\n", indent(depth, 0), (const char *)pp);
        pp += strlen((const char *)pp) + 1;
        if (pp >= end) {
            break;
        }
        mp4_print(pp, end - pp, depth);
    } while (0);

    mp4_hexdump(p, len, depth);
}

// Mime box according 14496-12 2015 Amd 2
static void
mp4_box_mime_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version and Flags: %u\n", indent(depth, 0), get_u32(p));
    printf("%s  Content Type: %.*s\n", indent(depth, 0), (int)(len - 4), (const char *)p + 4);
}

static void
mp4_box_mdat_hevc_nal_print(const uint8_t *p, size_t len, int depth)
{
    //  nal_unit_header( ) {        Descriptor
    //      forbidden_zero_bit          f(1)
    //      nal_unit_type               u(6)
    //      nuh_layer_id                u(6)
    //      nuh_temporal_id_plus1       u(3)
    //  }

    uint8_t type = (p[0] >> 1) & 0x3f;
    uint8_t layer_id = ((p[0] & 1) << 5) | (p[1] >> 3);
    uint8_t temporal_id_plus1 = p[1] & 0x3;

    char *typestr = NULL;
    bool hexdump = false;

    switch (type) {
    case 0:
    case 1:
        typestr = "SLICE non-TSA non-STSA";
        break;
    case 2:
    case 3:
        typestr = "SLICE TSA";
        break;
    case 8:
        typestr = "SLICE RASL_N";
        break;
    case 9:
        typestr = "SLICE RASL_R";
        break;
    case 19:
    case 20:
        typestr = "IDR";
        break;
    case 32:
        typestr = "VPS";
        hexdump = true;
        break;
    case 33:
        typestr = "SPS";
        hexdump = true;
        break;
    case 34:
        typestr = "PPS";
        hexdump = true;
        break;
    case 35:
        typestr = "AUD";
        hexdump = true;
        break;
    case 39:
    case 40:
        typestr = "SEI";
        break;
    default:
        typestr = "UND";
        break;
    }

    printf("%s  nal_unit_type:        %u (%s)\n", indent(depth, 0), type, typestr);
    printf("%s  nuh_layer_id:         %u\n", indent(depth, 0), layer_id);
    printf("%s  nuh_temporal_id_plus1 %u\n", indent(depth, 0), temporal_id_plus1);
    if (hexdump) {
        mp4_hexdump(p, len, depth);
    }
}

static void
mp4_box_mdat_hevc_print(const uint8_t *p, size_t len, int depth)
{

    const uint8_t *p_end = p + len;

    while (p < p_end) {
        uint32_t nal_length = get_u32(p);
        p += 4;

        printf("%s--- Offset: %zu Length %u Type: HEVC NAL\n", indent(depth, 1), p - g_content_buf, nal_length);
        mp4_box_mdat_hevc_nal_print(p, nal_length, depth + 1);
        p += nal_length;
    }
}

static void
mp4_box_stsd_hvcC_print(const uint8_t *p, size_t len, int depth)
{
    mp4_hexdump(p, len, depth);
    mdat_printer = mp4_box_mdat_hevc_print;
}

static void
mp4_box_stts_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    mp4_table_print("Time-to-sample table", "Sample count | Sample duration", p + 8, 4, 2, num, depth);
}

static void
mp4_box_ctts_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);
    printf("%s  Composition-offset table:\n", indent(depth, 0));
    printf("%s        Sample count | Composition offset\n", indent(depth, 0));

    mp4_table_print("Composition-offset table", "Sample count | Composition offset", p + 8, 4, 2, num, depth);
}

static void
mp4_box_saio_print(const uint8_t *p, size_t len, int depth)
{
    /*
     * aligned(8) class SampleAuxiliaryInformationOffsetsBox
     * extends FullBox(‘saio’, version, flags)
     * {
     *     if (flags & 1) {
     *         unsigned int(32) aux_info_type;
     *         unsigned int(32) aux_info_type_parameter;
     *     }
     *     unsigned int(32) entry_count;
     *     if ( version == 0 ) {
     *         unsigned int(32) offset[ entry_count ];
     *     }
     *     else {
     *         unsigned int(64) offset[ entry_count ];
     *     }
     * }
     *
     */
    uint8_t version = p[0];
    uint32_t flags = get_u24(p + 1);
    uint8_t entry_count = 0;

    printf("%s  Version:                  %u\n", indent(depth, 0), version);
    printf("%s  Flags:                    0x%.6x\n", indent(depth, 0), flags);
    p += 4;

    if (flags & 1) {
        printf("%s  Aux Info Type:            %c%c%c%c\n", indent(depth, 0), p[0], p[1], p[2], p[3]);
        printf("%s  Aux Info Type Parameter:  %u\n", indent(depth, 0), get_u32(p + 4));
        p += 8;
    }

    entry_count = get_u32(p);
    p += 4;
    if (version == 0) {
        printf("%s  Entry     Offset\n", indent(depth, 0));
        for (int i = 0; i < entry_count; i++) {
            printf("%s  %3d:       %u\n", indent(depth, 0), i, get_u32(p));
            p += 4;
        }
    } else {
        printf("%s  Entry     Offset\n", indent(depth, 0));
        for (int i = 0; i < entry_count; i++) {
            printf("%s  %3d:       %llu\n", indent(depth, 0), i, (long long unsigned)get_u64(p));
            p += 8;
        }
    }
}

static void
mp4_box_saiz_print(const uint8_t *p, size_t len, int depth)
{
    /*
     * aligned(8) class SampleAuxiliaryInformationSizesBox
     *  extends FullBox(‘saiz’, version = 0, flags)
     *  {
     *       if (flags & 1) {
     *           unsigned int(32) aux_info_type;
     *           unsigned int(32) aux_info_type_parameter;
     *       }
     *   unsigned int(8) default_sample_info_size;
     *   unsigned int(32) sample_count;
     *   if (default_sample_info_size == 0) {
     *       unsigned int(8) sample_info_size[ sample_count ];
     *   }
     * }
     *
     **/
    uint32_t flags = get_u24(p + 1);

    printf("%s  Version:                  %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:                    0x%.6x\n", indent(depth, 0), flags);
    p += 4;
    if (flags & 1) {
        printf("%s  Aux Info Type:            %c%c%c%c\n", indent(depth, 0), p[0], p[1], p[2], p[3]);
        printf("%s  Aux Info Type Parameter:  %u\n", indent(depth, 0), get_u32(p + 4));
        p += 8;
    }
    uint8_t default_sample_info_size = p[0];
    uint8_t sample_count = get_u32(p + 1);
    printf("%s  Default Sample Info Size: %u\n", indent(depth, 0), default_sample_info_size);
    printf("%s  Sample Count:             %u\n", indent(depth, 0), sample_count);

    p += 5;
    if (default_sample_info_size == 0) {
        printf("%s  Sample     Sample Info Size\n", indent(depth, 0));
        for (int i = 0; i < sample_count; i++) {
            printf("%s  %3d:           %.2u\n", indent(depth, 0), i, p[i]);
        }
    }
}

static void
mp4_box_stsc_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    mp4_table_print("Composition-offset table", "First chunk | Samples per chunk | Sample Description ID", p + 8, 4, 3, num, depth);
}

static void
mp4_box_stsz_print(const uint8_t *p, size_t len, int depth)
{
    const int sample_size = get_u32(p + 4);
    const int num = get_u32(p + 8);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Sample size: %u\n", indent(depth, 0), sample_size);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    mp4_table_print("Sample size table", "Size", p + 12, 4, 1, num, depth);
}

static void
mp4_box_stco_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    mp4_table_print("Sample size table", "Size", p + 8, 4, 1, num, depth);
}

static void
mp4_box_stss_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);

    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    mp4_table_print("Sync sample table", "Size", p + 8, 4, 1, num, depth);
}

// 14496-12 8.7.7
static void
mp4_box_subs_print(const uint8_t *p, size_t len, int depth)
{
    const int num = get_u32(p + 4);
    const int version = p[0];

    printf("%s  Version:     %u\n", indent(depth, 0), version);
    printf("%s  Flags:       0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Num Entries: %u\n", indent(depth, 0), num);

    p += 8;

    int last_entry = 0;
    for (int entry = 0; entry < num; entry++) {
        const int delta = get_u32(p);
        const int sub_count = get_u16(p + 4);

        p += 6;
        if (sub_count) {
            last_entry += delta;
            printf("%s      %3u      Size     Prio  Discardable\n", indent(depth, 0), last_entry);
        }
        for (int sub = 0; sub < sub_count; sub++) {
            printf("%s      %3d:", indent(depth, 0), sub + 1);
            if (version == 1) {
                printf("   %6u", get_u32(p));
                p += 4;
            } else {
                printf("   %6u", get_u16(p));
                p += 2;
            }
            printf("   %6u", p[0]);
            printf("   %6u", p[1]);
            printf("\n");
            p += 6;
        }
    }
}

static void
mp4_box_tenc_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags = get_u24(p + 1);
    uint32_t is_encrypted = get_u24(p + 4);

    printf("%s  Version:       %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:         0x%.6x\n", indent(depth, 0), flags);
    printf("%s  IsEncrypted:   %u\n", indent(depth, 0), is_encrypted);
    printf("%s  IV_Size:       %u\n", indent(depth, 0), p[7]);

    printf("%s  Default Key ID %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n", indent(depth, 0), p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15], p[16], p[17], p[18], p[19], p[20], p[21], p[22], p[23]);
}

static void
mp4_box_uuid_sample_encryption(const uint8_t *p, size_t len, int depth)
{
    const uint32_t flags = get_u24(p + 1);

    printf("%s  Name:        Sample Encryption Box\n", indent(depth, 0));
    printf("%s  Version:     %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:       0x%.6x\n", indent(depth, 0), flags);

    p += 4;
    if (flags & 1) {
        printf("%s  AlgorithmID: 0x%.2x%.2x%.2x\n", indent(depth, 0), p[0], p[1], p[2]);
        printf("%s  IV Sizes:       %u\n", indent(depth, 0), p[3]);
        printf("%s  Key ID:\n", indent(depth, 0));
        mp4_hexdump(p + 4, 16, depth);
        p += 20;
    }

    uint32_t num_entries = get_u32(p);
    p += 4;

    printf("%s  Num Entries: %u\n", indent(depth, 0), num_entries);
    printf("%s  Entry           IV             Entries\n", indent(depth, 0));

    uint8_t iv_size = 8;
    for (uint32_t i = 0; i < num_entries; i++) {
        if (iv_size) {
            printf("%s Entry: %3u\n", indent(depth, 1), i);
            printf("%s  IV:     %s\n", indent(depth + 1, 0), mp4_hexstr(p, 8));
            p += iv_size;
        }

        if (flags & 2) {
            uint16_t num_sub_samples = get_u16(p);
            printf("%s  Sub-Entries Count: %u\n", indent(depth + 1, 1), num_sub_samples);
            p += 2;

            printf("%s  Sub-Entry  BytesOfClear  BytesOfProtectedData\n", indent(depth + 2, 0));
            for (uint32_t j = 0; j < num_sub_samples; j++) {
                printf("%s  %9d  %12u  %20u\n", indent(depth + 2, 0), j, get_u16(p), get_u32(p + 2));
                p += 6;
            }
        }
    }
}

// Print tfrf uuid box containing fragment forward references, see
// https://msdn.microsoft.com/en-us/library/ff469633.aspx
static void
mp4_box_uuid_tfrf(const uint8_t *p, size_t len, int depth)
{
    const uint32_t flags = get_u24(p + 1);
    const uint8_t fragment_count = p[4];

    printf("%s  Name:           tfrf\n", indent(depth, 0));
    printf("%s  Version:        %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:          0x%.6x\n", indent(depth, 0), flags);
    printf("%s  Fragment Count: %u\n", indent(depth, 0), fragment_count);
    printf("%s    Fragment    Time              Duration\n", indent(depth, 0));
    for (unsigned int i = 0; i < fragment_count; i++) {
        if (flags & 1) {
            printf("%s    %u           %16u  %u\n", indent(depth, 0), i, get_u32(p + 5), get_u32(p + 9));
        } else {
            printf("%s    %u           %16lu  %lu\n", indent(depth, 0), i, get_u64(p + 5), get_u64(p + 13));
        }
    }
}

static void
mp4_box_uuid_print(const uint8_t *p, size_t len, int depth)
{
    struct {
        const uint8_t uuid[16];
        mp4_box_func func;
    } uuids[] = {
        {{0xa2, 0x39, 0x4f, 0x52, 0x5a, 0x9b, 0x4f, 0x14, 0xa2, 0x44, 0x6c, 0x42, 0x7c, 0x64, 0x8d, 0xf4}, mp4_box_uuid_sample_encryption},
        {{0xd4, 0x80, 0x7e, 0xf2, 0xca, 0x39, 0x46, 0x95, 0x8e, 0x54, 0x26, 0xcb, 0x9e, 0x46, 0xa7, 0x9f}, mp4_box_uuid_tfrf}
        // {{0x6d, 0x1d, 0x9b, 0x05, 0x42, 0xd5, 0x44, 0xe6, 0x80, 0xe2, 0x14, 0x1d, 0xaf, 0xf7, 0x57, 0xb2}, mp4tree_hexdump}
    };

    for (int i = 0; i < sizeof(uuids) / sizeof(uuids[0]); i++) {
        if (memcmp(uuids[i].uuid, p, 16) == 0) {
            return uuids[i].func(p + 16, len - 16, depth);
        }
    }

    return mp4_hexdump(p, len, depth);
}

static void
mp4_box_vmhd_print(const uint8_t *p, size_t len, int depth)
{
    printf("%s  Version:      %u\n", indent(depth, 0), p[0]);
    printf("%s  Flags:        0x%.2x%.2x%.2x\n", indent(depth, 0), p[1], p[2], p[3]);
    printf("%s  Graphic mode: %u\n", indent(depth, 0), get_u16(p + 4));
    printf("%s  Opcolor       TODO\n", indent(depth, 0));
}

static void
mp4_box_mdat_print(const uint8_t *p, size_t len, int depth)
{
    if (mdat_printer != NULL) {
        mdat_printer(p, len, depth);
    } else {
        // TODO: using h264 as default for now
        mp4_box_mdat_hevc_print(p, len, depth);
    }
}

// 14496-12:2015 8.8.3
struct trex_flags {
    uint32_t reserved : 4;
    uint32_t is_leading : 2;
    uint32_t sample_depends_on : 2;
    uint32_t sample_is_depended_on : 2;
    uint32_t sample_has_redundancy : 2;
    uint32_t sample_padding_value : 3;
    uint32_t sample_is_non_sync_sample : 1;
    uint32_t sample_degradation_priority : 16;
};

static void
mp4_box_trex_print(const uint8_t *p, size_t len, int depth)
{
    uint32_t flags_value = get_u32(p + 16);
    const struct trex_flags *flags = (const struct trex_flags *)&flags_value;

    printf("%s  Track ID:                %u\n", indent(depth, 0), get_u32(p));
    printf("%s  Default sample description index: %u\n", indent(depth, 0), get_u32(p + 4));
    printf("%s  Default sample duration: %u\n", indent(depth, 0), get_u32(p + 8));
    printf("%s  Default sample size:     %u\n", indent(depth, 0), get_u32(p + 12));

    printf("%s  Is Leading:              %u\n", indent(depth, 0), flags->is_leading);
    printf("%s  Sample Depends On:       %u\n", indent(depth, 0), flags->sample_depends_on);
    printf("%s  Sample Is Depended On:   %u\n", indent(depth, 0), flags->sample_is_depended_on);
    printf("%s  Sample Has Redundancy:   %u\n", indent(depth, 0), flags->sample_has_redundancy);
    printf("%s  Sample Padding Value:    %u\n", indent(depth, 0), flags->sample_padding_value);
    printf("%s  Sample Is Non-Sync:      %u\n", indent(depth, 0), flags->sample_is_non_sync_sample);
    printf("%s  Sample Degradation Prio: %u\n", indent(depth, 0), flags->sample_degradation_priority);
}

static void
mp4_hexdump(const uint8_t *p, size_t len, int depth)
{
    size_t truncated_len = 0;

    // String templates to keep pretty alignment
    const char *empty_hex = "                                                ";
    const char *empty_txt = "                ";
    char hex_buf[1024];
    char txt_buf[1024];
    sprintf(hex_buf, "%s", empty_hex);
    sprintf(txt_buf, "%s", empty_txt);

    if (len > 128) {
        truncated_len = len - 128;
        len = 128;
    }

    size_t line_pos = 0;
    for (size_t i = 0; i < len; i++) {
        line_pos = i & 0x0f;

        // Print hex output to temporary buffer...
        char hex[4];
        sprintf(hex, "%02x ", p[i]);
        // ...and use strncpy to avoid null-termination
        strncpy(&hex_buf[line_pos * 3], hex, 3);

        if (isprint(p[i])) {
            // A printable character
            txt_buf[line_pos] = p[i];
        } else {
            // Replace non-printable characters with a dot.
            txt_buf[line_pos] = '.';
        }

        if (i % 16 == 0) {
            printf("%s  %.4zx    ", indent(depth, 0), i);
        }

        if (line_pos == 15) {
            printf("%s |%s|\n", hex_buf, txt_buf);
            sprintf(hex_buf, "%s", empty_hex);
            sprintf(txt_buf, "%s", empty_txt);
        }
    }

    if (line_pos != 15) {
        printf("%s |%s|\n", hex_buf, txt_buf);
    }

    if (truncated_len) {
        printf("%s   ... %zu bytes truncated\n", indent(depth, 0), truncated_len);
    }
}
