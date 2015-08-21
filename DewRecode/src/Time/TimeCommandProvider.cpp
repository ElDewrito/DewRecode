#include "TimeCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace Time
{
	void TimeCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarGameSpeed = manager->Add(Command::CreateVariableFloat("Time", "GameSpeed", "game_speed", "The game's speed", (CommandFlags)(eCommandFlagsCheat | eCommandFlagsDontUpdateInitial), 1.0f, BIND_COMMAND(this, &TimeCommandProvider::VariableGameSpeedUpdate)));
		VarGameSpeed->ValueFloatMin = 0.0f;
		VarGameSpeed->ValueFloatMax = 10.0f;
	}

	bool TimeCommandProvider::VariableGameSpeedUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		auto speed = VarGameSpeed->ValueFloat;
		Pointer &gameTimeGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Time::TLSOffset)[0];
		gameTimeGlobalsPtr(GameGlobals::Time::GameSpeedIndex).Write(speed);

		std::stringstream ss;
		ss << "Game speed set to " << speed;
		returnInfo = ss.str();

		return true;
	}
}