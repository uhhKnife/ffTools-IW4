# ffTools

A tool for working with MW2 Alpha 482 FastFiles (.ff/.ffm format). Provides linking and unlinking capabilities for game assets.

## Building

### Requirements
- Visual Studio 2026
- C++18 or later

### Build Instructions

1. Open the solution file:
   ```
   build\ffTools.sln
   ```

2. Select your build configuration (Debug or Release)

3. Build the solution:
   - Build → Build Solution (F7)

The compiled executables will be placed in:
- Debug: `build\bin\Debug\`
- Release: `build\bin\Release\`

## Usage

### Linker

Creates fastfile archives from assets listed in a CSV manifest.

The `linker` expects a mod name and will read the CSV produced by `unlinker` at:

```
<modname>/zone_source/<modname>.csv
```

Usage:

```
linker.exe <modname>
```

By default the tool writes `<modname>.ff`
 pass `-m` before the mod name to produce `<modname>.ffm` instead.

Example (default):

```
linker.exe patch
```

Produces `patch.ff`.

Example (force .ffm):

```
linker.exe -m patch
```

Produces `patch.ffm`.

Note: pass `-k` to keep the generated `.ffraw`; by default it is removed.

### Unlinker (made for unlinking fastfiles made by linker specifically)

Extracts assets from a fastfile.

```
unlinker.exe <input.ffm> [output_directory]
```

**Example:**
```
unlinker.exe patch.ffm patch
```

If `output_directory` is not specified, it defaults to the input filename (without extension).

This extracts all assets to the `patch/` directory and creates a CSV manifest.

## Supported Asset Types

- **localize** - Localization strings
- **rawfile** - Configuration files (.cfg), script files (.gsc) and other raw text files
- **stringtable** - CSV tables used by the game

## Credits

- [riicchhaarrd/linker_nx1](https://github.com/riicchhaarrd/linker_nx1) - Original C implementation of NX1 linker
- [Laupetin/OpenAssetTools](https://github.com/Laupetin/OpenAssetTools) - IW4 asset type reference
