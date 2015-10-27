#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>

namespace Graphics
{
	class GraphicsCommandProvider : public CommandProvider
	{
	public:
		Command* VarSaturation;
		Command* VarRedHue;
		Command* VarGreenHue;
		Command* VarBlueHue;
		Command* VarBloom;
		Command* VarDepthOfField;
		Command* VarLetterbox;

		virtual void RegisterVariables(ICommandManager* manager) override;
		
		bool VariableSaturationUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableRedHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableGreenHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableBlueHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableBloomUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableDepthOfFieldUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableLetterboxUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
	};
}