#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>

namespace Updater
{
	class UpdaterCommandProvider : public CommandProvider
	{
	public:
		Command* VarBranch;
		Command* VarCheckOnLaunch;

		virtual std::vector<Command> UpdaterCommandProvider::GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		bool CommandCheck(const std::vector<std::string>& Arguments, CommandContext& context);
		std::string CheckForUpdates(const std::string& branch);
		void CheckForUpdatesCallback(void* param);

		bool CommandSignManifest(const std::vector<std::string>& Arguments, CommandContext& context);
		std::string SignManifest(const std::string& manifestPath, const std::string& privateKey);
	};
}