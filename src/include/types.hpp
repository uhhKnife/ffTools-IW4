#pragma once

#include <cstdint>

using u32 = std::uint32_t;
using u16 = std::uint16_t;
using u8 = std::uint8_t;

struct LocalizeEntry
{
	const char* value;
	const char* name;
};

struct RawFile
{
	const char* name;
	int len;
	const char* buffer;
};

struct StringTableCell
{
	const char* string;
	int hash;
};

struct StringTable
{
	const char* name;
	int columnCount;
	int rowCount;
	StringTableCell* values;
};

union XAssetHeader
{
	LocalizeEntry* localize;
	RawFile* rawfile;
	StringTable* stringtable;

	void* data;
};

enum class XAssetType : std::int32_t
{
	PHYSPRESET = 0x0,
	PHYSCOLLMAP = 0x1,
	XANIMPARTS = 0x2,
	XMODEL_SURFS = 0x3,
	XMODEL = 0x4,
	MATERIAL = 0x5,
	PIXELSHADER = 0x6,
	TECHNIQUE_SET = 0x7,
	IMAGE = 0x8,
	SOUND = 0x9,
	SOUND_CURVE = 0xA,
	LOADED_SOUND = 0xB,
	CLIPMAP_SP = 0xC,
	CLIPMAP_MP = 0xD,
	COMWORLD = 0xE,
	GAMEWORLD_SP = 0xF,
	GAMEWORLD_MP = 0x10,
	MAP_ENTS = 0x11,
	FXWORLD = 0x12,
	GFXWORLD = 0x13,
	LIGHT_DEF = 0x14,
	UI_MAP = 0x15,
	FONT = 0x16,
	MENULIST = 0x17,
	MENU = 0x18,
	LOCALIZE_ENTRY = 0x19,
	WEAPON = 0x1A,
	SNDDRIVER_GLOBALS = 0x1B,
	FX = 0x1C,
	IMPACT_FX = 0x1D,
	AITYPE = 0x1E,
	MPTYPE = 0x1F,
	CHARACTER = 0x20,
	XMODELALIAS = 0x21,
	RAWFILE = 0x22,
	STRINGTABLE = 0x23,
	LEADERBOARD = 0x24,
	STRUCTURED_DATA_DEF = 0x25,
	TRACER = 0x26,
	VEHICLE = 0x27,
	ADDON_MAP_ENTS = 0x28,
	COUNT = 0x29,
	STRING = 0x29,
	ASSETLIST = 0x2A,
};

constexpr int XFILE_BLOCK_TEMP = 0x0;
constexpr int XFILE_BLOCK_RUNTIME = 0x1;
constexpr int XFILE_BLOCK_VIRTUAL = 0x2;
constexpr int XFILE_BLOCK_LARGE = 0x3;
constexpr int XFILE_BLOCK_PHYSICAL = 0x4;
constexpr int MAX_XFILE_COUNT = 0x6;

struct XZoneMemory
{
	u32 size;
	u32 externalsize;
	u32 streams[MAX_XFILE_COUNT];
};

struct XFileHeader
{
	char magic[8];
	u32 version;
	u8 allowOnlineUpdate;
	u32 dwHighDateTime;
	u32 dwLowDateTime;
	u32 idk;
	u8 language;
	u32 numStreamFile;
	u32 zoneStart;
	u32 zoneEnd;
};

struct XAssetList
{
	u32 scriptStringCount;
	u32 scriptStrings;
	u32 assetCount;
	u32 assets;
};

struct XAsset
{
	u32 type;
	u32 ptr;
};
