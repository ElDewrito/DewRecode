#pragma once
#include <string>
#include "UIWindow.hpp"

#define IM_ARRAYSIZE(_ARR)  ((int)(sizeof(_ARR)/sizeof(*_ARR)))

typedef std::function<void(const std::string& boxTag, const std::string& result)> MsgBoxCallback;

enum class ChatWindowTab
{
	GlobalChat,
	GameChat,
	Rcon,
	Count
};

/*
if you want to make changes to this interface create a new IUtils002 class and make them there, then edit Utils class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IUserInterface001
{
public:

	virtual bool IsShown() = 0;
	virtual bool ShowUI(bool show) = 0;
	virtual bool ShowChat(bool show) = 0;
	virtual bool ShowConsole(bool show) = 0;
	virtual bool ShowMessageBox(const std::string& title, const std::string& message, const std::string& tag, std::vector<std::string> choices, MsgBoxCallback callback) = 0;
	virtual void WriteToConsole(const std::string& text) = 0;
	virtual void AddToChat(const std::string& text, ChatWindowTab tab) = 0;
	virtual ChatWindowTab SwitchToTab(ChatWindowTab tab) = 0;
};

#define USERINTERFACE_INTERFACE_VERSION001 "UserInterface001"

/* use this class if you're updating IUserInterface after we've released a build
also update the IUserInterface typedef and USERINTERFACE_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class UserInterface002 : public UserInterface001
{

};

#define USERINTERFACE_INTERFACE_VERSION001 "UserInterface002"*/

typedef IUserInterface001 IUserInterface;
#define USERINTEFACE_INTERFACE_LATEST USERINTERFACE_INTERFACE_VERSION001
