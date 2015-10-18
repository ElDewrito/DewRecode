#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "../Patches/CameraPatchProvider.hpp"

namespace Camera
{
	class CameraCommandProvider : public ICommandProvider
	{
	private:
		std::shared_ptr<CameraPatchProvider> cameraPatches;

	public:
		Command* VarCrosshair;
		Command* VarFov;
		Command* VarHideHud;
		Command* VarMode;
		Command* VarSpeed;
		Command* VarSave;
		Command* VarLoad;
		Command* VarSpectatorIndex;

		explicit CameraCommandProvider(std::shared_ptr<CameraPatchProvider> cameraPatches);

		virtual void RegisterVariables(ICommandManager* manager) override;
		
		bool VariableCrosshairUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableFovUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableHideHudUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableSpeedUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableSpectatorIndexUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
		bool VariableModeUpdate(const std::vector<std::string>& Arguments, ICommandContext& context);
	};
}