#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>
#include "CameraPatchProvider.hpp"

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
		
		bool VariableCrosshairUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableFovUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableHideHudUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableSpeedUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableSpectatorIndexUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
		bool VariableModeUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
	};
}