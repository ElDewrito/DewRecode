#include "GameRulesPatchProvider.hpp"
#include "../ElDorito.hpp"

namespace GameRules
{
	GameRulesPatchProvider::GameRulesPatchProvider()
	{
		auto& patches = ElDorito::Instance().PatchManager;

		PatchSprintDisable = patches.AddPatch("SprintDisable", 0x53E26B, { 0x90, 0xE9 });
		PatchSprintUnlimited = patches.AddPatch("SprintUnlimited", 0x53E5D1, 0, 1);
	}

	void GameRulesPatchProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnTick(BIND_CALLBACK(this, &GameRulesPatchProvider::TickCallback));
	}

	void GameRulesPatchProvider::TickCallback(const std::chrono::duration<double>& deltaTime)
	{
		auto persistentUserDataChud = ElDorito::Instance().Engine.GetMainTls(GameGlobals::PersistentUserDataChud::TLSOffset)[0];
		if (persistentUserDataChud)
			persistentUserDataChud(GameGlobals::PersistentUserDataChud::SprintMeterOffset).Write<bool>(!PatchSprintDisable->Enabled);
	}

	bool GameRulesPatchProvider::SprintEnable(bool enabled)
	{
		ElDorito::Instance().PatchManager.EnablePatch(PatchSprintDisable, !enabled);
		return enabled;
	}

	bool GameRulesPatchProvider::SprintUnlimited(bool enabled)
	{
		ElDorito::Instance().PatchManager.EnablePatch(PatchSprintUnlimited, enabled);
		return enabled;
	}
}
