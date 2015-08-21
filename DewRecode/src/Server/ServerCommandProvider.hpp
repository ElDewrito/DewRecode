#pragma once
#include <ElDorito/ICommandProvider.hpp>

namespace Server
{
	class ServerCommandProvider : public ICommandProvider
	{
	private:
		void announceStatsThread();

	public:
		Command* VarCheats;
		Command* VarCountdown;
		Command* VarMaxPlayers;
		Command* VarMode;
		Command* VarPort;

		BYTE SyslinkData[0x176];

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* manager) override;

		void CallbackEndGame(void* param);

		bool CommandAnnounceStats(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void AnnounceStats();

		bool CommandConnect(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void Connect();

		bool CommandMode(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetLobbyMode(Blam::ServerLobbyMode mode);

		bool VariableMaxPlayersUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetMaxPlayers(int maxPlayers);

		bool VariableCountdownUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetCountdown(int seconds);
	};
}