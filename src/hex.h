#ifndef _HEX_H
#define _HEX_H

void intel_hex_set_location(uint32_t address);
void intel_hex_output_byte(uint8_t value);
void intel_hex_skip(uint32_t count);
void intel_hex_close(void);

#endif // _HEX_H
