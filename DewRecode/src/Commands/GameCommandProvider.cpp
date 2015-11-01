#include "GameCommandProvider.hpp"
#include "../ElDorito.hpp"
#include <sstream>
#include <fstream>

namespace
{
	int GetUiGameMode();
	bool LoadGameVariant(std::ifstream& file, uint8_t* out);
	bool LoadDefaultGameVariant(const std::string& name, uint8_t* out);
	void SaveGameVariantToPreferences(const uint8_t* data);

	bool LoadMapVariant(std::ifstream& file, uint8_t* out);
	bool LoadDefaultMapVariant(const std::string& mapName, uint8_t* out);
	void SaveMapVariantToPreferences(const uint8_t* data);
}

namespace Game
{
	std::vector<Command> GameCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Game", "Info", "info", "Displays information about the game", eCommandFlagsNone, BIND_COMMAND(this, &GameCommandProvider::CommandInfo)),
			Command::CreateCommand("Game", "Exit", "exit", "Ends the game process", eCommandFlagsNone, BIND_COMMAND(this, &GameCommandProvider::CommandExit)),
			Command::CreateCommand("Game", "Version", "version", "Displays the game's version", eCommandFlagsNone, BIND_COMMAND(this, &GameCommandProvider::CommandVersion)),
			Command::CreateCommand("Game", "ForceLoad", "forceload", "Forces a map to load", eCommandFlagsNone, BIND_COMMAND(this, &GameCommandProvider::CommandForceLoad), { "mapname(string) The name of the map to load", "gametype(int) The gametype to load", "gamemode(int) The type of gamemode to play", }),
			Command::CreateCommand("Game", "Map", "map", "Loads a map or map variant", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &GameCommandProvider::CommandMap), { "name(string) The internal name of the map or Forge map to load" }),
			Command::CreateCommand("Game", "GameType", "gametype", "Loads a gametype", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &GameCommandProvider::CommandGameType), { "name(string) The internal name of the built-in gametype or custom gametype to load" }),
			Command::CreateCommand("Game", "Start", "start", "Starts or restarts the game", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &GameCommandProvider::CommandStart)),
			Command::CreateCommand("Game", "Stop", "stop", "Stops the game and returns to lobby", eCommandFlagsRunOnMainMenu, BIND_COMMAND(this, &GameCommandProvider::CommandStop))
		};

		return commands;
	}

	void GameCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarLanguageID = manager->Add(Command::CreateVariableInt("Game", "LanguageID", "languageid", "The index of the language to use", eCommandFlagsArchived, 0));
		VarLanguageID->ValueIntMin = 0;
		VarLanguageID->ValueIntMax = 11;
	}

	bool GameCommandProvider::CommandExit(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		this->Exit();
		return true;
	}

	void GameCommandProvider::Exit()
	{
		std::exit(0);
	}

	bool GameCommandProvider::CommandInfo(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		context.WriteOutput(this->GetInfo());
		return true;
	}

	std::string GameCommandProvider::GetInfo()
	{
		std::stringstream ss;
		std::string ArgList((char*)Pointer(0x199C0A4)[0]);
		std::string LocalSecureKey((char*)Pointer(0x50CCDB4 + 1));
		std::string Build((char*)Pointer(0x199C0F0));
		std::string SystemID((char*)Pointer(0x199C130));
		std::string SessionID((char*)Pointer(0x199C1D0));
		std::string DevKitName((char*)Pointer(0x160C8C8));
		std::string DevKitVersion((char*)Pointer(0x199C350));
		std::string FlashVersion((char*)Pointer(0x199C350));
		std::string MapName((char*)Pointer(0x22AB018)(0x1A4));
		std::wstring VariantName((wchar_t*)Pointer(0x23DAF4C));

		auto& dorito = ElDorito::Instance();

		ss << std::hex << "ThreadLocalStorage: 0x" << std::hex << (size_t)(void*)dorito.Engine.GetMainTls() << std::endl;

		ss << "Command line args: " << (ArgList.empty() ? "(null)" : ArgList) << std::endl;
		ss << "Local secure key: " << (LocalSecureKey.empty() ? "(null)" : LocalSecureKey) << std::endl;

		auto* session = dorito.Engine.GetActiveNetworkSession();
		if (session && (session->IsEstablished() || session->IsHost()))
		{
			auto managedBase = Pointer(0x2247450);
			Blam::Network::ManagedSession* managedSession = managedBase(session->AddressIndex * 0x608);

			std::string xnkid;
			std::string xnaddr;
			dorito.Utils.BytesToHexString((char*)managedSession->HostAddr.Xnkid, 0x10, xnkid);
			dorito.Utils.BytesToHexString((char*)managedSession->HostAddr.Xnaddr, 0x10, xnaddr);
			ss << "XNKID: " << xnkid << std::endl;
			ss << "XNAddr: " << xnaddr << std::endl;
		}
		ss << "Game server port: " << std::dec << Pointer(0x1860454).Read<uint32_t>() << std::endl;
		ss << "Info server running: " << (dorito.ServerCommands->IsInfoServerRunning() ? "true" : "false") << std::endl;
		ss << "Info server port: " << std::dec << dorito.ServerCommands->VarPort->ValueInt << std::endl;
		ss << "Rcon server running: " << (dorito.ServerCommands->IsRconServerRunning() ? "true" : "false") << std::endl;
		ss << "Rcon server port:" << std::dec << dorito.ServerCommands->VarRconPort->ValueInt << std::endl;
		ss << "Build: " << (Build.empty() ? "(null)" : Build) << std::endl;
		ss << "SystemID: " << (SystemID.empty() ? "(null)" : SystemID) << std::endl;
		ss << "SessionID: " << (SessionID.empty() ? "(null)" : SessionID) << std::endl;
		ss << "SDK info: " << (DevKitName.empty() ? "(null)" : DevKitName) << '|';
		ss << (DevKitVersion.empty() ? "(null)" : DevKitVersion) << std::endl;

		ss << "Flash version: " << (FlashVersion.empty() ? "(null)" : FlashVersion) << std::endl;
		ss << "Current map: " << (MapName.empty() ? "(null)" : MapName) << std::endl;
		ss << "Current map cache size: " << std::dec << Pointer(0x22AB018)(0x8).Read<int32_t>() << std::endl;
		ss << "Loaded game variant: " << (VariantName.empty() ? "(null)" : dorito.Utils.ThinString(VariantName)) << std::endl;
		ss << "Loaded game type: 0x" << std::hex << Pointer(0x023DAF18).Read<int32_t>() << std::endl;
		ss << "Tag table offset: 0x" << std::hex << Pointer(0x22AAFF4).Read<uint32_t>() << std::endl;
		ss << "Tag bank offset: 0x" << std::hex << Pointer(0x22AAFF8).Read<uint32_t>() << std::endl;
		ss << "Players global addr: 0x" << std::hex << dorito.Engine.GetMainTls(GameGlobals::Players::TLSOffset).Read<uint32_t>() << std::endl;

		return ss.str();
	}

	bool GameCommandProvider::CommandVersion(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		context.WriteOutput(this->GetVersion());
		return true;
	}

	std::string GameCommandProvider::GetVersion()
	{
		return Utils::Version::GetVersionString(); // todo: return game version here instead? it is Game.Version after all
	}

	bool GameCommandProvider::CommandForceLoad(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		std::stringstream ss;

		if (Arguments.size() <= 0)
		{
			ss << "Current map: " << std::string((char*)(0x2391824)) << std::endl;
			ss << "Usage: Game.ForceLoad <mapname> [gametype] [maptype]" << std::endl;
			ss << "Available maps:";

			for (auto map : MapList)
				ss << std::endl << "\t" << map;

			ss << std::endl << std::endl << "Valid gametypes:";
			for (size_t i = 0; i < (size_t)Blam::GameType::Count - 1; i++)
				ss << std::endl << "\t" << "[" << i << "] " << Blam::GameTypeNames[i];

			context.WriteOutput(ss.str());
			return false;
		}

		auto mapName = Arguments[0];

		auto gameTypeStr = "none";
		Blam::GameType gameType = Blam::GameType::None;
		Blam::GameMode gameMode = Blam::GameMode::Multiplayer;

		if (Arguments.size() >= 2)
		{
			//Look up gametype string.
			size_t i;
			for (i = 0; i < (size_t)Blam::GameType::Count; i++)
			{
				// Todo: case insensiive
				if (!Blam::GameTypeNames[i].compare(Arguments[1]))
				{
					gameType = Blam::GameType(i);
					break;
				}
			}

			if (i == (size_t)Blam::GameType::Count)
				gameType = (Blam::GameType)std::atoi(Arguments[1].c_str());

			if (gameType > Blam::GameType::Count) // only valid gametypes are 1 to 10
				gameType = Blam::GameType::Slayer;
		}

		if (Arguments.size() >= 3)
		{
			//Look up gamemode string.
			size_t i;
			for (i = 0; i < (size_t)Blam::GameMode::Count; i++)
			{
				// Todo: case insensiive
				if (!Blam::GameModeNames[i].compare(Arguments[2]))
				{
					gameMode = Blam::GameMode(i);
					break;
				}
			}

			if (i == (size_t)Blam::GameMode::Count)
				gameMode = (Blam::GameMode)std::atoi(Arguments[2].c_str());

			if (gameMode > Blam::GameMode::Count) // only valid gametypes are 1 to 10
				gameMode = Blam::GameMode::Multiplayer;
		}

		ss << "Loading " << mapName << " gametype: " << Blam::GameTypeNames[(size_t)gameType] << " gamemode: " << Blam::GameModeNames[(size_t)gameMode];

		auto ret = ForceLoad(mapName, gameType, gameMode);
		if (!ret)
			ss << std::endl << "Fprce load failed: map not found.";

		context.WriteOutput(ss.str());
		return ret;
	}

	bool GameCommandProvider::ForceLoad(const std::string& map, Blam::GameType gameType, Blam::GameMode gameMode)
	{
		if (std::find(MapList.begin(), MapList.end(), map) == MapList.end())
			return false;

		std::string mapName = "maps\\" + map;

		Pointer(0x2391B2C).Write<uint32_t>((uint32_t)gameType);
		Pointer(0x2391800).Write<uint32_t>((uint32_t)gameMode);
		Pointer(0x2391824).Write(mapName.c_str(), mapName.length() + 1);

		// Infinite play time
		Pointer(0x2391C51).Write<uint8_t>(0);

		// Map Reset
		Pointer(0x23917F0).Write<uint8_t>(0x1);
		return true;
	}


	bool GameCommandProvider::CommandMap(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.size() != 1)
		{
			context.WriteOutput("You must specify an internal map or Forge map name!");
			return false;
		}

		context.WriteOutput("Loading map \"" + Arguments[0] + "\"...");

		auto ret = SetMap(Arguments[0]);
		switch (ret)
		{
		case SetGameTypeReturnCode::NotInLobby:
			context.WriteOutput("\nYou can only change maps from a Custom Games or Forge lobby.");
			break;
		case SetGameTypeReturnCode::InvalidFile:
			context.WriteOutput("\nInvalid map variant file!");
			break;
		case SetGameTypeReturnCode::InvalidGameFile:
			context.WriteOutput("\nInvalid map file!");
			break;
		case SetGameTypeReturnCode::LoadFailed:
			context.WriteOutput("\nMap load failed.");
			break;
		case SetGameTypeReturnCode::Success:
			context.WriteOutput("\nMap loaded successfully!");
			break;
		}

		return ret == SetGameTypeReturnCode::Success;
	}

	SetGameTypeReturnCode GameCommandProvider::SetMap(const std::string& name)
	{
		auto lobbyType = GetUiGameMode();
		if (lobbyType != 2 && lobbyType != 3)
			return SetGameTypeReturnCode::NotInLobby;

		const auto UnkVariantBlfSize = 0xE090;
		uint8_t* variantData = (uint8_t*)malloc(UnkVariantBlfSize);

		// If the name is the name of a valid map variant, load it
		auto variantFileName = "mods/maps/" + name + "/sandbox.map";
		std::ifstream mapVariant(variantFileName, std::ios::binary);
		if (mapVariant.is_open())
		{
			if (!LoadMapVariant(mapVariant, variantData))
			{
				free(variantData);
				return SetGameTypeReturnCode::InvalidFile;
			}
		}
		else
		{
			if (!LoadDefaultMapVariant(name, variantData))
			{
				free(variantData);
				return SetGameTypeReturnCode::InvalidGameFile;
			}
		}

		// Submit a request to load the variant
		typedef bool(*LoadMapVariantPtr)(uint8_t* variant, void* unknown);
		auto LoadMapVariant = reinterpret_cast<LoadMapVariantPtr>(0xA83AF0);
		if (!LoadMapVariant(variantData, nullptr))
		{
			free(variantData);
			return SetGameTypeReturnCode::LoadFailed;
		}
		SaveMapVariantToPreferences(variantData);
		free(variantData);

		return SetGameTypeReturnCode::Success;
	}

	bool GameCommandProvider::CommandGameType(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.size() != 1)
		{
			context.WriteOutput("You must specify a built-in gametype or custom gametype name!");
			return false;
		}

		context.WriteOutput("Loading game variant \"" + Arguments[0] + "\"...");

		auto ret = SetGameType(Arguments[0]);
		switch (ret)
		{
		case SetGameTypeReturnCode::NotInLobby:
			context.WriteOutput("\nYou can only change gametypes from a Custom Games lobby.");
			break;
		case SetGameTypeReturnCode::InvalidFile:
			context.WriteOutput("\nInvalid game variant file!");
			break;
		case SetGameTypeReturnCode::NotFound:
			context.WriteOutput("\nGame variant not found.");
			break;
		case SetGameTypeReturnCode::LoadFailed:
			context.WriteOutput("\nLoad failed.");
			break;
		case SetGameTypeReturnCode::Success:
			context.WriteOutput("\nGame variant loaded successfully!");
			break;
		}

		return ret == SetGameTypeReturnCode::Success;
	}

	SetGameTypeReturnCode GameCommandProvider::SetGameType(const std::string& gameType)
	{
		if (GetUiGameMode() != 2)
			return SetGameTypeReturnCode::NotInLobby;

		uint8_t variantData[0x264];

		// Check if this is a custom gametype by searching for a file
		// corresponding to each supported game mode
		std::ifstream gameVariant;
		std::string variantFileName;
		for (auto i = 1; i < (uint32_t)Blam::GameType::Count; i++)
		{
			variantFileName = "mods/variants/" + gameType + "/variant." + Blam::GameTypeNames[i];
			gameVariant.open(variantFileName, std::ios::binary);
			if (gameVariant.is_open())
				break;
		}
		if (gameVariant.is_open())
		{
			if (!LoadGameVariant(gameVariant, variantData))
				return SetGameTypeReturnCode::InvalidFile;
		}
		else
		{
			if (!LoadDefaultGameVariant(gameType, variantData))
				return SetGameTypeReturnCode::NotFound;
		}

		// Submit a request to load the variant
		typedef bool(*LoadGameVariantPtr)(uint8_t* variant);
		auto LoadGameVariant = reinterpret_cast<LoadGameVariantPtr>(0x439860);
		if (!LoadGameVariant(variantData))
			return SetGameTypeReturnCode::LoadFailed;

		SaveGameVariantToPreferences(variantData);
		
		return SetGameTypeReturnCode::Success;
	}

	bool GameCommandProvider::CommandStart(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		context.WriteOutput("Starting game...");
		auto ret = Start();
		if (!ret)
			context.WriteOutput("Unable to start the game!");
		return ret;
	}

	bool GameCommandProvider::Start()
	{
		typedef bool(__thiscall *SetSessionModePtr)(void* thisptr, int mode);
		auto SetSessionMode = reinterpret_cast<SetSessionModePtr>(0x459A40);

		// Note: this isn't necessarily a proper way of getting the this
		// pointer, but it seems to work OK
		return SetSessionMode(reinterpret_cast<void*>(0x1BF1B90), 2);
	}

	bool GameCommandProvider::CommandStop(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		context.WriteOutput("Stopping game...");
		auto ret = Stop();
		if (!ret)
			context.WriteOutput("Unable to stop the game!");
		return ret;
	}

	bool GameCommandProvider::Stop()
	{
		typedef bool(__thiscall *SetSessionModePtr)(void* thisptr, int mode);
		auto SetSessionMode = reinterpret_cast<SetSessionModePtr>(0x459A40);

		// Note: this isn't necessarily a proper way of getting the this
		// pointer, but it seems to work OK
		return SetSessionMode(reinterpret_cast<void*>(0x1BF1B90), 1);
	}


}

namespace
{

	size_t GetFileSize(std::ifstream& file)
	{
		auto originalPos = file.tellg();
		file.seekg(0, std::ios::end);
		auto size = static_cast<size_t>(file.tellg());
		file.seekg(originalPos);
		return size;
	}

	bool LoadMapVariant(std::ifstream& file, uint8_t* out)
	{
		// TODO: Would it be better to figure out how to use the game's file
		// functions here?

		// Verify file size
		const auto MapVariantBlfSize = 0xE1F0;
		if (GetFileSize(file) < MapVariantBlfSize)
			return false;

		// Load it into a buffer and have the game parse it
		uint8_t* blfData = (uint8_t*)malloc(MapVariantBlfSize);
		file.read(reinterpret_cast<char*>(blfData), MapVariantBlfSize);

		typedef bool(__thiscall *ParseMapVariantBlfPtr)(void* blf, uint8_t* outVariant, bool* result);
		auto ParseMapVariant = reinterpret_cast<ParseMapVariantBlfPtr>(0x573250);

		bool retVal = ParseMapVariant(blfData, out, nullptr);
		free(blfData);

		return retVal;
	}

	int GetMapId(const std::string& mapName)
	{
		// Open the .map file
		auto mapPath = "maps/" + mapName + ".map";
		std::ifstream mapFile(mapPath, std::ios::binary);
		if (!mapFile.is_open())
			return -1;

		// Verify file size
		const auto MapHeaderSize = 0x3390;
		if (GetFileSize(mapFile) < MapHeaderSize)
			return -1;

		// Verify file header
		const int32_t FileHeaderMagic = 'head';
		int32_t actualMagic = 0;
		mapFile.read(reinterpret_cast<char*>(&actualMagic), sizeof(actualMagic));
		if (actualMagic != FileHeaderMagic)
			return -1;

		// Read map ID
		int32_t mapId = 0;
		mapFile.seekg(0x2DEC);
		mapFile.read(reinterpret_cast<char*>(&mapId), sizeof(mapId));
		return mapId;
	}

	bool LoadDefaultMapVariant(const std::string& mapName, uint8_t* out)
	{
		int mapId = GetMapId(mapName);
		if (mapId < 0)
			return false;

		// Initialize an empty variant for the map
		typedef void(__thiscall *InitializeMapVariantPtr)(uint8_t* outVariant, int mapId);
		auto InitializeMapVariant = reinterpret_cast<InitializeMapVariantPtr>(0x581F70);
		InitializeMapVariant(out, mapId);

		// Make sure it actually loaded the map correctly by verifying that the
		// variant is valid for the map
		int32_t firstMapId = *reinterpret_cast<int32_t*>(out + 0xE0);
		int32_t secondMapId = *reinterpret_cast<int32_t*>(out + 0x100);
		return (firstMapId == mapId && secondMapId == mapId);
	}

	void SaveMapVariantToPreferences(const uint8_t* data)
	{
		// Check the lobby type so we know where to save the variant
		size_t variantOffset;
		switch (GetUiGameMode())
		{
		case 2: // Customs
			variantOffset = 0x7F0;
			break;
		case 3: // Forge
			variantOffset = 0xEA98;
			break;
		default:
			return;
		}

		// Copy the data in
		auto savedVariant = reinterpret_cast<uint8_t*>(0x22C0130 + variantOffset);
		memcpy(savedVariant, data, 0xE090);

		// Mark preferences as dirty
		*reinterpret_cast<bool*>(0x22C0129) = true;
	}

	// TODO: Might be useful to map out this enum
	// 2 = customs, 3 = forge
	int GetUiGameMode()
	{
		typedef int(__thiscall *GetUiGameModePtr)();
		auto GetUiGameModeImpl = reinterpret_cast<GetUiGameModePtr>(0x435640);
		return GetUiGameModeImpl();
	}

	void SaveGameVariantToPreferences(const uint8_t* data)
	{
		if (GetUiGameMode() != 2)
			return; // Only allow doing this from a customs lobby

		// Copy the data in
		auto savedVariant = reinterpret_cast<uint8_t*>(0x22C0130 + 0x37C);
		memcpy(savedVariant, data, 0x264);

		// Mark preferences as dirty
		*reinterpret_cast<bool*>(0x22C0129) = true;
	}

	bool LoadGameVariant(std::ifstream& file, uint8_t* out)
	{
		// Verify file size
		const auto GameVariantBlfSize = 0x3BC;
		if (GetFileSize(file) < GameVariantBlfSize)
			return false;

		// Load it into a buffer and have the game parse it
		uint8_t blfData[GameVariantBlfSize];
		file.read(reinterpret_cast<char*>(blfData), GameVariantBlfSize);

		typedef bool(__thiscall *ParseGameVariantBlfPtr)(void* blf, uint8_t* outVariant, bool* result);
		auto ParseGameVariant = reinterpret_cast<ParseGameVariantBlfPtr>(0x573150);
		return ParseGameVariant(blfData, out, nullptr);
	}

	template<class T>
	int FindDefaultGameVariant(const Blam::Tags::TagBlock<T>& variants, const std::string& name)
	{
		static_assert(std::is_base_of<Blam::Tags::GameVariantDefinition, T>::value, "T must be a GameVariantDefinition");
		if (!variants)
			return -1;
		for (auto i = 0; i < variants.Count; i++)
		{
			if (variants[i].Name[0] && strcmp(variants[i].Name, name.c_str()) == 0)
				return i;
		}
		return -1;
	}

	int FindDefaultGameVariant(Blam::Tags::GameEngineSettingsDefinition* wezr, Blam::GameType type, const std::string& name)
	{
		switch (type)
		{
		case Blam::GameType::CTF:
			return FindDefaultGameVariant(wezr->CtfVariants, name);
		case Blam::GameType::Slayer:
			return FindDefaultGameVariant(wezr->SlayerVariants, name);
		case Blam::GameType::Oddball:
			return FindDefaultGameVariant(wezr->OddballVariants, name);
		case Blam::GameType::KOTH:
			return FindDefaultGameVariant(wezr->KothVariants, name);
		case Blam::GameType::VIP:
			return FindDefaultGameVariant(wezr->VipVariants, name);
		case Blam::GameType::Juggernaut:
			return FindDefaultGameVariant(wezr->JuggernautVariants, name);
		case Blam::GameType::Territories:
			return FindDefaultGameVariant(wezr->TerritoriesVariants, name);
		case Blam::GameType::Assault:
			return FindDefaultGameVariant(wezr->AssaultVariants, name);
		case Blam::GameType::Infection:
			return FindDefaultGameVariant(wezr->InfectionVariants, name);
		default: // None, Forge
			return -1;
		}
	}

	bool LoadDefaultGameVariant(const std::string& name, uint8_t* out)
	{
		// Get a handle to the wezr tag
		typedef Blam::Tags::GameEngineSettingsDefinition* (*GetWezrTagPtr)();
		auto GetWezrTag = reinterpret_cast<GetWezrTagPtr>(0x719290);
		auto wezr = GetWezrTag();
		if (!wezr)
			return false;

		// Search through each variant type until something is found
		auto index = -1;
		int type;
		for (type = 1; type < (uint32_t)Blam::GameType::Count; type++)
		{
			index = FindDefaultGameVariant(wezr, static_cast<Blam::GameType>(type), name);
			if (index != -1)
				break;
		}
		if (type == (uint32_t)Blam::GameType::Count)
			return false;

		const auto VariantDataSize = 0x264;
		memset(out, 0, VariantDataSize);

		// Ask the game to generate the variant data
		typedef bool(*LoadBuiltInGameVariantPtr)(Blam::GameType type, int index, uint8_t* out);
		auto LoadBuiltInGameVariant = reinterpret_cast<LoadBuiltInGameVariantPtr>(0x572270);
		return LoadBuiltInGameVariant(static_cast<Blam::GameType>(type), index, out);
	}
}