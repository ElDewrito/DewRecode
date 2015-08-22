#pragma once
#include <ElDorito/ICommandProvider.hpp>

#define WM_RCON WM_USER + 1337
#define WM_INFOSERVER WM_RCON + 1

namespace Server
{
	enum class KickPlayerReturnCode
	{
		Success,
		NoSession,
		NotHost,
		KickFailed,
		NotFound
	};

	class ServerCommandProvider : public ICommandProvider
	{
	private:
		void announceServerThread();
		void unannounceServerThread();
		void announceStatsThread();

		SOCKET infoSocket;
		SOCKET rconSocket;

		bool infoServerRunning = false;
		bool rconServerRunning = false;

		time_t lastAnnounce = 0;
		const time_t serverContactTimeLimit = 30 + (2 * 60);
	public:
		Command* VarCheats;
		Command* VarCountdown;
		Command* VarName;
		Command* VarMaxPlayers;
		Command* VarMode;
		Command* VarPassword;
		Command* VarPort;
		Command* VarShouldAnnounce;

		BYTE SyslinkData[0x176];

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* manager) override;

		void CallbackEndGame(void* param);
		void CallbackInfoServerStart(void* param);
		void CallbackInfoServerStop(void* param);
		void CallbackRemoteConsoleStart(void* param);

		bool CommandAnnounce(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void AnnounceServer();

		bool CommandUnannounce(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void UnannounceServer();

		bool CommandAnnounceStats(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void AnnounceStats();

		bool CommandConnect(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void Connect();

		bool CommandKickPlayer(const std::vector<std::string>& Arguments, std::string& returnInfo);
		KickPlayerReturnCode KickPlayer(const std::string& playerName);
		bool KickPlayer(int peerIdx);

		bool CommandListPlayers(const std::vector<std::string>& Arguments, std::string& returnInfo);
		std::string ListPlayers();

		bool VariableModeUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetLobbyMode(Blam::ServerLobbyMode mode);

		bool VariableCountdownUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetCountdown(int seconds);

		bool VariableMaxPlayersUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool SetMaxPlayers(int maxPlayers);

		bool VariableShouldAnnounceUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void SetShouldAnnounce(bool shouldAnnounce);

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}