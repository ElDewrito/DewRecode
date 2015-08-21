#pragma once
#include <memory>
#include <ElDorito/ICommandProvider.hpp>

namespace Time
{
	class TimeCommandProvider : public ICommandProvider
	{
	public:
		Command* VarGameSpeed;

		virtual void RegisterVariables(ICommandManager* manager) override;
		
		bool VariableGameSpeedUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo);
	};
}