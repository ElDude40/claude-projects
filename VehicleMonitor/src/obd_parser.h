// Pure parsing helpers for ELM327 / OBD-II responses.
//
// This module has no Arduino dependencies so it can be unit-tested on a PC
// (see test/test_obd_parser). All functions take the raw text returned by the
// adapter (everything before the '>' prompt).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace obd {

// Reassembles an ELM327 response into one hex string per ECU message.
// Handles single-frame lines, ISO 15765 multi-frame responses ("014", "0:..",
// "1:.."), stray spaces and status lines such as "SEARCHING...".
std::vector<std::string> splitMessages(const std::string& raw);

// True if the response contains no usable data ("NO DATA", "?", errors...).
bool isErrorResponse(const std::string& raw);

// Finds the data bytes of a Mode 01 response for `pid`.
// Returns false if the PID was not in the response.
bool parsePidBytes(const std::string& raw, uint8_t pid, std::vector<uint8_t>& out);

// Decodes a 2-byte DTC (e.g. 0x01, 0x33) into "P0133".
std::string decodeDtc(uint8_t a, uint8_t b);

// Parses the reply to Mode 03 (stored), 07 (pending) or 0A (permanent).
// `isCan` must be true for protocols 6-9 / A-C, where a count byte follows the
// mode byte. Duplicate codes reported by several ECUs are removed.
std::vector<std::string> parseDtcResponse(const std::string& raw, uint8_t mode, bool isCan);

// Extracts the 17-character VIN from a Mode 09 PID 02 response.
std::string parseVin(const std::string& raw);

// Parses a "supported PIDs" bitmask reply (PID 00, 20, 40 ...). Sets the
// matching entries of `supported` (index = PID number, 256 entries).
bool parseSupportedPids(const std::string& raw, uint8_t basePid, bool supported[256]);

}  // namespace obd
