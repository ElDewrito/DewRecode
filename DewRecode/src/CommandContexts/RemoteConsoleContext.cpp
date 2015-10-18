#include "RemoteConsoleContext.hpp"
#include "../ElDorito.hpp"
#include <algorithm>

RemoteConsoleContext::RemoteConsoleContext(SOCKET clientSocket)
{
	auto& dorito = ElDorito::Instance();
	ClientSocket = clientSocket;

	std::string motd = "ElDewrito " + dorito.Engine.GetDoritoVersionString() + " Remote Console";
	if (!dorito.ServerCommands->VarRconPassword->ValueString.empty())
		motd += "\nPASSWORDED: This console is password protected.\nEnter PASSWORD <password> to authenticate.";

	WriteOutput(motd);

	isAuthed = dorito.ServerCommands->VarRconPassword->ValueString.empty();
}

void RemoteConsoleContext::HandleInput(const std::string& input)
{
	auto& dorito = ElDorito::Instance();

	if (!dorito.ServerCommands->VarRconPassword->ValueString.empty())
	{
		if (input.length() > 9 && input.substr(0, 9) == "PASSWORD ")
		{
			if (isAuthed)
			{
				WriteOutput("You are already authenticated!");
				return;
			}

			if (passwordTries >= MaxPasswordTries)
			{
				WriteOutput("You have exceeded the maximum number of login attempts.");
				Disconnect();
				return;
			}

			auto passwd = input.substr(9);
			dorito.Utils.ReplaceString(passwd, "\n", "");
			dorito.Utils.ReplaceString(passwd, "\r", "");

			if (passwd != dorito.ServerCommands->VarRconPassword->ValueString)
			{
				WriteOutput("Incorrect password.");
				passwordTries++;
			}
			else
			{
				WriteOutput("You have logged in.");
				isAuthed = true;
			}
			return;
		}

		if (!isAuthed)
		{
			WriteOutput("PASSWORDED: This console is password protected.\nEnter PASSWORD <password> to authenticate.");
			return;
		}
	}

	dorito.CommandManager.Execute(input, *this);
}

void RemoteConsoleContext::WriteOutput(const std::string& output)
{
	if (!ClientSocket)
		return;

	auto sout = output + "\n";
	ElDorito::Instance().Utils.ReplaceString(sout, "\n", "\r\n");
	send(ClientSocket, sout.c_str(), sout.length(), 0);
}

void RemoteConsoleContext::Disconnect()
{
	if (!ClientSocket)
		return;

	closesocket(ClientSocket);
	ClientSocket = 0;

	auto* vec = &ElDorito::Instance().ServerCommands->RconContexts;
	vec->erase(std::remove(vec->begin(), vec->end(), this), vec->end());
	delete this;
}