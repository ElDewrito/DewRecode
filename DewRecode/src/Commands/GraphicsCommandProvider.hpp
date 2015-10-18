#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace Graphics
{
	class GraphicsCommandProvider : public ICommandProvider
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
		
		bool VariableSaturationUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableRedHueUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableGreenHueUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableBlueHueUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableBloomUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableDepthOfFieldUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableLetterboxUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
	};
}