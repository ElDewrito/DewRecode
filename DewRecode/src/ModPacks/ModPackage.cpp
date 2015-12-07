#include "ModPackage.hpp"
#include "../ElDorito.hpp"

ModPackage::ModPackage(const std::string& path)
{
	this->path = path;
}

ModPackage::ModPackage(libzippp::ZipArchive* archive, const std::string& path)
{
	this->archive = archive;
	this->path = path;
}

bool ModPackage::load()
{
	auto& dorito = ElDorito::Instance();
	pugi::xml_document doc;

	pugi::xml_parse_result result;
	if(!archive)
		result = doc.load_file((path + "\\mod.xml").c_str());
	else
	{
		archive->open(libzippp::ZipArchive::READ_ONLY);
		auto entries = archive->getEntries();
		bool foundXml = false;
		for (auto entry : entries)
		{
			if (entry.getName() == "mod.xml")
			{
				result = doc.load_string(entry.readAsText().c_str());
				foundXml = true;
				break;
			}
		}
		if (!foundXml)
		{
			dorito.Logger.Log(LogSeverity::Error, "ModPackage", "[%s] doesn't contain mod.xml in root", path.c_str());
			return false;
		}
	}

	dorito.Logger.Log(LogSeverity::Info, "ModPackage", "[%s] mod.xml parse result: %s", path.c_str(), result.description());
	if (!result)
		return false;

	auto modPack = doc.child("modPack");
	if (!modPack)
	{
		dorito.Logger.Log(LogSeverity::Error, "ModPackage", "[%s] mod.xml doesn't contain modPack node", path.c_str());
		return false;
	}

	auto name = modPack.child("name");
	if (name)
		Name = name.text().as_string();

	auto desc = modPack.child("description");
	if (desc)
		Description = desc.text().as_string();

	auto author = modPack.child("author");
	if (author)
		Author = author.text().as_string();

	auto version = modPack.child("version");
	if (version)
		Version = version.text().as_string();

	auto build = modPack.child("build");
	if (build)
		Build = build.text().as_int();

	auto id = modPack.child("id");
	if (!id)
	{
		dorito.Logger.Log(LogSeverity::Error, "ModPackage", "[%s] modPack doesn't contain id element", path.c_str());
		return false;
	}
	ID = id.text().as_string();

	/* TODO: check dewritoVersion attribute */

	for (pugi::xml_node modXml = modPack.child("mod"); modXml; modXml = modXml.next_sibling("mod"))
	{
		auto idStr = std::string(modXml.attribute("id").value());
		auto selector = std::string(modXml.attribute("selector").value());
		auto name = std::string(modXml.attribute("name").value());
		auto import = std::string(modXml.attribute("import").value());

		auto id = std::stoi(idStr, 0, 0);

		Mod* mod = nullptr;

		if (!import.empty())
		{
			mod = new Mod(id, import, name);
		}
		else
		{
			if (selector == "class")
			{
				auto ignoreIdsStr = std::string(modXml.attribute("ignoreIds").value());
				std::vector<int> ignoreIds;
				if (!ignoreIdsStr.empty() && ignoreIdsStr.find(",") == std::string::npos)
					ignoreIds.push_back(std::stoi(ignoreIdsStr, 0, 0));
				else
				{
					auto idsStr = dorito.Utils.SplitString(ignoreIdsStr, ',');
					for (auto idStr : idsStr)
						ignoreIds.push_back(std::stoi(idStr, 0, 0));
				}

				auto className = std::string(modXml.attribute("className").value());
				mod = new Mod(id, selector, className, ignoreIds, name);
			}
			else if (selector == "index")
			{
				auto indexStr = std::string(modXml.attribute("index").value());

				std::vector<int> index;
				if (!indexStr.empty() && indexStr.find(",") == std::string::npos)
					index.push_back(std::stoi(indexStr, 0, 0));
				else
				{
					auto idsStr = dorito.Utils.SplitString(indexStr, ',');
					for (auto idStr : idsStr)
						index.push_back(std::stoi(idStr, 0, 0));
				}

				mod = new Mod(id, selector, index, name);
			}
		}

		if (!mod)
			continue;

		if (mod->import.empty())
			mod->readFieldsFromXml(modXml);

		mods.push_back(mod);
	}

	int fields = 0;
	for (auto mod : mods)
		fields += mod->getNumFields();

	dorito.Logger.Log(LogSeverity::Info, "ModPackage", "[%s] loaded %d mods and %d field edits", Name.c_str(), mods.size(), fields);
	Enabled = true;

	return true;
}

void ModPackage::onTagsLoaded()
{
	if (!Enabled)
		return;

	auto& dorito = ElDorito::Instance();

	dorito.Logger.Log(LogSeverity::Debug, "ModPackage", "[%s] applying mod package", Name.c_str());

	std::map<std::string, std::string> vars;

	for (auto mod : mods)
	{
		if (!mod->import.empty())
		{
			dorito.Logger.Log(LogSeverity::Debug, "ModPackage", "[%s] importing tag from path %s", Name.c_str(), mod->import.c_str());

			// import tag
			int tagIdx = 0xBEEFCAFE;
			vars.insert(std::pair<std::string, std::string>(std::to_string(mod->ID), std::to_string(tagIdx)));
		}
		else
		{
			std::vector<int> tagsToEdit;

			if (mod->selector == "class")
			{
				auto wantedClass = mod->className;
				uint32_t classNum = _byteswap_ulong(*(uint32_t*)wantedClass.c_str()); // is this safe?

				std::vector<int> tags = Blam::Tags::GetTagsOfClass(classNum);
				for (auto idx : tags)
				{
					if (std::find(mod->ignoreIds.begin(), mod->ignoreIds.end(), idx) != mod->ignoreIds.end())
						continue; // id is ignored

					tagsToEdit.push_back(idx);
				}
			}
			else if (mod->selector == "index")
			{
				for (auto idx : mod->index)
				{
					auto* addr = Blam::Tags::GetTagAddress(idx);
					if (!addr)
						continue;
					tagsToEdit.push_back(idx);
				}
			}

			if (tagsToEdit.size() <= 0)
			{
				dorito.Logger.Log(LogSeverity::Debug, "ModPackage", "[%s] onTagsLoaded: no tags to edit for mod %d [%s]!", Name.c_str(), mod->ID, mod->name.c_str());
				return;
			}

			for (auto idx : tagsToEdit)
			{
				dorito.Logger.Log(LogSeverity::Debug, "ModPackage", "[%s] applying mod %d [%s] to tag 0x%x", Name.c_str(), mod->ID, mod->name.c_str(), idx);
				auto* tagAddr = Blam::Tags::GetTagAddress(idx);
				mod->apply(tagAddr, vars);
			}
		}

		dorito.Logger.Log(LogSeverity::Info, "ModPackage", "[%s] mod %d [%s] applied", Name.c_str(), mod->ID, mod->name.c_str());
		dorito.UserInterface.WriteToConsole("Loaded mod " + mod->name + " (#" + std::to_string(mod->ID) + ") from package " + Name);
	}
}