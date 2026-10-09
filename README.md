# ArenaSaveEditor
A simple save game editor for Arena

## Download

Prebuilt Windows binaries are on the [Releases](https://github.com/Lysunder/ArenaSaveEditor/releases) page. A new release is built automatically each time something is merged into `master`.

## Building

With g++ (MinGW):

```
g++ -std=c++14 -O2 -static -o build/ArenaSaveEditor.exe src/main.cpp src/SaveEngine.cpp
```

Or with CMake (e.g. Visual Studio): `cmake -B build && cmake --build build --config Release`

## Usage

```
ArenaSaveEditor <ARENA dir> [slot 0-9]
```

Without a slot, it lists the saves from `NAMES.DAT` (with each character's name and level)
and asks which one to edit. It then shows the save's current values and a menu:

```
1. Change HP
2. Change Max HP
3. Change Spell Points
4. Change Max Spell Points
5. Change Experience
6. Change Gold
7. Add Potions
8. Exit
```

Each change is written to the save immediately. Before the first write to a slot, the
original `SAVEENGN.xx` is copied to `SAVEENGN.xx.bak` (an existing backup is never overwritten).

## Save format notes

`SAVEENGN.xx` (17983 bytes) starts with the player's NPC record (1054 bytes) followed by
PlayerData (2609 bytes). These 3663 bytes are scrambled **as a single region**: each byte is
XORed with the low byte of a 16-bit counter rotated right by `counter & 0xF`, where the counter
starts at 3663 and decrements per byte. The rest of the file is not scrambled.

| Field        | Offset (decrypted) | Type   |
|--------------|-------------------:|--------|
| Name         | 9                  | char[32] |
| Current HP   | 89                 | uint16 |
| Max HP       | 91                 | uint16 |
| Spell points | 102                | uint16 |
| Max spell pts| 104                | uint16 |
| Inventory    | 110                | 40 x 19-byte item records (all zero = empty) |
| Experience   | 1033               | uint32 |
| Gold         | 1054 (PlayerData+0)| uint32 |

Struct layouts are based on [OpenTESArena](https://github.com/afritz1/OpenTESArena)'s `ArenaTypes.h`.
