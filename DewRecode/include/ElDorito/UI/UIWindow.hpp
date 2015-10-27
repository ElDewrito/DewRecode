#pragma once
#include "../ElDorito.hpp"

class UIWindow
{
public:
	virtual void Draw() {}
	virtual bool SetVisible(bool visible) { return false; }
	virtual bool GetVisible() { return false; }
};