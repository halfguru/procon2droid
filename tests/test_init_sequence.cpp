// Tests for the USB init packet sequence (17 packets).
#include "common/init_sequence.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace procon2droid;

TEST_CASE("Init sequence has exactly 17 packets", "[init_sequence]")
{
    REQUIRE(kInitSequence.size() == 17);
}

TEST_CASE("First packet is CMD_03 with correct size", "[init_sequence]")
{
    REQUIRE(kInitSequence[0].data.size() == kCmd03.size());
    REQUIRE(kInitSequence[0].data[0] == 0x03);
    REQUIRE(kInitSequence[0].data[1] == 0x91);
}

TEST_CASE("CMD_LED packet has correct content", "[init_sequence]")
{
    REQUIRE(kInitSequence[16].data.size() == kCmdLed.size());
    REQUIRE(kInitSequence[16].data[0] == 0x09);
}

TEST_CASE("CMD_HAPTICS packet exists and has correct size", "[init_sequence]")
{
    bool found = false;
    for (const auto& pkt : kInitSequence)
    {
        if (pkt.data.size() == kCmdHaptics.size() && pkt.data[0] == 0x03 && pkt.data[3] == 0x0a)
        {
            found = true;
            break;
        }
    }
    REQUIRE(found);
}

TEST_CASE("Init sequence total packet count constant matches", "[init_sequence]")
{
    REQUIRE(kInitSequenceLength == 17);
}

TEST_CASE("All packets have non-zero size", "[init_sequence]")
{
    for (const auto& pkt : kInitSequence)
    {
        REQUIRE(pkt.data.size() > 0);
    }
}
