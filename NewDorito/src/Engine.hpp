#pragma once
#include <ElDorito/ElDorito.hpp>

// handles game ticks and tick callbacks for different modules/plugins
class Engine : public IEngine001
{
public:
	bool RegisterTickCallback(TickCallbackFunc callback);

	void Tick(const std::chrono::duration<double>& DeltaTime);

private:
	std::vector<TickCallbackFunc> tickCallbacks;
};
