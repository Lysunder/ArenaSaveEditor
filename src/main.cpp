#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

#include "SaveEngine.h"

namespace
{
	void printUsage(const char *exe)
	{
		std::cout <<
			"Usage: " << exe << " <ARENA dir> <slot 0-9> [options]\n"
			"\n"
			"With no options, prints the save's current values.\n"
			"\n"
			"Options:\n"
			"  --gold <n>      Set gold (0-4294967295)\n"
			"  --hp <n>        Set current health (0-65535)\n"
			"  --max-hp <n>    Set maximum health (0-65535)\n"
			"  --no-backup     Don't create SAVEENGN.xx.bak before writing\n"
			"\n"
			"Example:\n"
			"  " << exe << " \"F:\\Steam\\steamapps\\common\\The Elder Scrolls Arena\\ARENA\" 0 --gold 5000 --hp 100 --max-hp 100\n";
	}

	bool parseUInt(const std::string &text, uint32_t max, uint32_t &out)
	{
		if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos)
		{
			return false;
		}

		errno = 0;
		const unsigned long long value = std::strtoull(text.c_str(), nullptr, 10);
		if (errno != 0 || value > max)
		{
			return false;
		}

		out = static_cast<uint32_t>(value);
		return true;
	}

	// NAMES.DAT holds ten 48-byte save slot names.
	std::string readSlotName(const std::string &dir, int slot)
	{
		std::ifstream file(dir + "/NAMES.DAT", std::ios::binary);
		if (!file)
		{
			return "";
		}

		char name[48] = {};
		file.seekg(slot * 48);
		file.read(name, sizeof(name));
		name[47] = '\0';
		return file ? std::string(name) : std::string();
	}

	bool copyFile(const std::string &src, const std::string &dst)
	{
		std::ifstream in(src, std::ios::binary);
		std::ofstream out(dst, std::ios::binary | std::ios::trunc);
		if (!in || !out)
		{
			return false;
		}

		out << in.rdbuf();
		return static_cast<bool>(out);
	}

	bool fileExists(const std::string &path)
	{
		return static_cast<bool>(std::ifstream(path));
	}

	void printSave(const SaveEngine &save, const std::string &slotName)
	{
		std::cout <<
			"Save name:  " << slotName << "\n"
			"Character:  " << save.getName() << "\n"
			"Level:      " << (save.getLevel() + 1) << "\n"
			"Experience: " << save.getExperience() << "\n"
			"Health:     " << save.getHealth() << " / " << save.getMaxHealth() << "\n"
			"Spell pts:  " << save.getSpellPoints() << " / " << save.getMaxSpellPoints() << "\n"
			"Gold:       " << save.getGold() << "\n";
	}
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printUsage(argv[0]);
		return 1;
	}

	std::string dir = argv[1];
	while (!dir.empty() && (dir.back() == '/' || dir.back() == '\\'))
	{
		dir.pop_back();
	}

	uint32_t slot;
	if (!parseUInt(argv[2], 9, slot))
	{
		std::cerr << "Slot must be 0-9.\n";
		return 1;
	}

	bool setGold = false, setHp = false, setMaxHp = false, backup = true;
	uint32_t gold = 0, hp = 0, maxHp = 0;

	for (int i = 3; i < argc; i++)
	{
		const std::string arg = argv[i];
		const bool hasValue = (i + 1) < argc;

		if (arg == "--gold" && hasValue && parseUInt(argv[i + 1], UINT32_MAX, gold))
		{
			setGold = true;
			i++;
		}
		else if (arg == "--hp" && hasValue && parseUInt(argv[i + 1], UINT16_MAX, hp))
		{
			setHp = true;
			i++;
		}
		else if (arg == "--max-hp" && hasValue && parseUInt(argv[i + 1], UINT16_MAX, maxHp))
		{
			setMaxHp = true;
			i++;
		}
		else if (arg == "--no-backup")
		{
			backup = false;
		}
		else
		{
			std::cerr << "Bad or incomplete option: " << arg << "\n\n";
			printUsage(argv[0]);
			return 1;
		}
	}

	char extension[4];
	std::snprintf(extension, sizeof(extension), ".%02u", slot);
	const std::string path = dir + "/SAVEENGN" + extension;
	const std::string slotName = readSlotName(dir, static_cast<int>(slot));

	SaveEngine save;
	std::string error;
	if (!save.load(path, error))
	{
		std::cerr << error << "\n";
		return 1;
	}

	if (!setGold && !setHp && !setMaxHp)
	{
		printSave(save, slotName);
		return 0;
	}

	if (setGold)
	{
		save.setGold(gold);
	}

	if (setMaxHp)
	{
		save.setMaxHealth(static_cast<uint16_t>(maxHp));
	}

	if (setHp)
	{
		save.setHealth(static_cast<uint16_t>(hp));
		if (hp > save.getMaxHealth())
		{
			std::cout << "Warning: health " << hp << " exceeds max health " <<
				save.getMaxHealth() << "; consider --max-hp too.\n";
		}
	}

	if (backup)
	{
		const std::string backupPath = path + ".bak";
		if (!fileExists(backupPath))
		{
			if (!copyFile(path, backupPath))
			{
				std::cerr << "Could not create backup \"" << backupPath << "\"; aborting.\n";
				return 1;
			}

			std::cout << "Backup written to " << backupPath << "\n";
		}
	}

	if (!save.save(path, error))
	{
		std::cerr << error << "\n";
		return 1;
	}

	std::cout << "Saved.\n\n";
	printSave(save, slotName);
	return 0;
}
