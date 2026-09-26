/*
 * Host-only deterministic test for the experimental AIS NRZI/HDLC/CRC checker.
 * No radio, no microphone, no on-air assertions.
 *
 * gcc -std=c11 -Wall -Wextra -Werror -Iapp \
 *     app/ais_bits.c utils/ais_bits_test.c -o /tmp/ais_bits_test
 * /tmp/ais_bits_test
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../app/ais_bits.h"

static uint8_t nrzi_level;
static unsigned delivered;
static AIS_FrameInfo last_frame;

static void on_frame(const AIS_FrameInfo *frame, void *user)
{
    (void)user;
    last_frame = *frame;
    ++delivered;
}

static uint16_t make_fcs(const uint8_t *p, unsigned n)
{
    uint16_t crc = 0xffffu;
    for (unsigned i = 0; i < n; ++i) {
        crc ^= p[i];
        for (unsigned j = 0; j < 8; ++j)
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0x8408u)
                             : (uint16_t)(crc >> 1);
    }
    return (uint16_t)~crc;
}

static void push_bit(AIS_BitChecker *s, uint8_t bit)
{
    if (bit == 0u) nrzi_level ^= 1u;
    AIS_BitsPushNRZI(s, nrzi_level);
}

static void flag(AIS_BitChecker *s)
{
    for (unsigned i = 0; i < 8; ++i)
        push_bit(s, (uint8_t)((0x7eu >> i) & 1u));
}

static void send_frame(AIS_BitChecker *s, const uint8_t *p, unsigned n)
{
    unsigned ones = 0;
    /* 24-bit alternating training sequence and HDLC start flag. */
    for (unsigned i = 0; i < 24; ++i)
        push_bit(s, (uint8_t)(i & 1u));
    flag(s);
    for (unsigned i = 0; i < n; ++i) {
        for (unsigned b = 0; b < 8; ++b) {
            uint8_t bit = (uint8_t)((p[i] >> b) & 1u);
            push_bit(s, bit);
            ones = bit ? ones + 1u : 0u;
            if (ones == 5u) {
                push_bit(s, 0u); /* transmitter's stuffed zero */
                ones = 0u;
            }
        }
    }
    flag(s);
}

static void build_type1(uint8_t *frame, uint32_t mmsi)
{
    memset(frame, 0, 23u);
    frame[0] = 0x04u; /* message type 1, repeat indicator 0 */
    for (unsigned i = 0; i < 30; ++i) {
        unsigned pos = 8u + i;
        unsigned bit = (mmsi >> (29u - i)) & 1u;
        if (bit) frame[pos / 8u] |= (uint8_t)(1u << (7u - (pos % 8u)));
    }
    /* Type-1 position message length: 168 bits / 21 bytes. */
    uint16_t fcs = make_fcs(frame, 21u);
    frame[21] = (uint8_t)fcs;
    frame[22] = (uint8_t)(fcs >> 8u);
}

int main(void)
{
    AIS_BitChecker s;
    uint8_t frame[23];
    const uint32_t mmsi = 123456789u;

    assert(make_fcs((const uint8_t *)"123456789", 9u) == 0x906eu);
    AIS_BitsInit(&s, on_frame, NULL);
    build_type1(frame, mmsi);
    send_frame(&s, frame, sizeof(frame));
    assert(delivered == 1u);
    assert(last_frame.type == 1u);
    assert(last_frame.mmsi == mmsi);
    assert(last_frame.payload_bytes == 21u);
    assert(s.crc_ok == 1u && s.crc_bad == 0u);

    /* Flip one payload bit WITHOUT regenerating the FCS: must be rejected. */
    frame[10] ^= 0x01u;
    send_frame(&s, frame, sizeof(frame));
    assert(delivered == 1u);
    assert(s.crc_ok == 1u && s.crc_bad == 1u);

    /* Independent checker initialized with an inverted initial NRZI level. */
    nrzi_level ^= 1u;
    AIS_BitsInit(&s, on_frame, NULL);
    build_type1(frame, mmsi);
    send_frame(&s, frame, sizeof(frame));
    assert(delivered == 2u && s.crc_ok == 1u);

    printf("PASS: CRC-16/X-25 vector, AIS type 1/MMSI, corrupt FCS rejection, NRZI inversion\n");
    return 0;
}
