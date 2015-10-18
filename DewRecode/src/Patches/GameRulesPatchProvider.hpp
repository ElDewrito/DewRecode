#pragma once
#include <ElDorito/IPatchProvider.hpp>

namespace GameRules
{
	class GameRulesPatchProvider : public IPatchProvider
	{
		Patch* PatchSprintDisable;
		Patch* PatchSprintUnlimited;

	public:
		GameRulesPatchProvider();

		virtual void RegisterCallbacks(IEngine* engine) override;

		bool SprintEnable(bool enabled);
		bool SprintUnlimited(bool enabled);

		void TickCallback(const std::chrono::duration<double>& deltaTime);
	};
}