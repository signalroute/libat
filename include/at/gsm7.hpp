// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Signalroute

/**
 * @file gsm7.hpp
 * @brief GSM-7 basic character set encoder/decoder (3GPP TS 23.038 Table 1).
 *
 * Provides:
 *   at::gsm7::is_gsm7(text)          — true if every UTF-8 code point in
 *                                       text is representable in the GSM-7
 *                                       basic character set.
 *   at::gsm7::encode(text)            — pack a UTF-8 string into 7-bit packed
 *                                       bytes (one byte per 8/7 chars).
 *   at::gsm7::decode(packed, n_chars) — unpack n_chars GSM-7 septets back to
 *                                       a UTF-8 string.
 *
 * The full 128-entry GSM-7 Basic Character Set LUT maps each 7-bit GSM-7
 * position to its Unicode code point.  Characters outside the basic table
 * (extension table, e.g. '{', '}', '[', ']', '@', '€' …) are not supported
 * by is_gsm7 / encode — callers must fall back to UCS-2.
 *
 * Thread safety: all functions are stateless and re-entrant.
 */

#ifndef AT_GSM7_HPP
#define AT_GSM7_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace at::gsm7 {

// ============================================================================
// GSM-7 Basic Character Set LUT  (3GPP TS 23.038 Table 1)
// Index = 7-bit GSM-7 code, value = Unicode code point.
// ============================================================================

/// @cond INTERNAL
inline constexpr std::array<uint32_t, 128> kTable = {{
    //  GSM7  Unicode  Char
    0x0040, // 0x00   @
    0x00A3, // 0x01   £
    0x0024, // 0x02   $
    0x00A5, // 0x03   ¥
    0x00E8, // 0x04   è
    0x00E9, // 0x05   é
    0x00F9, // 0x06   ù
    0x00EC, // 0x07   ì
    0x00F2, // 0x08   ò
    0x00C7, // 0x09   Ç
    0x000A, // 0x0A   LF
    0x00D8, // 0x0B   Ø
    0x00F8, // 0x0C   ø
    0x000D, // 0x0D   CR
    0x00C5, // 0x0E   Å
    0x00E5, // 0x0F   å
    0x0394, // 0x10   Δ
    0x005F, // 0x11   _
    0x03A6, // 0x12   Φ
    0x0393, // 0x13   Γ
    0x039B, // 0x14   Λ
    0x03A9, // 0x15   Ω
    0x03A0, // 0x16   Π
    0x03A8, // 0x17   Ψ
    0x03A3, // 0x18   Σ
    0x0398, // 0x19   Θ
    0x039E, // 0x1A   Ξ
    0x001B, // 0x1B   ESC  (extension table escape — not a printable char)
    0x00C6, // 0x1C   Æ
    0x00E6, // 0x1D   æ
    0x00DF, // 0x1E   ß
    0x00C9, // 0x1F   É
    0x0020, // 0x20   SP
    0x0021, // 0x21   !
    0x0022, // 0x22   "
    0x0023, // 0x23   #
    0x00A4, // 0x24   ¤
    0x0025, // 0x25   %
    0x0026, // 0x26   &
    0x0027, // 0x27   '
    0x0028, // 0x28   (
    0x0029, // 0x29   )
    0x002A, // 0x2A   *
    0x002B, // 0x2B   +
    0x002C, // 0x2C   ,
    0x002D, // 0x2D   -
    0x002E, // 0x2E   .
    0x002F, // 0x2F   /
    0x0030, // 0x30   0
    0x0031, // 0x31   1
    0x0032, // 0x32   2
    0x0033, // 0x33   3
    0x0034, // 0x34   4
    0x0035, // 0x35   5
    0x0036, // 0x36   6
    0x0037, // 0x37   7
    0x0038, // 0x38   8
    0x0039, // 0x39   9
    0x003A, // 0x3A   :
    0x003B, // 0x3B   ;
    0x003C, // 0x3C   <
    0x003D, // 0x3D   =
    0x003E, // 0x3E   >
    0x003F, // 0x3F   ?
    0x00A1, // 0x40   ¡
    0x0041, // 0x41   A
    0x0042, // 0x42   B
    0x0043, // 0x43   C
    0x0044, // 0x44   D
    0x0045, // 0x45   E
    0x0046, // 0x46   F
    0x0047, // 0x47   G
    0x0048, // 0x48   H
    0x0049, // 0x49   I
    0x004A, // 0x4A   J
    0x004B, // 0x4B   K
    0x004C, // 0x4C   L
    0x004D, // 0x4D   M
    0x004E, // 0x4E   N
    0x004F, // 0x4F   O
    0x0050, // 0x50   P
    0x0051, // 0x51   Q
    0x0052, // 0x52   R
    0x0053, // 0x53   S
    0x0054, // 0x54   T
    0x0055, // 0x55   U
    0x0056, // 0x56   V
    0x0057, // 0x57   W
    0x0058, // 0x58   X
    0x0059, // 0x59   Y
    0x005A, // 0x5A   Z
    0x00C4, // 0x5B   Ä
    0x00D6, // 0x5C   Ö
    0x00D1, // 0x5D   Ñ
    0x00DC, // 0x5E   Ü
    0x00A7, // 0x5F   §
    0x00BF, // 0x60   ¿
    0x0061, // 0x61   a
    0x0062, // 0x62   b
    0x0063, // 0x63   c
    0x0064, // 0x64   d
    0x0065, // 0x65   e
    0x0066, // 0x66   f
    0x0067, // 0x67   g
    0x0068, // 0x68   h
    0x0069, // 0x69   i
    0x006A, // 0x6A   j
    0x006B, // 0x6B   k
    0x006C, // 0x6C   l
    0x006D, // 0x6D   m
    0x006E, // 0x6E   n
    0x006F, // 0x6F   o
    0x0070, // 0x70   p
    0x0071, // 0x71   q
    0x0072, // 0x72   r
    0x0073, // 0x73   s
    0x0074, // 0x74   t
    0x0075, // 0x75   u
    0x0076, // 0x76   v
    0x0077, // 0x77   w
    0x0078, // 0x78   x
    0x0079, // 0x79   y
    0x007A, // 0x7A   z
    0x00E4, // 0x7B   ä
    0x00F6, // 0x7C   ö
    0x00F1, // 0x7D   ñ
    0x00FC, // 0x7E   ü
    0x00E0, // 0x7F   à
}};
/// @endcond

// ============================================================================
// Internal helpers
// ============================================================================

namespace detail {

/// Encode a Unicode code point as UTF-8, appending to @p out.
inline void cp_to_utf8(uint32_t cp, std::string& out) {
    if (cp < 0x80) {
        out += static_cast<char>(cp);
    } else if (cp < 0x800) {
        out += static_cast<char>(0xC0 | (cp >> 6));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else if (cp < 0x10000) {
        out += static_cast<char>(0xE0 | (cp >> 12));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    } else {
        out += static_cast<char>(0xF0 | (cp >> 18));
        out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (cp & 0x3F));
    }
}

/**
 * Decode one UTF-8 code point from @p sv starting at @p pos.
 * Advances @p pos past the decoded sequence.
 * Returns 0xFFFFFFFF on invalid/truncated input.
 */
inline uint32_t utf8_next(std::string_view sv, size_t& pos) {
    if (pos >= sv.size()) return 0xFFFFFFFF;

    const auto b0 = static_cast<uint8_t>(sv[pos]);

    if (b0 < 0x80) {
        ++pos;
        return b0;
    }

    uint32_t cp;
    size_t extra;

    if ((b0 & 0xE0) == 0xC0) {
        cp    = b0 & 0x1F;
        extra = 1;
    } else if ((b0 & 0xF0) == 0xE0) {
        cp    = b0 & 0x0F;
        extra = 2;
    } else if ((b0 & 0xF8) == 0xF0) {
        cp    = b0 & 0x07;
        extra = 3;
    } else {
        ++pos;
        return 0xFFFFFFFF; // invalid leading byte
    }

    if (pos + extra >= sv.size()) return 0xFFFFFFFF;

    for (size_t i = 0; i < extra; ++i) {
        const auto cb = static_cast<uint8_t>(sv[++pos]);
        if ((cb & 0xC0) != 0x80) return 0xFFFFFFFF;
        cp = (cp << 6) | (cb & 0x3F);
    }
    ++pos;
    return cp;
}

/// Find the GSM-7 position for a Unicode code point, or 0xFF if not present.
inline uint8_t unicode_to_gsm7(uint32_t cp) noexcept {
    for (uint8_t i = 0; i < 128; ++i) {
        if (kTable[i] == cp) return i;
    }
    return 0xFF;
}

} // namespace detail

// ============================================================================
// Public API
// ============================================================================

/**
 * Returns true if every Unicode code point in the UTF-8 string @p text is
 * representable in the GSM-7 basic character set (3GPP TS 23.038 Table 1).
 *
 * The ESC code point (U+001B, GSM-7 position 0x1B) is included in the basic
 * table but is not a printable character; callers that need strict printable
 * validation should handle it separately.
 */
[[nodiscard]] inline bool is_gsm7(std::string_view text) {
    size_t pos = 0;
    while (pos < text.size()) {
        const uint32_t cp = detail::utf8_next(text, pos);
        if (cp == 0xFFFFFFFF) return false;
        if (detail::unicode_to_gsm7(cp) == 0xFF) return false;
    }
    return true;
}

/**
 * Encode @p text (UTF-8) into 7-bit packed bytes.
 *
 * Each character is looked up in the GSM-7 basic table and its 7-bit code is
 * packed sequentially: character N occupies bits [N*7 + 6 : N*7] of the
 * output byte stream.  Padding bits in the final byte are zero.
 *
 * @throws std::invalid_argument if any code point is not in the GSM-7 table.
 */
[[nodiscard]] inline std::vector<uint8_t> encode(std::string_view text) {
    // Collect GSM-7 septets
    std::vector<uint8_t> septets;
    septets.reserve(text.size());

    size_t pos = 0;
    while (pos < text.size()) {
        const uint32_t cp  = detail::utf8_next(text, pos);
        const uint8_t  gsm = detail::unicode_to_gsm7(cp);
        if (gsm == 0xFF) {
            throw std::invalid_argument("character not in GSM-7 basic table");
        }
        septets.push_back(gsm);
    }

    // 7-bit packing
    const size_t n = septets.size();
    const size_t packed_len = (n * 7 + 7) / 8; // ceil(n * 7 / 8)
    std::vector<uint8_t> out(packed_len, 0);

    for (size_t i = 0; i < n; ++i) {
        const size_t bit_offset = i * 7;
        const size_t byte_idx   = bit_offset / 8;
        const size_t bit_shift  = bit_offset % 8;

        out[byte_idx] |= static_cast<uint8_t>(septets[i] << bit_shift);
        if (bit_shift > 1) {
            out[byte_idx + 1] |= static_cast<uint8_t>(septets[i] >> (8 - bit_shift));
        }
    }

    return out;
}

/**
 * Decode @p n_chars 7-bit GSM-7 septets from @p packed and return the
 * resulting UTF-8 string.
 *
 * @param packed  Packed byte buffer as produced by encode().
 * @param n_chars Number of septets to unpack (must satisfy
 *                n_chars * 7 <= packed.size() * 8).
 */
[[nodiscard]] inline std::string decode(std::span<const uint8_t> packed,
                                        size_t n_chars) {
    std::string out;
    out.reserve(n_chars);

    for (size_t i = 0; i < n_chars; ++i) {
        const size_t bit_offset = i * 7;
        const size_t byte_idx   = bit_offset / 8;
        const size_t bit_shift  = bit_offset % 8;

        // Unified extraction works for both single-byte and cross-byte cases.
        uint8_t val = static_cast<uint8_t>(packed[byte_idx] >> bit_shift);
        if (byte_idx + 1 < packed.size()) {
            val |= static_cast<uint8_t>(packed[byte_idx + 1] << (8 - bit_shift));
        }
        val &= 0x7F;

        detail::cp_to_utf8(kTable[val], out);
    }

    return out;
}

} // namespace at::gsm7

#endif // AT_GSM7_HPP
