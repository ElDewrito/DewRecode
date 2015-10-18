#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../Patches/DebugPatchProvider.hpp"

namespace Debug
{
	enum class DebugLoggingModes
	{
		Network = 1 << 0,
		SSL = 1 << 1,
		UI = 1 << 2,
		Game1 = 1 << 3,
		Game2 = 1 << 4,
		Packets = 1 << 5,
	};

	class DebugCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<DebugPatchProvider> debugPatches;

	public:
		int DebugFlags;
		std::vector<std::string> FiltersExclude;
		std::vector<std::string> FiltersInclude;

		Command* VarLogName;
		Command* VarMemcpySrc;
		Command* VarMemcpyDst;
		Command* VarMemsetDst;

		explicit DebugCommandProvider(std::shared_ptr<DebugPatchProvider> debugPatches);

		virtual std::vector<Command> GetCommands() override;
		virtual void RegisterVariables(ICommandManager* manager) override;

		bool CommandLogMode(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool CommandLogFilter(const std::vector<std::string>& Arguments, ICommandContext& context);

		bool VariableMemcpySrcUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableMemcpyDstUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableMemsetDstUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);

	};
}