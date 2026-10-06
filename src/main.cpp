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
			"Usage: " << exe << " <ARENA dir> [slot 0-9]\n"
			"\n"
			"Without a slot, lists the saves in NAMES.DAT to choose from.\n"
			"\n"
			"Example:\n"
			"  " << exe << " \"F:\\Steam\\steamapps\\common\\The Elder Scrolls Arena\\ARENA\"\n";
	}

	std::string makeSavePath(const std::string &dir, uint32_t slot)
	{
		char extension[4];
		std::snprintf(extension, sizeof(extension), ".%02u", slot);
		return dir + "/SAVEENGN" + extension;
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

	// Copies the save to SAVEENGN.xx.bak if no backup exists yet, then writes it.
	bool writeSave(const SaveEngine &save, const std::string &path)
	{
		const std::string backupPath = path + ".bak";
		if (!fileExists(backupPath))
		{
			if (!copyFile(path, backupPath))
			{
				std::cerr << "Could not create backup \"" << backupPath << "\"; not saving.\n";
				return false;
			}

			std::cout << "Backup written to " << backupPath << "\n";
		}

		std::string error;
		if (!save.save(path, error))
		{
			std::cerr << error << "\n";
			return false;
		}

		return true;
	}

	// Prints the used save slots and marks them in `used`. Returns false if there are none.
	bool listSaves(const std::string &dir, bool (&used)[10])
	{
		bool anyUsed = false;

		std::cout << "Saves in " << dir << ":\n\n";
		for (uint32_t i = 0; i < 10; i++)
		{
			// Unused slots are named "EMPTY" in NAMES.DAT.
			const std::string name = readSlotName(dir, static_cast<int>(i));
			if (name.empty() || name == "EMPTY")
			{
				continue;
			}

			SaveEngine save;
			std::string error;
			if (!save.load(makeSavePath(dir, i), error))
			{
				continue;
			}

			std::cout << "  " << i << ". " << name << "  (" << save.getName() <<
				", level " << (save.getLevel() + 1) << ")\n";
			used[i] = true;
			anyUsed = true;
		}

		std::cout << "\n";
		return anyUsed;
	}

	// Asks which of the listed saves to edit. Returns false if the user cancels.
	bool pickSlot(const bool (&used)[10], uint32_t &slot)
	{
		while (true)
		{
			std::cout << "Choose a save (blank to exit): ";
			std::string line;
			if (!std::getline(std::cin, line) || line.empty())
			{
				return false;
			}

			if (parseUInt(line, 9, slot) && used[slot])
			{
				return true;
			}

			std::cout << "Please choose one of the saves listed above.\n";
		}
	}

	// Prompts for a number. Returns false on blank input (cancel) or end of input.
	bool promptValue(const std::string &label, uint32_t max, uint32_t &out)
	{
		while (true)
		{
			std::cout << "New " << label << " (0-" << max << ", blank to cancel): ";
			std::string line;
			if (!std::getline(std::cin, line) || line.empty())
			{
				return false;
			}

			if (parseUInt(line, max, out))
			{
				return true;
			}

			std::cout << "Please enter a whole number from 0 to " << max << ".\n";
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2 && argc != 3)
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
	if (argc == 3)
	{
		if (!parseUInt(argv[2], 9, slot))
		{
			std::cerr << "Slot must be 0-9.\n";
			return 1;
		}
	}
	else
	{
		bool used[10] = {};
		if (!listSaves(dir, used))
		{
			std::cerr << "No saves found. Is this the ARENA folder?\n";
			return 1;
		}

		if (!pickSlot(used, slot))
		{
			return 0;
		}
	}

	const std::string path = makeSavePath(dir, slot);
	const std::string slotName = readSlotName(dir, static_cast<int>(slot));

	SaveEngine save;
	std::string error;
	if (!save.load(path, error))
	{
		std::cerr << error << "\n";
		return 1;
	}

	while (true)
	{
		std::cout << "\n";
		printSave(save, slotName);
		std::cout <<
			"\n"
			"1. Change HP\n"
			"2. Change Max HP\n"
			"3. Change Gold\n"
			"4. Exit\n"
			"> ";

		std::string choice;
		if (!std::getline(std::cin, choice) || choice == "4")
		{
			break;
		}

		uint32_t value;
		if (choice == "1")
		{
			if (!promptValue("HP", UINT16_MAX, value))
			{
				continue;
			}

			save.setHealth(static_cast<uint16_t>(value));
			if (value > save.getMaxHealth())
			{
				std::cout << "Note: HP is now above Max HP (" << save.getMaxHealth() << ").\n";
			}
		}
		else if (choice == "2")
		{
			if (!promptValue("Max HP", UINT16_MAX, value))
			{
				continue;
			}

			save.setMaxHealth(static_cast<uint16_t>(value));
		}
		else if (choice == "3")
		{
			if (!promptValue("Gold", UINT32_MAX, value))
			{
				continue;
			}

			save.setGold(value);
		}
		else
		{
			std::cout << "Please choose 1-4.\n";
			continue;
		}

		if (writeSave(save, path))
		{
			std::cout << "Saved.\n";
		}
	}

	return 0;
}
