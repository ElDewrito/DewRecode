#pragma once
#include <memory>
#include <ElDorito/CommandProvider.hpp>
#include "../Patches/GraphicsPatchProvider.hpp"

namespace Graphics
{
	class GraphicsCommandProvider : public CommandProvider
	{
	private:
		std::shared_ptr<GraphicsPatchProvider> graphicsPatches;

	public:
		Command* VarSaturation;
		Command* VarRedHue;
		Command* VarGreenHue;
		Command* VarBlueHue;
		Command* VarBloom;
		Command* VarDepthOfField;
		Command* VarLetterbox;
		Command* VarSSAO;
		Command* VarSSAOArg1;
		Command* VarSSAOArg2;
		Command* VarSSAOArg3;

		explicit GraphicsCommandProvider(std::shared_ptr<GraphicsPatchProvider> graphicsPatches);

		virtual void RegisterVariables(ICommandManager* manager) override;
		
		bool VariableSaturationUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableRedHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableGreenHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableBlueHueUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableBloomUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableDepthOfFieldUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableLetterboxUpdate(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableSSAOUpdate(const std::vector<std::string>& Arguments, CommandContext& context);

		bool VariableSSAOArg1Update(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableSSAOArg2Update(const std::vector<std::string>& Arguments, CommandContext& context);
		bool VariableSSAOArg3Update(const std::vector<std::string>& Arguments, CommandContext& context);
	};
}