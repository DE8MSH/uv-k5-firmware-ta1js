/* Experimental AIS bit checker, independent of the BK4819 RF path.
 * HDLC flags, bit de-stuffing and CRC-16/X-25 are used by AIS.
 * No bit is considered received until a GMSK demodulator supplies symbols.
 */
#include "ais_bits.h"
#include <string.h>

#define AIS_MAX_FRAME_BYTES (AIS_MAX_RAW_BITS / 8u)

static uint16_t ais_crc16(const uint8_t *p, uint16_t len)
{
    uint16_t crc = 0xffffu;
    while (len--) {
        crc ^= *p++;
        for (uint8_t b = 0; b < 8; ++b)
            crc = (crc & 1u) ? (uint16_t)((crc >> 1) ^ 0x8408u) : (uint16_t)(crc >> 1);
    }
    return (uint16_t)~crc;
}

static uint32_t ais_read_msb(const uint8_t *bytes, unsigned start, unsigned count)
{
    uint32_t result = 0;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned pos = start + i;
        result = (result << 1) | ((bytes[pos / 8] >> (7u - (pos % 8u))) & 1u);
    }
    return result;
}

static void ais_check_frame(AIS_BitChecker *s, uint16_t nbits)
{
    uint8_t data[AIS_MAX_FRAME_BYTES] = { 0 };
    uint16_t destuffed = 0;
    uint8_t ones = 0;

    /* Flags have already been removed; destuff the enclosed raw bit stream. */
    for (uint16_t i = 0; i < nbits; ++i) {
        const uint8_t bit = s->raw[i] & 1u;
        if (ones == 5u) {
            if (bit != 0u) return; /* HDLC abort / malformed stuffing */
            ones = 0;
            continue;
        }
        if (destuffed >= AIS_MAX_RAW_BITS) return;
        if (bit) data[destuffed / 8u] |= (uint8_t)(1u << (destuffed % 8u));
        ++destuffed;
        ones = bit ? (uint8_t)(ones + 1u) : 0u;
    }

    /* At least the AIS message type, repeat, 30-bit MMSI and two FCS bytes. */
    if ((destuffed % 8u) != 0u || destuffed < 56u) return;
    const uint16_t nbytes = destuffed / 8u;
    const uint16_t payload = nbytes - 2u;
    const uint16_t received = (uint16_t)data[payload] |
                              (uint16_t)((uint16_t)data[payload + 1u] << 8u);
    if (ais_crc16(data, payload) != received) {
        ++s->crc_bad;
        return;
    }

    const uint8_t type = (uint8_t)ais_read_msb(data, 0, 6);
    if (type < 1u || type > 27u) return;
    ++s->crc_ok;
    if (s->callback) {
        AIS_FrameInfo info = {
            .type = type,
            .mmsi = ais_read_msb(data, 8, 30),
            .payload_bytes = payload
        };
        s->callback(&info, s->user);
    }
}

void AIS_BitsInit(AIS_BitChecker *s, AIS_FrameCallback cb, void *user)
{
    memset(s, 0, sizeof(*s));
    s->callback = cb;
    s->user = user;
}

void AIS_BitsPushNRZI(AIS_BitChecker *s, uint8_t symbol)
{
    symbol &= 1u;
    if (!s->have_symbol) {
        s->have_symbol = 1u;
        s->last_symbol = symbol;
        return;
    }

    /* AIS NRZI: logical 0 = transition; logical 1 = no transition. */
    const uint8_t bit = (symbol == s->last_symbol) ? 1u : 0u;
    s->last_symbol = symbol;
    s->shift = (uint8_t)((s->shift << 1) | bit);

    if (s->in_frame && s->nbits < AIS_MAX_RAW_BITS)
        s->raw[s->nbits++] = bit;

    if (s->shift == 0x7eu) {
        ++s->flags;
        if (s->in_frame && s->nbits >= 8u)
            ais_check_frame(s, (uint16_t)(s->nbits - 8u));
        s->in_frame = 1u;   /* closing flag can also start the next frame */
        s->nbits = 0;
    } else if (s->in_frame && s->nbits >= AIS_MAX_RAW_BITS) {
        s->in_frame = 0u;  /* overflow: wait for the next flag */
        s->nbits = 0;
    }
}
