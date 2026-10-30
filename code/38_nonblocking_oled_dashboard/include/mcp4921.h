#ifndef MCP4921_H
#define MCP4921_H

#include <stdint.h>
#include <stdbool.h>

#define MCP4921_MAX 4095
#define MCP4921_IGNORE (1U << 15)
#define MCP4921_BUF (1U << 14)
#define MCP4921_GA_1X (1U << 13)
#define MCP4921_ACTIVE (1U << 12)
#define MCP4921_BYTES 2

bool mcp4921_command(uint16_t value, bool buffered, bool gain_1x, bool active, uint8_t out[MCP4921_BYTES]);
uint32_t mcp4921_microvolts(uint16_t value, uint32_t vref_mv, bool gain_1x);
bool mcp4921_mode_ok(uint8_t mode);

#endif
