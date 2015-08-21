#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "ForgePatchProvider.hpp"

namespace Forge
{
	class ForgeCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<ForgePatchProvider> forgePatches;

	public:
		explicit ForgeCommandProvider(std::shared_ptr<ForgePatchProvider> forgePatches);

		virtual std::vector<Command> GetCommands() override;
		
		bool CommandDeleteItem(const std::vector<std::string>& Arguments, std::string& returnInfo);
		void DeleteItem();
	};
}