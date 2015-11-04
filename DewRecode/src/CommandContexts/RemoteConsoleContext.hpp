#include <ElDorito/ElDorito.hpp>

class RemoteConsoleContext : public CommandContext
{
private:
	bool isAuthed = false;
	int passwordTries = 0;
	const int MaxPasswordTries = 5;

public:
	SOCKET ClientSocket;

	RemoteConsoleContext(SOCKET socket);
	void Disconnect();

	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal() { return false; }
	bool IsChat() { return false; }

	int GetPeerIdx() { return 0; }
	Blam::Network::Session* GetSession() { return 0; }
};