#pragma once
#include <ElDorito/ICommandProvider.hpp>

#define WM_RCON WM_USER + 1337
#define WM_INFOSERVER WM_RCON + 1
class RemoteConsoleContext;

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
		bool IsInfoServerRunning()
		{
			return infoServerRunning;
		}

		bool IsRconServerRunning()
		{
			return rconServerRunning;
		}

		Command* VarCheats;
		Command* VarCountdown;
		Command* VarName;
		Command* VarMaxPlayers;
		Command* VarMode;
		Command* VarLobbyType;
		Command* VarPassword;
		Command* VarPort;
		Command* VarShouldAnnounce;
		Command* VarRconPort;
		Command* VarRconPassword;

		BYTE SyslinkData[0x176];

		std::vector<RemoteConsoleContext*> RconContexts;

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* manager) override;

		void CallbackEndGame(void* param);
		void CallbackInfoServerStart(void* param);
		void CallbackInfoServerStop(void* param);
		void CallbackRemoteConsoleStart(void* param);
		void CallbackPongReceived(void* param);
		void CallbackLifeCycleStateChanged(void* param);

		bool CommandAnnounce(const std::vector<std::string>& Arguments, ICommandContext& context);
		void AnnounceServer();

		bool CommandUnannounce(const std::vector<std::string>& Arguments, ICommandContext& context);
		void UnannounceServer();

		bool CommandAnnounceStats(const std::vector<std::string>& Arguments, ICommandContext& context);
		void AnnounceStats();

		bool CommandConnect(const std::vector<std::string>& Arguments, ICommandContext& context);
		void Connect();

		bool CommandKickPlayer(const std::vector<std::string>& Arguments, ICommandContext& context);
		KickPlayerReturnCode KickPlayer(const std::string& playerName);
		bool KickPlayer(int peerIdx);

		bool CommandListPlayers(const std::vector<std::string>& Arguments, ICommandContext& context);
		std::string ListPlayers();

		bool VariableModeUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool SetLobbyMode(Blam::ServerLobbyMode mode);

		bool VariableLobbyTypeUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool SetLobbyType(Blam::ServerLobbyType type);

		bool VariableCountdownUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool SetCountdown(int seconds);

		bool VariableMaxPlayersUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool SetMaxPlayers(int maxPlayers);

		bool VariableShouldAnnounceUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		void SetShouldAnnounce(bool shouldAnnounce);

		bool CommandPing(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool Ping(const std::string& address, ICommandContext& context);

		LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	};
}