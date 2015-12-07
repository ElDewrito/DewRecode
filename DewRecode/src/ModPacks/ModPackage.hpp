#pragma once
#include <string>
#include <vector>
#include <libzippp.h>
#include "ModField.hpp"

class Mod : public ModFieldContainer
{
public:
	int ID;
	std::string selector;
	std::string className;
	std::vector<int> ignoreIds;
	std::string name;

	std::string import;
	std::vector<int> index;

	Mod(int id, const std::string& selector, const std::string& className, const std::vector<int>& ignoreIds, const std::string& name)
	{
		ID = id;
		this->selector = selector;
		this->className = className;
		this->ignoreIds = ignoreIds;
		this->name = name;
	}
	Mod(int id, const std::string& import, const std::string& name)
	{
		ID = id;
		this->import = import;
		this->name = name;
	}
	Mod(int id, const std::string& selector, const std::vector<int>& index, const std::string& name)
	{
		ID = id;
		this->selector = selector;
		this->index = index;
		this->name = name;
	}
};

class ModPackage
{
	std::string path;
	libzippp::ZipArchive* archive;
	std::vector<Mod*> mods;

public:
	std::string ID;
	std::string Name;
	int Build;
	std::string Description;
	std::string Author;
	std::string Version;
	bool Enabled;

	ModPackage(const std::string& path);
	ModPackage(libzippp::ZipArchive* archive, const std::string& path);

	bool load();
	void onTagsLoaded();
};