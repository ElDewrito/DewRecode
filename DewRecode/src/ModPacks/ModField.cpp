#include "ModField.hpp"
#include "../ElDorito.hpp"

void ModFieldContainer::readFieldsFromXml(pugi::xml_node containerNode)
{
	// read edits
	for (pugi::xml_node fieldXml = containerNode.child("field"); fieldXml; fieldXml = fieldXml.next_sibling("field"))
	{
		auto fieldIdStr = std::string(fieldXml.attribute("id").value());
		auto fieldType = std::string(fieldXml.attribute("type").value());
		auto fieldOffsetStr = std::string(fieldXml.attribute("offset").value());
		auto fieldValue = std::string(fieldXml.text().as_string());

		auto fieldId = std::stoi(fieldIdStr, 0, 0);
		auto fieldOffset = std::stoi(fieldOffsetStr, 0, 0);

		fields.push_back(new ModField(fieldId, fieldType, fieldOffset, fieldValue));
	}

	// read reflexives
	for (pugi::xml_node blockXml = containerNode.child("reflexive"); blockXml; blockXml = blockXml.next_sibling("reflexive"))
	{
		auto blockOffsetStr = std::string(blockXml.attribute("offset").value());
		auto blockSelector = std::string(blockXml.attribute("selector").value());
		auto blockEntrySizeStr = std::string(blockXml.attribute("entrySize").value());

		auto blockOffset = std::stoi(blockOffsetStr, 0, 0);
		auto blockEntrySize = std::stoi(blockEntrySizeStr, 0, 0);

		auto* reflexive = new ModReflexive(blockOffset, blockEntrySize, blockSelector);

		if (blockSelector == "index")
		{
			auto indexStr = std::string(blockXml.attribute("index").value());

			std::vector<int> index;
			if (!indexStr.empty() && indexStr.find(",") == std::string::npos)
				index.push_back(std::stoi(indexStr, 0, 0));
			else
			{
				auto idsStr = ElDorito::Instance().Utils.SplitString(indexStr, ',');
				for (auto idStr : idsStr)
					index.push_back(std::stoi(idStr, 0, 0));
			}

			reflexive = new ModReflexive(blockOffset, blockEntrySize, blockSelector, index);
		}
		else if (blockSelector == "range")
		{
			auto blockRangeStartStr = std::string(blockXml.attribute("rangeStart").value());
			auto blockRangeEndStr = std::string(blockXml.attribute("rangeEnd").value());

			auto blockRangeStart = std::stoi(blockRangeStartStr, 0, 0);
			auto blockRangeEnd = std::stoi(blockRangeEndStr, 0, 0);

			reflexive = new ModReflexive(blockOffset, blockEntrySize, blockSelector, blockRangeStart, blockRangeEnd);
		}
		reflexive->parent = this;
		reflexive->readFieldsFromXml(blockXml);

		reflexives.push_back(reflexive);
	}
}

int ModFieldContainer::getNumFields()
{
	int count = fields.size();
	for (auto reflexive : reflexives)
		count += reflexive->getNumFields();
	return count;
}

void ModFieldContainer::apply(void* baseAddr, const std::map<std::string, std::string>& vars)
{
	for (auto field : fields)
	{
		std::string value = field->value;
		for (auto var : vars)
		{
			std::string testVar = "{" + var.first + "}";
			if (field->value == testVar)
			{
				value = var.second;
				break;
			}
		}
		field->apply(baseAddr, value);
	}

	for (auto reflexive : reflexives)
	{
		auto* reflexiveAddr = (char*)baseAddr + reflexive->offset;
		auto* reflexiveBaseAddr = (char*)*(uint32_t*)(reflexiveAddr + 4);
		int count = *(uint32_t*)reflexiveAddr;

		if (!count || !reflexiveBaseAddr || count == -1 || reflexiveBaseAddr == (char*)-1)
			continue;

		std::vector<int> blockIdxs;
		if (reflexive->selector == "all")
			for (auto i = 0; i < count; i++)
				blockIdxs.push_back(i);

		else if (reflexive->selector == "index")
			for (auto idx : reflexive->index)
			{
				if (idx < count && idx > 0)
					blockIdxs.push_back(idx);
			}
		else if (reflexive->selector == "range")
			for (auto i = reflexive->rangeStart; i < reflexive->rangeEnd; i++)
				if (i < count && i > 0)
					blockIdxs.push_back(i);

		for (auto idx : blockIdxs)
		{
			reflexive->apply(reflexiveBaseAddr + (reflexive->entrySize * idx), vars);
		}
	}
}

bool ModField::apply(void* baseAddr, const std::string& value)
{
	char* tagAddr = (char*)baseAddr + offset;

	if (type == "byte" || type == "int8" || type == "uint8")
	{
		uint8_t val = (uint8_t)std::stoi(value, 0, 0);
		*(uint8_t*)tagAddr = val;
	}
	else if (type == "short" || type == "int16" || type == "uint16")
	{
		uint16_t val = (uint16_t)std::stoi(value, 0, 0);
		*(uint16_t*)tagAddr = val;
	}
	else if (type == "int" || type == "int32" || type == "uint32")
	{
		uint32_t val = (uint32_t)std::stoi(value, 0, 0);
		*(uint32_t*)tagAddr = val;
	}
	else if (type == "float" || type == "float32")
	{
		float val = std::stof(value, 0);
		*(float*)tagAddr = val;
	}
	else if (type == "tagRef")
	{
		uint32_t val = (uint32_t)std::stoi(value, 0, 0);
		*(uint32_t*)(tagAddr + 0xC) = val;
		*(uint32_t*)tagAddr = Blam::Tags::GetTagClass(val);
	}
	else
		return false;

	return true;
}