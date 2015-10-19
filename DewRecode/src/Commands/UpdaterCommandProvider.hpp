#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace Updater
{
	class UpdaterCommandProvider : public ICommandProvider
	{
	public:
		Command* VarBranch;
		Command* VarCheckOnLaunch;

		virtual void RegisterVariables(ICommandManager* manager) override;
		virtual void RegisterCallbacks(IEngine* engine) override;

		bool CommandCheck(const std::vector<std::string>& Arguments, ICommandContext& context);
		std::string CheckForUpdates(const std::string& branch);
		void CheckForUpdatesCallback(void* param);

		bool CommandSignManifest(const std::vector<std::string>& Arguments, ICommandContext& context);
		std::string SignManifest(const std::string& manifestPath, const std::string& privateKey);
	};
}