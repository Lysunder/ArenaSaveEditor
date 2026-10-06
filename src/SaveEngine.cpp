#include "SaveEngine.h"

#include <algorithm>
#include <fstream>
#include <iterator>

void SaveEngine::scramble(uint8_t *bytes, size_t length)
{
	// Key is the low byte of a 16-bit counter rotated right by its own low nibble.
	// The counter starts at the region length and counts down each byte.
	uint16_t counter = static_cast<uint16_t>(length);
	for (size_t i = 0; i < length; i++)
	{
		const unsigned int shift = counter & 0xF;
		const uint16_t rotated = (shift == 0) ? counter :
			static_cast<uint16_t>((counter >> shift) | (counter << (16 - shift)));
		bytes[i] ^= static_cast<uint8_t>(rotated & 0xFF);
		counter--;
	}
}

bool SaveEngine::load(const std::string &path, std::string &error)
{
	std::ifstream file(path, std::ios::binary);
	if (!file)
	{
		error = "Could not open \"" + path + "\" for reading.";
		return false;
	}

	this->data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	if (this->data.size() != FILE_SIZE)
	{
		error = "\"" + path + "\" is " + std::to_string(this->data.size()) +
			" bytes, expected " + std::to_string(FILE_SIZE) + ".";
		return false;
	}

	scramble(this->data.data(), SCRAMBLED_SIZE);
	return true;
}

bool SaveEngine::save(const std::string &path, std::string &error) const
{
	std::vector<uint8_t> out = this->data;
	scramble(out.data(), SCRAMBLED_SIZE);

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file)
	{
		error = "Could not open \"" + path + "\" for writing.";
		return false;
	}

	file.write(reinterpret_cast<const char*>(out.data()), out.size());
	if (!file)
	{
		error = "Failed writing \"" + path + "\".";
		return false;
	}

	return true;
}

std::string SaveEngine::getName() const
{
	const char *start = reinterpret_cast<const char*>(this->data.data() + OFF_NAME);
	size_t length = 0;
	while (length < 32 && start[length] != '\0')
	{
		length++;
	}

	return std::string(start, length);
}

bool SaveEngine::isInventorySlotEmpty(size_t slot) const
{
	const uint8_t *item = this->data.data() + OFF_INVENTORY + (slot * ITEM_SIZE);
	for (size_t i = 0; i < ITEM_SIZE; i++)
	{
		if (item[i] != 0)
		{
			return false;
		}
	}

	return true;
}

size_t SaveEngine::countFreeInventorySlots() const
{
	size_t count = 0;
	for (size_t slot = 0; slot < INVENTORY_SLOTS; slot++)
	{
		if (this->isInventorySlotEmpty(slot))
		{
			count++;
		}
	}

	return count;
}

size_t SaveEngine::addItems(const ItemRecord &record, size_t count)
{
	size_t added = 0;
	for (size_t slot = 0; (slot < INVENTORY_SLOTS) && (added < count); slot++)
	{
		if (this->isInventorySlotEmpty(slot))
		{
			std::copy(record, record + ITEM_SIZE, this->data.begin() + OFF_INVENTORY + (slot * ITEM_SIZE));
			added++;
		}
	}

	return added;
}

uint16_t SaveEngine::getU16(size_t offset) const
{
	return static_cast<uint16_t>(this->data[offset] | (this->data[offset + 1] << 8));
}

uint32_t SaveEngine::getU32(size_t offset) const
{
	return static_cast<uint32_t>(this->data[offset]) |
		(static_cast<uint32_t>(this->data[offset + 1]) << 8) |
		(static_cast<uint32_t>(this->data[offset + 2]) << 16) |
		(static_cast<uint32_t>(this->data[offset + 3]) << 24);
}

void SaveEngine::setU16(size_t offset, uint16_t value)
{
	this->data[offset] = static_cast<uint8_t>(value & 0xFF);
	this->data[offset + 1] = static_cast<uint8_t>(value >> 8);
}

void SaveEngine::setU32(size_t offset, uint32_t value)
{
	for (int i = 0; i < 4; i++)
	{
		this->data[offset + i] = static_cast<uint8_t>((value >> (i * 8)) & 0xFF);
	}
}
