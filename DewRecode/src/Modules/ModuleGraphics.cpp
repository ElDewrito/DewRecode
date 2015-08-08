#include "ModuleGraphics.hpp"
#include <sstream>
#include "../ElDorito.hpp"

namespace
{
	bool VariableSaturationUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		auto saturation = dorito.Modules.Graphics.VarSaturation->ValueFloat;
		Pointer &hueSaturationControlPtr = dorito.Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::SaturationIndex).Write(saturation);

		std::stringstream ss;
		ss << "Set saturation to " << saturation;
		returnInfo = ss.str();

		return true;
	}

	bool VariableRedHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		auto redHue = dorito.Modules.Graphics.VarRedHue->ValueFloat;
		Pointer &hueSaturationControlPtr = dorito.Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 0).Write(redHue);

		std::stringstream ss;
		ss << "Set red hue to " << redHue;
		returnInfo = ss.str();

		return true;
	}

	bool VariableGreenHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		auto greenHue = dorito.Modules.Graphics.VarGreenHue->ValueFloat;
		Pointer &hueSaturationControlPtr = dorito.Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 1).Write(greenHue);

		std::stringstream ss;
		ss << "Set green hue to " << greenHue;
		returnInfo = ss.str();

		return true;
	}

	bool VariableBlueHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();

		auto blueHue = dorito.Modules.Graphics.VarBlueHue->ValueFloat;
		Pointer &hueSaturationControlPtr = dorito.Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 2).Write(blueHue);

		std::stringstream ss;
		ss << "Set blue hue to " << blueHue;
		returnInfo = ss.str();

		return true;
	}

	bool VariableBloomUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		auto bloom = dorito.Modules.Graphics.VarBloom->ValueFloat;

		Pointer &atmoFogGlobalsPtr = dorito.Engine.GetMainTls(GameGlobals::Bloom::TLSOffset)[0];
		atmoFogGlobalsPtr(GameGlobals::Bloom::EnableIndex).Write(1L);
		atmoFogGlobalsPtr(GameGlobals::Bloom::IntensityIndex).Write(bloom);

		std::stringstream ss;
		ss << "Set bloom intensity to " << bloom;
		returnInfo = ss.str();

		return true;
	}

	bool VariableDepthOfFieldUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		auto dof = dorito.Modules.Graphics.VarDepthOfField->ValueFloat;

		Pointer &dofGlobals = dorito.Engine.GetMainTls(GameGlobals::DepthOfField::TLSOffset)[0];
		dofGlobals(GameGlobals::DepthOfField::EnableIndex).Write(true);
		dofGlobals(GameGlobals::DepthOfField::IntensityIndex).Write(dof);

		std::stringstream ss;
		ss << "Set depth of field intensity to " << dof;
		returnInfo = ss.str();

		return true;
	}

	bool VariableLetterboxUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto& dorito = ElDorito::Instance();
		auto enabled = dorito.Modules.Graphics.VarLetterbox->ValueInt;

		Pointer &cinematicGlobals = dorito.Engine.GetMainTls(GameGlobals::Cinematic::TLSOffset)[0];
		cinematicGlobals(GameGlobals::Cinematic::LetterboxIndex).Write(enabled);

		std::stringstream ss;
		ss << (enabled ? "Enabled" : "Disabled") << " letterbox";
		returnInfo = ss.str();

		return true;
	}
}

namespace Modules
{
	ModuleGraphics::ModuleGraphics() : ModuleBase("Graphics")
	{
		VarSaturation = AddVariableFloat("Saturation", "saturation", "The saturation", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsDontUpdateInitial), 1.0f, VariableSaturationUpdate);
		VarSaturation->ValueFloatMin = -10.0f;
		VarSaturation->ValueFloatMax = 10.0f;

		VarRedHue = AddVariableFloat("RedHue", "red_hue", "The red hue", eCommandFlagsDontUpdateInitial, 1.0f, VariableRedHueUpdate);
		VarRedHue->ValueFloatMin = 0.0f;
		VarRedHue->ValueFloatMax = 1.0f;

		VarGreenHue = AddVariableFloat("GreenHue", "green_hue", "The green hue", eCommandFlagsDontUpdateInitial, 1.0f, VariableGreenHueUpdate);
		VarGreenHue->ValueFloatMin = 0.0f;
		VarGreenHue->ValueFloatMax = 1.0f;

		VarBlueHue = AddVariableFloat("BlueHue", "blue_hue", "The blue hue", eCommandFlagsDontUpdateInitial, 1.0f, VariableBlueHueUpdate);
		VarBlueHue->ValueFloatMin = 0.0f;
		VarBlueHue->ValueFloatMax = 1.0f;

		// TODO: consider breaking some of these out into a separate cinematics module or possibly moving dof to camera

		VarBloom = AddVariableFloat("Bloom", "bloom", "The atmosphere bloom", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsDontUpdateInitial), 0.0f, VariableBloomUpdate);
		VarBloom->ValueFloatMin = 0.0f;
		VarBloom->ValueFloatMax = 5.0f;

		VarDepthOfField = AddVariableFloat("DepthOfField", "dof", "The camera's depth of field", eCommandFlagsDontUpdateInitial, 0.0f, VariableDepthOfFieldUpdate);
		VarDepthOfField->ValueFloatMin = 0.0f;
		VarDepthOfField->ValueFloatMax = 1.0f;

		VarLetterbox = AddVariableInt("Letterbox", "letterbox", "A cinematic letterbox.", eCommandFlagsDontUpdateInitial, 0, VariableLetterboxUpdate);
		VarLetterbox->ValueIntMin = 0;
		VarLetterbox->ValueIntMax = 1;
	}
}