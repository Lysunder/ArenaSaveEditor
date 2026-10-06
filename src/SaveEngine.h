#pragma once

#include <cstdint>
#include <string>
#include <vector>

// SAVEENGN.xx -- the engine save file. 17983 bytes.
//
// The first 3663 bytes (player NPC record + player data) are scrambled as one
// contiguous region with a rotating XOR key; the rest of the file is plaintext.
//
// Layout of the scrambled region (offsets into the decrypted buffer):
//   0     NPCData    player     (1054 bytes)
//   1054  PlayerData playerData (2609 bytes)
class SaveEngine
{
public:
	static constexpr size_t FILE_SIZE = 17983;
	static constexpr size_t NPC_DATA_SIZE = 1054;
	static constexpr size_t PLAYER_DATA_SIZE = 2609;
	static constexpr size_t SCRAMBLED_SIZE = NPC_DATA_SIZE + PLAYER_DATA_SIZE;

	// Offsets within the decrypted buffer.
	static constexpr size_t OFF_NAME = 9;           // char[32]
	static constexpr size_t OFF_RACE = 4;           // uint8
	static constexpr size_t OFF_CLASS = 5;          // uint8
	static constexpr size_t OFF_LEVEL = 6;          // uint8
	static constexpr size_t OFF_HP = 89;            // uint16
	static constexpr size_t OFF_MAX_HP = 91;        // uint16
	static constexpr size_t OFF_SP = 102;           // uint16
	static constexpr size_t OFF_MAX_SP = 104;       // uint16
	static constexpr size_t OFF_EXPERIENCE = 1033;  // uint32
	static constexpr size_t OFF_GOLD = NPC_DATA_SIZE + 0; // uint32, PlayerData::gold

	bool load(const std::string &path, std::string &error);
	bool save(const std::string &path, std::string &error) const;

	std::string getName() const;
	uint8_t getRace() const { return this->data[OFF_RACE]; }
	uint8_t getClass() const { return this->data[OFF_CLASS]; }
	uint8_t getLevel() const { return this->data[OFF_LEVEL]; }

	uint16_t getHealth() const { return this->getU16(OFF_HP); }
	uint16_t getMaxHealth() const { return this->getU16(OFF_MAX_HP); }
	uint16_t getSpellPoints() const { return this->getU16(OFF_SP); }
	uint16_t getMaxSpellPoints() const { return this->getU16(OFF_MAX_SP); }
	uint32_t getExperience() const { return this->getU32(OFF_EXPERIENCE); }
	uint32_t getGold() const { return this->getU32(OFF_GOLD); }

	void setHealth(uint16_t value) { this->setU16(OFF_HP, value); }
	void setMaxHealth(uint16_t value) { this->setU16(OFF_MAX_HP, value); }
	void setSpellPoints(uint16_t value) { this->setU16(OFF_SP, value); }
	void setMaxSpellPoints(uint16_t value) { this->setU16(OFF_MAX_SP, value); }
	void setExperience(uint32_t value) { this->setU32(OFF_EXPERIENCE, value); }
	void setGold(uint32_t value) { this->setU32(OFF_GOLD, value); }

private:
	// Whole file; the scrambled region is kept decrypted in memory.
	std::vector<uint8_t> data;

	// Same operation both ways (XOR).
	static void scramble(uint8_t *bytes, size_t length);

	uint16_t getU16(size_t offset) const;
	uint32_t getU32(size_t offset) const;
	void setU16(size_t offset, uint16_t value);
	void setU32(size_t offset, uint32_t value);
};
