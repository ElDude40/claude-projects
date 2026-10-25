#include "obd_parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace obd {

namespace {

bool isHexString(const std::string& s) {
  if (s.empty()) return false;
  for (char c : s) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
  }
  return true;
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
  std::vector<uint8_t> bytes;
  for (size_t i = 0; i + 1 < hex.size(); i += 2) {
    bytes.push_back(static_cast<uint8_t>(std::strtoul(hex.substr(i, 2).c_str(), nullptr, 16)));
  }
  return bytes;
}

std::string byteToHex(uint8_t b) {
  char buf[3];
  std::snprintf(buf, sizeof(buf), "%02X", b);
  return buf;
}

}  // namespace

std::vector<std::string> splitMessages(const std::string& raw) {
  std::vector<std::string> messages;

  std::string multiFrame;
  size_t multiFrameLen = 0;  // expected payload length in bytes, 0 = none open
  auto flushMultiFrame = [&]() {
    if (!multiFrame.empty()) {
      if (multiFrameLen > 0 && multiFrame.size() > multiFrameLen * 2) {
        multiFrame.resize(multiFrameLen * 2);  // drop frame padding
      }
      messages.push_back(multiFrame);
    }
    multiFrame.clear();
    multiFrameLen = 0;
  };

  std::string line;
  auto processLine = [&](std::string l) {
    // Normalise: upper case, no spaces.
    l.erase(std::remove_if(l.begin(), l.end(), [](char c) { return c == ' ' || c == '\t'; }), l.end());
    std::transform(l.begin(), l.end(), l.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
    if (l.empty()) return;
    if (l.find("SEARCHING") != std::string::npos || l.find("BUSINIT") != std::string::npos) return;

    // ISO 15765 multi-frame: a 3-digit byte count, then "0:...", "1:..." lines.
    if (l.size() == 3 && isHexString(l)) {
      flushMultiFrame();
      multiFrameLen = std::strtoul(l.c_str(), nullptr, 16);
      multiFrame.clear();
      return;
    }
    size_t colon = l.find(':');
    if (colon != std::string::npos && colon > 0 && isHexString(l.substr(0, colon))) {
      std::string data = l.substr(colon + 1);
      if (isHexString(data)) multiFrame += data;
      return;
    }
    if (isHexString(l) && l.size() % 2 == 0) {
      flushMultiFrame();
      messages.push_back(l);
    }
  };

  for (char c : raw) {
    if (c == '\r' || c == '\n' || c == '>') {
      processLine(line);
      line.clear();
    } else {
      line += c;
    }
  }
  processLine(line);
  flushMultiFrame();
  return messages;
}

bool isErrorResponse(const std::string& raw) {
  std::string up = raw;
  std::transform(up.begin(), up.end(), up.begin(), [](char c) { return std::toupper(static_cast<unsigned char>(c)); });
  static const char* const kErrors[] = {"NO DATA", "UNABLE TO CONNECT", "CAN ERROR", "BUS ERROR",
                                        "BUS BUSY", "STOPPED", "ERROR", "?"};
  for (const char* e : kErrors) {
    if (up.find(e) != std::string::npos) return true;
  }
  return splitMessages(raw).empty();
}

bool parsePidBytes(const std::string& raw, uint8_t pid, std::vector<uint8_t>& out) {
  const std::string prefix = "41" + byteToHex(pid);
  for (const std::string& msg : splitMessages(raw)) {
    if (msg.compare(0, prefix.size(), prefix) == 0) {
      out = hexToBytes(msg.substr(prefix.size()));
      return !out.empty();
    }
  }
  return false;
}

std::string decodeDtc(uint8_t a, uint8_t b) {
  static const char kSystems[] = {'P', 'C', 'B', 'U'};
  char buf[6];
  std::snprintf(buf, sizeof(buf), "%c%01X%01X%02X", kSystems[(a >> 6) & 0x03], (a >> 4) & 0x03, a & 0x0F, b);
  return buf;
}

std::vector<std::string> parseDtcResponse(const std::string& raw, uint8_t mode, bool isCan) {
  std::vector<std::string> codes;
  const std::string prefix = byteToHex(static_cast<uint8_t>(mode + 0x40));

  for (const std::string& msg : splitMessages(raw)) {
    if (msg.compare(0, prefix.size(), prefix) != 0) continue;
    std::vector<uint8_t> bytes = hexToBytes(msg);

    size_t start = 1;  // skip the response mode byte
    size_t limit = bytes.size();
    if (isCan) {
      if (bytes.size() < 2) continue;
      size_t count = bytes[1];
      start = 2;
      limit = std::min(bytes.size(), start + count * 2);
    }

    for (size_t i = start; i + 1 < limit; i += 2) {
      if (bytes[i] == 0 && bytes[i + 1] == 0) continue;  // padding
      std::string code = decodeDtc(bytes[i], bytes[i + 1]);
      if (std::find(codes.begin(), codes.end(), code) == codes.end()) codes.push_back(code);
    }
  }
  return codes;
}

std::string parseVin(const std::string& raw) {
  std::string vin;
  for (const std::string& msg : splitMessages(raw)) {
    if (msg.compare(0, 4, "4902") != 0) continue;
    std::vector<uint8_t> bytes = hexToBytes(msg);
    // Skip 0x49, 0x02 and the item count (CAN) / sequence number (legacy).
    for (size_t i = 3; i < bytes.size(); ++i) {
      char c = static_cast<char>(bytes[i]);
      if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z')) vin += c;
    }
  }
  if (vin.size() > 17) vin = vin.substr(vin.size() - 17);
  return vin.size() == 17 ? vin : std::string();
}

bool parseSupportedPids(const std::string& raw, uint8_t basePid, bool supported[256]) {
  bool found = false;
  const std::string prefix = "41" + byteToHex(basePid);
  for (const std::string& msg : splitMessages(raw)) {
    if (msg.compare(0, prefix.size(), prefix) != 0) continue;
    std::vector<uint8_t> mask = hexToBytes(msg.substr(prefix.size()));
    if (mask.size() < 4) continue;
    found = true;
    for (int i = 0; i < 32; ++i) {
      int pid = basePid + 1 + i;
      if (pid < 256 && (mask[i / 8] & (0x80 >> (i % 8)))) supported[pid] = true;
    }
  }
  return found;
}

}  // namespace obd
