// Short descriptions for common generic (SAE J2012) diagnostic trouble codes.
#pragma once

// Returns a human-readable description for `code` (e.g. "P0420").
// Unknown codes get a generic description based on the code family.
const char* describeDtc(const char* code);
