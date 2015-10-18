#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace Updater
{
	class UpdaterCommandProvider : public ICommandProvider
	{
	public:
		Command* VarUpdaterBranch;

		virtual void RegisterVariables(ICommandManager* manager) override;

		bool CommandCheck(const std::vector<std::string>& Arguments, ICommandContext& context);
		std::string CheckForUpdates(const std::string& branch);
	};
}