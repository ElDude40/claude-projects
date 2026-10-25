// Host-side tests for the OBD-II response parsers.
// Run with:  pio test -e native
#include <unity.h>

#include "dtc_descriptions.h"
#include "obd_parser.h"

using namespace obd;

void setUp() {}
void tearDown() {}

void test_decode_dtc() {
  TEST_ASSERT_EQUAL_STRING("P0133", decodeDtc(0x01, 0x33).c_str());
  TEST_ASSERT_EQUAL_STRING("C0300", decodeDtc(0x43, 0x00).c_str());
  TEST_ASSERT_EQUAL_STRING("B1234", decodeDtc(0x92, 0x34).c_str());
  TEST_ASSERT_EQUAL_STRING("U0100", decodeDtc(0xC1, 0x00).c_str());
  TEST_ASSERT_EQUAL_STRING("P2A0F", decodeDtc(0x2A, 0x0F).c_str());
}

void test_rpm_with_searching_prefix() {
  std::vector<uint8_t> b;
  TEST_ASSERT_TRUE(parsePidBytes("SEARCHING...\r41 0C 1A F8\r\r", 0x0C, b));
  TEST_ASSERT_EQUAL(2, b.size());
  TEST_ASSERT_EQUAL_HEX8(0x1A, b[0]);
  TEST_ASSERT_EQUAL_HEX8(0xF8, b[1]);
}

void test_pid_no_data() {
  std::vector<uint8_t> b;
  TEST_ASSERT_FALSE(parsePidBytes("NO DATA\r", 0x0C, b));
  TEST_ASSERT_TRUE(isErrorResponse("NO DATA\r"));
  TEST_ASSERT_TRUE(isErrorResponse("?\r"));
  TEST_ASSERT_FALSE(isErrorResponse("410D32\r"));
}

void test_dtc_can_single_frame() {
  auto codes = parseDtcResponse("43 02 01 33 03 00\r", 0x03, true);
  TEST_ASSERT_EQUAL(2, codes.size());
  TEST_ASSERT_EQUAL_STRING("P0133", codes[0].c_str());
  TEST_ASSERT_EQUAL_STRING("P0300", codes[1].c_str());
}

void test_dtc_can_none() {
  auto codes = parseDtcResponse("4300\r", 0x03, true);
  TEST_ASSERT_EQUAL(0, codes.size());
}

void test_dtc_can_multi_frame() {
  // 4 codes: P0133 P0300 P0171 P0420
  const char* raw =
      "00A\r"
      "0:43040133030001\r"
      "1:7104200000000000\r\r";
  auto codes = parseDtcResponse(raw, 0x03, true);
  TEST_ASSERT_EQUAL(4, codes.size());
  TEST_ASSERT_EQUAL_STRING("P0133", codes[0].c_str());
  TEST_ASSERT_EQUAL_STRING("P0300", codes[1].c_str());
  TEST_ASSERT_EQUAL_STRING("P0171", codes[2].c_str());
  TEST_ASSERT_EQUAL_STRING("P0420", codes[3].c_str());
}

void test_dtc_legacy_multi_line() {
  const char* raw =
      "43 01 33 03 00 01 71\r"
      "43 04 20 00 00 00 00\r";
  auto codes = parseDtcResponse(raw, 0x03, false);
  TEST_ASSERT_EQUAL(4, codes.size());
  TEST_ASSERT_EQUAL_STRING("P0420", codes[3].c_str());
}

void test_dtc_duplicates_from_two_ecus() {
  auto codes = parseDtcResponse("4301 0420\r4301 0420\r", 0x03, true);
  TEST_ASSERT_EQUAL(1, codes.size());
}

void test_pending_dtc_mode07() {
  auto codes = parseDtcResponse("47 01 01 71\r", 0x07, true);
  TEST_ASSERT_EQUAL(1, codes.size());
  TEST_ASSERT_EQUAL_STRING("P0171", codes[0].c_str());
}

void test_vin_can() {
  const char* raw =
      "014\r"
      "0: 49 02 01 31 47 31\r"
      "1: 4A 43 35 34 34 34 52\r"
      "2: 37 32 35 32 33 36 37\r";
  TEST_ASSERT_EQUAL_STRING("1G1JC5444R7252367", parseVin(raw).c_str());
}

void test_vin_legacy() {
  const char* raw =
      "49 02 01 00 00 00 31\r"
      "49 02 02 47 31 4A 43\r"
      "49 02 03 35 34 34 34\r"
      "49 02 04 52 37 32 35\r"
      "49 02 05 32 33 36 37\r";
  TEST_ASSERT_EQUAL_STRING("1G1JC5444R7252367", parseVin(raw).c_str());
}

void test_supported_pids() {
  bool supported[256] = {false};
  TEST_ASSERT_TRUE(parseSupportedPids("41 00 BE 1F A8 13\r", 0x00, supported));
  TEST_ASSERT_TRUE(supported[0x01]);
  TEST_ASSERT_FALSE(supported[0x02]);
  TEST_ASSERT_TRUE(supported[0x05]);
  TEST_ASSERT_TRUE(supported[0x0C]);
  TEST_ASSERT_TRUE(supported[0x0D]);
  TEST_ASSERT_TRUE(supported[0x20]);
}

void test_dtc_descriptions() {
  TEST_ASSERT_EQUAL_STRING("Catalyst Efficiency Below Threshold (Bank 1)", describeDtc("P0420"));
  TEST_ASSERT_EQUAL_STRING("Manufacturer specific code", describeDtc("P1234"));
  TEST_ASSERT_EQUAL_STRING("Network / communication fault", describeDtc("U0999"));
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_decode_dtc);
  RUN_TEST(test_rpm_with_searching_prefix);
  RUN_TEST(test_pid_no_data);
  RUN_TEST(test_dtc_can_single_frame);
  RUN_TEST(test_dtc_can_none);
  RUN_TEST(test_dtc_can_multi_frame);
  RUN_TEST(test_dtc_legacy_multi_line);
  RUN_TEST(test_dtc_duplicates_from_two_ecus);
  RUN_TEST(test_pending_dtc_mode07);
  RUN_TEST(test_vin_can);
  RUN_TEST(test_vin_legacy);
  RUN_TEST(test_supported_pids);
  RUN_TEST(test_dtc_descriptions);
  return UNITY_END();
}
