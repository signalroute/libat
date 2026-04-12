// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Signalroute

/**
 * @file test_gsm7.cpp
 * @brief Google Test suite for at::gsm7 — GSM-7 basic character set codec.
 *
 * Tests cover:
 *   • is_gsm7() — character set membership
 *   • encode()  — UTF-8 → 7-bit packed bytes
 *   • decode()  — 7-bit packed bytes → UTF-8
 *   • Round-trip identity for ASCII and non-ASCII GSM-7 characters
 */

#include <at/gsm7.hpp>

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================================
// is_gsm7 tests
// ============================================================================

TEST(Gsm7IsGsm7, EmptyStringIsGsm7) {
    EXPECT_TRUE(at::gsm7::is_gsm7(""));
}

TEST(Gsm7IsGsm7, BasicAsciiLetters) {
    EXPECT_TRUE(at::gsm7::is_gsm7("Hello World"));
    EXPECT_TRUE(at::gsm7::is_gsm7("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    EXPECT_TRUE(at::gsm7::is_gsm7("abcdefghijklmnopqrstuvwxyz"));
}

TEST(Gsm7IsGsm7, Digits) {
    EXPECT_TRUE(at::gsm7::is_gsm7("0123456789"));
}

TEST(Gsm7IsGsm7, CommonPunctuation) {
    EXPECT_TRUE(at::gsm7::is_gsm7(" !\"#%&'()*+,-./:;<=>?"));
}

TEST(Gsm7IsGsm7, AtSignIsGsm7) {
    // '@' maps to GSM-7 position 0x00 (not ASCII 0x40)
    EXPECT_TRUE(at::gsm7::is_gsm7("@"));
}

TEST(Gsm7IsGsm7, DollarSignIsGsm7) {
    EXPECT_TRUE(at::gsm7::is_gsm7("$"));
}

TEST(Gsm7IsGsm7, UnderscoreIsGsm7) {
    EXPECT_TRUE(at::gsm7::is_gsm7("_"));
}

TEST(Gsm7IsGsm7, NonAsciiGsm7Characters) {
    // Latin-supplement characters present in the basic table
    EXPECT_TRUE(at::gsm7::is_gsm7("\xC3\xA9")); // é  U+00E9
    EXPECT_TRUE(at::gsm7::is_gsm7("\xC3\xA8")); // è  U+00E8
    EXPECT_TRUE(at::gsm7::is_gsm7("\xC2\xA3")); // £  U+00A3
    EXPECT_TRUE(at::gsm7::is_gsm7("\xC3\x9F")); // ß  U+00DF
}

TEST(Gsm7IsGsm7, GreekLettersInTable) {
    EXPECT_TRUE(at::gsm7::is_gsm7("\xCE\x94")); // Δ  U+0394
    EXPECT_TRUE(at::gsm7::is_gsm7("\xCE\xA9")); // Ω  U+03A9
    EXPECT_TRUE(at::gsm7::is_gsm7("\xCE\xA3")); // Σ  U+03A3
}

TEST(Gsm7IsGsm7, ExtendedCharactersAreNotGsm7) {
    // Characters NOT in the basic table (they are in the extension table)
    EXPECT_FALSE(at::gsm7::is_gsm7("{")); // U+007B
    EXPECT_FALSE(at::gsm7::is_gsm7("}")); // U+007D
    EXPECT_FALSE(at::gsm7::is_gsm7("[")); // U+005B
    EXPECT_FALSE(at::gsm7::is_gsm7("]")); // U+005D
    EXPECT_FALSE(at::gsm7::is_gsm7("\\")); // U+005C
    EXPECT_FALSE(at::gsm7::is_gsm7("~")); // U+007E
    EXPECT_FALSE(at::gsm7::is_gsm7("^")); // U+005E — maps to Û in ext table
}

TEST(Gsm7IsGsm7, FullUnicodeBeyondBasicSetIsFalse) {
    EXPECT_FALSE(at::gsm7::is_gsm7("\xE2\x82\xAC")); // € U+20AC (extension only)
    EXPECT_FALSE(at::gsm7::is_gsm7("\xF0\x9F\x98\x80")); // emoji U+1F600
    EXPECT_FALSE(at::gsm7::is_gsm7("\xC3\xA6\xF0\x9F\x98\x80")); // æ + emoji
}

// ============================================================================
// encode tests
// ============================================================================

TEST(Gsm7Encode, EmptyInput) {
    const auto packed = at::gsm7::encode("");
    EXPECT_TRUE(packed.empty());
}

TEST(Gsm7Encode, SingleCharA) {
    // 'A' → GSM-7 0x41.  Single septet: byte = 0x41 (padding 0 in MSB).
    const auto packed = at::gsm7::encode("A");
    ASSERT_EQ(packed.size(), 1u);
    EXPECT_EQ(packed[0], 0x41u);
}

TEST(Gsm7Encode, SingleCharAt) {
    // '@' → GSM-7 0x00.  Packed byte is 0x00.
    const auto packed = at::gsm7::encode("@");
    ASSERT_EQ(packed.size(), 1u);
    EXPECT_EQ(packed[0], 0x00u);
}

TEST(Gsm7Encode, TwoChars_AB) {
    // 'A'=0x41, 'B'=0x42
    // Bit layout:
    //   byte 0: A[6:0] | B[0]<<7  = 0x41 | (0x42<<7 & 0xFF) = 0x41 | 0x00 = 0x41
    //   Wait: 0x42 = 0b01000010, B[0]=0, so byte0 bit7 = 0 → byte0 = 0x41
    //   byte 1: B[6:1] = 0x42>>1 = 0x21
    const auto packed = at::gsm7::encode("AB");
    ASSERT_EQ(packed.size(), 2u);
    EXPECT_EQ(packed[0], 0x41u);
    EXPECT_EQ(packed[1], 0x21u);
}

TEST(Gsm7Encode, EightCharsProduceSevenBytes) {
    // 8 septets × 7 bits = 56 bits = 7 bytes
    const auto packed = at::gsm7::encode("AAAAAAAA");
    EXPECT_EQ(packed.size(), 7u);
}

TEST(Gsm7Encode, HelloWorld) {
    // Known-good encoding of "hello" using GSM-7:
    // h=0x68, e=0x65, l=0x6C, l=0x6C, o=0x6F
    // byte0 = 0x68 | (0x65<<7 & 0xFF) = 0x68 | 0x80 = 0xE8
    // byte1 = 0x65>>1 | (0x6C<<6 & 0xFF) = 0x32 | 0x00 = 0x32 ... (well-known test vector)
    // Use round-trip test instead of hardcoded bytes (encoding tested by decode).
    const auto packed  = at::gsm7::encode("hello");
    const auto decoded = at::gsm7::decode(packed, 5);
    EXPECT_EQ(decoded, "hello");
}

TEST(Gsm7Encode, KnownVector_TestMessage) {
    // "SMS" → GSM-7: S=0x53, M=0x4D, S=0x53
    // byte0 = 0x53 | (0x4D << 7 & 0xFF) = 0x53 | 0x80 = 0xD3
    // byte1 = 0x4D >> 1 | (0x53 << 6 & 0xFF) = 0x26 | 0xC0 = 0xE6  (wrong, recalc)
    //   0x4D=0b01001101 >> 1 = 0b00100110 = 0x26
    //   0x53=0b01010011 << 6 = 0b11000000 (low 8 bits) = 0xC0  → 0x26|0xC0=0xE6
    // byte2 = 0x53 >> 2 = 0x14 (only 5 bits needed, in bits [4:0] of byte2)
    //   0b01010011 >> 2 = 0b00010100 = 0x14
    const auto packed = at::gsm7::encode("SMS");
    ASSERT_EQ(packed.size(), 3u);
    EXPECT_EQ(packed[0], 0xD3u);
    EXPECT_EQ(packed[1], 0xE6u);
    EXPECT_EQ(packed[2], 0x14u);
}

TEST(Gsm7Encode, ThrowsOnNonGsm7Character) {
    EXPECT_THROW(at::gsm7::encode("{"), std::invalid_argument);
    EXPECT_THROW(at::gsm7::encode("Hello \xF0\x9F\x98\x80"), std::invalid_argument);
}

// ============================================================================
// decode tests
// ============================================================================

TEST(Gsm7Decode, EmptyInput) {
    const std::vector<uint8_t> empty;
    const auto text = at::gsm7::decode(empty, 0);
    EXPECT_TRUE(text.empty());
}

TEST(Gsm7Decode, SingleSeptet_A) {
    const std::vector<uint8_t> packed = {0x41};
    const auto text = at::gsm7::decode(packed, 1);
    EXPECT_EQ(text, "A");
}

TEST(Gsm7Decode, SingleSeptet_At) {
    // GSM-7 0x00 → '@'
    const std::vector<uint8_t> packed = {0x00};
    const auto text = at::gsm7::decode(packed, 1);
    EXPECT_EQ(text, "@");
}

TEST(Gsm7Decode, KnownVector_SMS) {
    const std::vector<uint8_t> packed = {0xD3, 0xE6, 0x14};
    const auto text = at::gsm7::decode(packed, 3);
    EXPECT_EQ(text, "SMS");
}

TEST(Gsm7Decode, NonAsciiGsm7ToUtf8) {
    // Encode "é" (U+00E9, GSM-7 0x05) and verify decode produces UTF-8.
    const auto packed  = at::gsm7::encode("\xC3\xA9"); // é
    const auto decoded = at::gsm7::decode(packed, 1);
    EXPECT_EQ(decoded, "\xC3\xA9"); // é in UTF-8
}

TEST(Gsm7Decode, GreekLetterRoundTrip) {
    const std::string text = "\xCE\x94"; // Δ U+0394
    const auto packed  = at::gsm7::encode(text);
    const auto decoded = at::gsm7::decode(packed, 1);
    EXPECT_EQ(decoded, text);
}

// ============================================================================
// Round-trip tests
// ============================================================================

TEST(Gsm7RoundTrip, PureAsciiSentence) {
    const std::string text = "The quick brown fox jumps over the lazy dog.";
    const auto packed  = at::gsm7::encode(text);
    // Expected byte count: ceil(44 * 7 / 8) = ceil(308 / 8) = 39 bytes
    EXPECT_EQ(packed.size(), (text.size() * 7 + 7) / 8);
    const auto decoded = at::gsm7::decode(packed, text.size());
    EXPECT_EQ(decoded, text);
}

TEST(Gsm7RoundTrip, MixedGsm7Chars) {
    // Mix of ASCII and non-ASCII GSM-7 characters
    const std::string text =
        "Hello \xC3\xA9\xC3\xA8\xC2\xA3"; // "Hello éè£"
    ASSERT_TRUE(at::gsm7::is_gsm7(text));
    const auto packed  = at::gsm7::encode(text);
    const auto decoded = at::gsm7::decode(packed, /* n_chars= */ 9);
    EXPECT_EQ(decoded, text);
}

TEST(Gsm7RoundTrip, AllPrintableAsciiGsm7Subset) {
    // Printable ASCII characters that are in the GSM-7 table
    const std::string text =
        " !\"#$%&'()*+,-./0123456789:;<=>?@"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "_";
    ASSERT_TRUE(at::gsm7::is_gsm7(text));
    const auto packed  = at::gsm7::encode(text);
    const auto decoded = at::gsm7::decode(packed, text.size());
    EXPECT_EQ(decoded, text);
}

TEST(Gsm7RoundTrip, SixteenCharsProduces14Bytes) {
    // 16 septets × 7 = 112 bits = 14 bytes
    const std::string text(16, 'A');
    const auto packed = at::gsm7::encode(text);
    EXPECT_EQ(packed.size(), 14u);
    EXPECT_EQ(at::gsm7::decode(packed, 16), text);
}
