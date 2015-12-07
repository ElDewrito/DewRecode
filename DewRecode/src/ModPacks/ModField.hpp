#pragma once
#include <string>
#include <vector>
#include <map>
#include "../Utils/pugixml.hpp"
class ModField;
class ModReflexive;
class ModFieldContainer;

class ModFieldContainer
{
public:
	ModFieldContainer* parent = nullptr;

	std::vector<ModField*> fields;
	std::vector<ModReflexive*> reflexives;

	void readFieldsFromXml(pugi::xml_node containerNode);
	int getNumFields();

	void apply(void* baseAddr, const std::map<std::string, std::string>& vars);
};

class ModReflexive : public ModFieldContainer
{
public:
	int offset;
	int entrySize;

	std::string selector;
	std::vector<int> index;
	int rangeStart;
	int rangeEnd;

	ModReflexive(int offset, int entrySize, const std::string& selector)
	{
		this->offset = offset;
		this->entrySize = entrySize;
		this->selector = selector;
	}
	ModReflexive(int offset, int entrySize, const std::string& selector, const std::vector<int>& index)
	{
		this->offset = offset;
		this->entrySize = entrySize;
		this->selector = selector;
		this->index = index;
	}
	ModReflexive(int offset, int entrySize, const std::string& selector, int rangeStart, int rangeEnd)
	{
		this->offset = offset;
		this->entrySize = entrySize;
		this->selector = selector;
		this->rangeStart = rangeStart;
		this->rangeEnd = rangeEnd;
	}

};

class ModField
{
public:
	ModFieldContainer* parent;

	int ID;
	std::string type;
	int offset;
	std::string value;

	ModField(int id, const std::string& type, int offset, const std::string& value)
	{
		ID = id;
		this->type = type;
		this->offset = offset;
		this->value = value;
	}

	bool apply(void* baseAddr, const std::string& value);
};