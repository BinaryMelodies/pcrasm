#ifndef _OMF_H
#define _OMF_H

typedef enum omf_record_t
{
	OMF_THEADR80 = 0x02,
	OMF_MODEND80 = 0x04,
	OMF_LEDATA80 = 0x06,
	OMF_EOF80    = 0x0E,
	OMF_PUBDEF80 = 0x16,
	OMF_EXTDEF80 = 0x18,
	OMF_FIXUPC80 = 0x20, // external
	OMF_FIXUPA80 = 0x22, // intrasegment
	OMF_FIXUPB80 = 0x24, // intersegment
	OMF_COMDEF80 = 0x2E,

	OMF_THEADR = 0x80,
	OMF_COMENT = 0x88,
	OMF_MODEND16 = 0x8A,
	OMF_MODEND32 = 0x8B,
	OMF_EXTDEF = 0x8C,
	OMF_PUBDEF16 = 0x90,
	OMF_PUBDEF32 = 0x91,
	OMF_LNAMES = 0x96,
	OMF_SEGDEF16 = 0x98,
	OMF_SEGDEF32 = 0x99,
	OMF_GRPDEF = 0x9A,
	OMF_FIXUPP16 = 0x9C,
	OMF_FIXUPP32 = 0x9D,
	OMF_LEDATA16 = 0xA0,
	OMF_LEDATA32 = 0xA1,
	OMF_COMDEF = 0xB0,
} omf_record_t;

// OMF_COMENT subtypes
enum
{
	OMF_extension = 0xA0,
};

// OMF_COMENT/OMF_extension subtypes
enum
{
	OMF_IMPDEF = 0x01,
	OMF_EXPDEF = 0x02,
};

#define OMF_EXPDEF_ORDINAL 0x80
#define OMF_EXPDEF_RESIDENT_NAME 0x40
#define OMF_EXPDEF_NODATA 0x20

void omf_generate(const char * module_name);

#endif // _OMF_H
