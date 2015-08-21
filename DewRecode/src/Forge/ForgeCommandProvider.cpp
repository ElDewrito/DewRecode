#include "ForgeCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace Forge
{
	ForgeCommandProvider::ForgeCommandProvider(std::shared_ptr<ForgePatchProvider> forgePatches)
	{
		this->forgePatches = forgePatches;
	}

	std::vector<Command> ForgeCommandProvider::GetCommands()
	{
		std::vector<Command> commands =
		{
			Command::CreateCommand("Forge", "DeleteItem", "forge_delete", "Deletes the Forge item under the crosshairs", eCommandFlagsNone, BIND_COMMAND(this, &ForgeCommandProvider::CommandDeleteItem))
		};

		return commands;
	}

	bool ForgeCommandProvider::CommandDeleteItem(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		DeleteItem();
		return true;
	}

	void ForgeCommandProvider::DeleteItem()
	{
		forgePatches->SignalDelete();
	}
}