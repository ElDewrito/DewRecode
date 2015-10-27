#include "UICommandProvider.hpp"
#include "../ElDorito.hpp"

namespace UI
{
	UICommandProvider::UICommandProvider(std::shared_ptr<UIPatchProvider> uiPatches)
	{
		this->uiPatches = uiPatches;
	}

	std::vector<Command> UICommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("UI", "ShowChat", "chat_show", "Shows/hides the chat", eCommandFlagsNone, BIND_COMMAND(this, &UICommandProvider::CommandShowChat)),
			Command::CreateCommand("UI", "ShowConsole", "console_show", "Shows/hides the console", eCommandFlagsNone, BIND_COMMAND(this, &UICommandProvider::CommandShowConsole)),
			Command::CreateCommand("UI", "ShowH3UI", "show_ui", "Attempts to force a H3UI widget to open", eCommandFlagsNone, BIND_COMMAND(this, &UICommandProvider::CommandShowH3UI)),
			Command::CreateCommand("UI", "SettingsMenu", "settings", "Opens the ElDewrito settings menu", eCommandFlagsNone, BIND_COMMAND(this, &UICommandProvider::CommandSettingsMenu), { "menuName(string) The menu to open, can be blank" }),
		};

		return commands;
	}

	bool UICommandProvider::CommandShowChat(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		bool show = true;
		if (Arguments.size() > 0)
			if (Arguments[0] == "false" || Arguments[0] == "no" || Arguments[0] == "0")
				show = false;

		ShowChat(show);

		return true;
	}

	void UICommandProvider::ShowChat(bool show)
	{
		ElDorito::Instance().UserInterface.ShowChat(show);
	}

	bool UICommandProvider::CommandShowConsole(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		bool show = true;
		if (Arguments.size() > 0)
			if (Arguments[0] == "false" || Arguments[0] == "no" || Arguments[0] == "0")
				show = false;

		ShowConsole(show);

		return true;
	}

	void UICommandProvider::ShowConsole(bool show)
	{
		ElDorito::Instance().UserInterface.ShowConsole(show);
	}

	bool UICommandProvider::CommandShowH3UI(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		if (Arguments.size() <= 0)
		{
			context.WriteOutput("You must specify at least a dialog ID!");
			return false;
		}

		unsigned int dialogStringId = 0;
		int dialogArg1 = 0; // todo: figure out a better name for this
		int dialogFlags = 4;
		unsigned int dialogParentStringId = 0x1000C;
		try
		{
			dialogStringId = std::stoul(Arguments[0], 0, 0);
			if (Arguments.size() > 1)
				dialogArg1 = std::stoi(Arguments[1], 0, 0);
			if (Arguments.size() > 2)
				dialogFlags = std::stoi(Arguments[2], 0, 0);
			if (Arguments.size() > 3)
				dialogParentStringId = std::stoul(Arguments[3], 0, 0);
		}
		catch (std::invalid_argument)
		{
			context.WriteOutput("Invalid argument given.");
			return false;
		}
		catch (std::out_of_range)
		{
			context.WriteOutput("Invalid argument given.");
			return false;
		}

		ShowH3UI(dialogStringId, dialogArg1, dialogFlags, dialogParentStringId);

		context.WriteOutput("Sent Show_UI notification to game.");
		return true;
	}

	void UICommandProvider::ShowH3UI(uint32_t stringId, int32_t arg1, int32_t flags, uint32_t parentStringId)
	{
		uiPatches->DialogStringId = stringId;
		uiPatches->DialogArg1 = arg1;
		uiPatches->DialogFlags = flags;
		uiPatches->DialogParentStringId = parentStringId;
		uiPatches->DialogShow = true;
	}

	bool UICommandProvider::CommandSettingsMenu(const std::vector<std::string>& Arguments, CommandContext& context)
	{
		std::string& menu = std::string();
		if (Arguments.size() > 0)
			menu = ElDorito::Instance().Utils.ToLower(Arguments[0]);

		auto ret = ShowSettingsMenu(menu);
		if (!ret)
		{
			context.WriteOutput("Usage: Game.SettingsMenu <Player/Server/IRC/Setup>");
			return false;
		}

		return true;
	}

	bool UICommandProvider::ShowSettingsMenu(const std::string& menuName)
	{
		auto& dorito = ElDorito::Instance();

		if (menuName.empty())
		{
			//dorito.Engine.ShowMessageBox("ElDewrito settings", "Which settings do you want to change?", "settingsChoice", { "Player", "Server", "IRC", "Return" }, BIND_CALLBACK2(this, &UICommandProvider::CallbackMsgBox));
			return true;
		}
		const std::string& menu = dorito.Utils.ToLower(menuName);

		/*if (menu == "player")
			dorito.Engine.ShowMessageBox("Player settings", "", "playerSettingsChoice", { "Player Name", "Return" }, BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingsMsgBox));
		else if (menu == "server")
			dorito.Engine.ShowMessageBox("Server settings", "", "serverSettingsChoice", { "Server Name", "Server Password", "Return" }, BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingsMsgBox));
		else if (menu == "irc")
			dorito.Engine.ShowMessageBox("IRC settings", "", "ircSettingsChoice", { "IRC Server Host", "IRC Server Port", "IRC Global Channel", "Return" }, BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingsMsgBox));
		else if (menu == "setup")
			dorito.Engine.ShowInputBox("Player name", "Enter the nickname you want to be known as online.\nPress ENTER to confirm.", "Player.Name", "Player.Name", BIND_CALLBACK2(this, &UICommandProvider::CallbackInitialSetup));
		else
			return false;*/

		return true;
	}

	void UICommandProvider::CallbackMsgBox(const std::string& boxTag, const std::string& result)
	{
		ShowSettingsMenu(result);
	}

	void UICommandProvider::CallbackSettingsMsgBox(const std::string& boxTag, const std::string& result)
	{
		auto& dorito = ElDorito::Instance();
		/*if (result == "Player Name")
			dorito.Engine.ShowInputBox("Player name", "Enter the nickname you want to be known as online.\nPress ENTER to confirm.", "Player.Name", "Player.Name", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else if (result == "Server Name")
			dorito.Engine.ShowInputBox("Server name", "Enter the name of your server.\nPress ENTER to confirm.", "Server.Name", "Server.Name", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else if (result == "Server Password")
			dorito.Engine.ShowInputBox("Server password", "Enter the password for your server.\nPress ENTER to confirm.", "Server.Password", "Server.Password", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else if (result == "IRC Server Host")
			dorito.Engine.ShowInputBox("IRC server hostname", "Enter the hostname of the IRC server.\nPress ENTER to confirm.", "IRC.Server", "IRC.Server", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else if (result == "IRC Server Port")
			dorito.Engine.ShowInputBox("IRC server port", "Enter the port number of the IRC server.\nPress ENTER to confirm.", "IRC.ServerPort", "IRC.ServerPort", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else if (result == "IRC Global Channel")
			dorito.Engine.ShowInputBox("Global channel name", "Enter the name of the global chat channel.\nPress ENTER to confirm.", "IRC.GlobalChannel", "IRC.GlobalChannel", BIND_CALLBACK2(this, &UICommandProvider::CallbackSettingChanged));
		else
		{
			CallbackMsgBox("", "");
			return;
		}*/
		dorito.CommandManager.Execute("WriteConfig", dorito.CommandManager.LogFileContext);
	}

	void UICommandProvider::CallbackInitialSetup(const std::string& boxTag, const std::string& result)
	{
		auto& dorito = ElDorito::Instance();
		dorito.CommandManager.Execute(boxTag + " \"" + result + "\"", dorito.CommandManager.LogFileContext);
		//if (boxTag == "Player.Name")
		//	dorito.Engine.ShowMessageBox("Name set", "Your name has been set to " + result + "\nYou can change your name any time you're outside a game\nby opening the ElDewrito settings menu (Default key: F5)", "", { "OK" }, nullptr);
	}

	void UICommandProvider::CallbackSettingChanged(const std::string& boxTag, const std::string& result)
	{
		auto& dorito = ElDorito::Instance();
		dorito.CommandManager.Execute(boxTag + " \"" + result + "\"", dorito.CommandManager.LogFileContext);
	}
}