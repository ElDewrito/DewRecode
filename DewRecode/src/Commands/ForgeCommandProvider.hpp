#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../Patches/ForgePatchProvider.hpp"

namespace Forge
{
	class ForgeCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<ForgePatchProvider> forgePatches;

	public:
		explicit ForgeCommandProvider(std::shared_ptr<ForgePatchProvider> forgePatches);

		virtual std::vector<Command> GetCommands() override;
		
		bool CommandDeleteItem(const std::vector<std::string>& Arguments, ICommandContext& context);
		void DeleteItem();
	};
}