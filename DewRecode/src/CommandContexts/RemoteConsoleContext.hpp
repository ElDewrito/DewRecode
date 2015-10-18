#include <ElDorito/ElDorito.hpp>

class RemoteConsoleContext : public ICommandContext
{
private:
	bool isAuthed = false;
	int passwordTries = 0;
	const int MaxPasswordTries = 5;

public:
	SOCKET ClientSocket;

	RemoteConsoleContext(SOCKET socket);

	void HandleInput(const std::string& input);
	void WriteOutput(const std::string& output);

	bool IsInternal()
	{
		return false;
	}

	void Disconnect();
};