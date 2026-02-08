
#include <stdint.h>
#include <stdio.h>
#include "asm.h"

struct
{
	uint8_t length;
	uint32_t address;
	uint8_t data[256];
	uint16_t high_address;
} intel_hex =
{
	.length = 0,
	.address = 0,
	.high_address = 0,
};

static void intel_hex_flush(void)
{
	if(intel_hex.length > 0)
	{
		uint16_t address = 0;
		switch(output.format)
		{
		case FORMAT_HEX16:
			// TODO: addresses beyond 0x100000?
			if(intel_hex.high_address != (intel_hex.address >> 4))
			{
				intel_hex.high_address = intel_hex.address >> 4;
				uint8_t checksum = -(2 + 2 + intel_hex.high_address + (intel_hex.high_address >> 8));
				fprintf(output.file, ":02000002%04X%02X\n", intel_hex.high_address, checksum);
			}
			address = intel_hex.address - (intel_hex.high_address << 4);
			break;
		case FORMAT_HEX32:
			if(intel_hex.high_address != (intel_hex.address >> 16))
			{
				intel_hex.high_address = intel_hex.address >> 16;
				uint8_t checksum = -(4 + 2 + intel_hex.high_address + (intel_hex.high_address >> 8));
				fprintf(output.file, ":04000002%04X%02X\n", intel_hex.high_address, checksum);
			}
			address = intel_hex.address;
			break;
		default:
			break;
		}

		uint8_t checksum = 0;
		fprintf(output.file, ":%02X%04X00", intel_hex.length, address & 0xFFFF);
		for(size_t i = 0; i < intel_hex.length; i++)
		{
			fprintf(output.file, "%02X", intel_hex.data[i]);
			checksum += intel_hex.data[i];
		}
		checksum += intel_hex.length + address + (address >> 8);
		checksum = -checksum;
		fprintf(output.file, "%02X\n", checksum);

		intel_hex.length = 0;
	}
}

void intel_hex_set_location(uint32_t address)
{
	if(intel_hex.address + intel_hex.length != address)
	{
		intel_hex_flush();
		intel_hex.address = address;
	}
}

void intel_hex_output_byte(uint8_t value)
{
	intel_hex.data[intel_hex.length++] = value;
	if(intel_hex.length == 256)
		intel_hex_flush();
}

void intel_hex_skip(uint32_t count)
{
	output_set_location(intel_hex.address + intel_hex.length + count);
}

void intel_hex_close(void)
{
	intel_hex_flush();
	fprintf(output.file, ":00000001FF\n");
}

