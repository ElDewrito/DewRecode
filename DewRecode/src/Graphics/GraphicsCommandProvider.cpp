#include "GraphicsCommandProvider.hpp"
#include "../ElDorito.hpp"

namespace Graphics
{
	void GraphicsCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarSaturation = manager->Add(Command::CreateVariableFloat("Graphics", "Saturation", "saturation", "The saturation", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsDontUpdateInitial), 1.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableSaturationUpdate)));
		VarSaturation->ValueFloatMin = -10.0f;
		VarSaturation->ValueFloatMax = 10.0f;

		VarRedHue = manager->Add(Command::CreateVariableFloat("Graphics", "RedHue", "red_hue", "The red hue", eCommandFlagsDontUpdateInitial, 1.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableRedHueUpdate)));
		VarRedHue->ValueFloatMin = 0.0f;
		VarRedHue->ValueFloatMax = 1.0f;

		VarGreenHue = manager->Add(Command::CreateVariableFloat("Graphics", "GreenHue", "green_hue", "The green hue", eCommandFlagsDontUpdateInitial, 1.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableGreenHueUpdate)));
		VarGreenHue->ValueFloatMin = 0.0f;
		VarGreenHue->ValueFloatMax = 1.0f;

		VarBlueHue = manager->Add(Command::CreateVariableFloat("Graphics", "BlueHue", "blue_hue", "The blue hue", eCommandFlagsDontUpdateInitial, 1.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableBlueHueUpdate)));
		VarBlueHue->ValueFloatMin = 0.0f;
		VarBlueHue->ValueFloatMax = 1.0f;

		// TODO: consider breaking some of these out into a separate cinematics module or possibly moving dof to camera

		VarBloom = manager->Add(Command::CreateVariableFloat("Graphics", "Bloom", "bloom", "The atmosphere bloom", (CommandFlags)(eCommandFlagsArchived | eCommandFlagsDontUpdateInitial), 0.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableBloomUpdate)));
		VarBloom->ValueFloatMin = 0.0f;
		VarBloom->ValueFloatMax = 5.0f;

		VarDepthOfField = manager->Add(Command::CreateVariableFloat("Graphics", "DepthOfField", "dof", "The camera's depth of field", eCommandFlagsDontUpdateInitial, 0.0f, BIND_COMMAND(this, &GraphicsCommandProvider::VariableDepthOfFieldUpdate)));
		VarDepthOfField->ValueFloatMin = 0.0f;
		VarDepthOfField->ValueFloatMax = 1.0f;

		VarLetterbox = manager->Add(Command::CreateVariableInt("Graphics", "Letterbox", "letterbox", "A cinematic letterbox.", eCommandFlagsDontUpdateInitial, 0, BIND_COMMAND(this, &GraphicsCommandProvider::VariableLetterboxUpdate)));
		VarLetterbox->ValueIntMin = 0;
		VarLetterbox->ValueIntMax = 1;
	}

	bool GraphicsCommandProvider::VariableSaturationUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto saturation = VarSaturation->ValueFloat;

		Pointer &hueSaturationControlPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::SaturationIndex).Write(saturation);

		std::stringstream ss;
		ss << "Set saturation to " << saturation;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableRedHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto redHue = VarRedHue->ValueFloat;

		Pointer &hueSaturationControlPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 0).Write(redHue);

		std::stringstream ss;
		ss << "Set red hue to " << redHue;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableGreenHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto greenHue = VarGreenHue->ValueFloat;

		Pointer &hueSaturationControlPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 1).Write(greenHue);

		std::stringstream ss;
		ss << "Set green hue to " << greenHue;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableBlueHueUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto blueHue = VarBlueHue->ValueFloat;

		Pointer &hueSaturationControlPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Graphics::TLSOffset)[0];
		hueSaturationControlPtr(GameGlobals::Graphics::GraphicsOverrideIndex).Write(true);
		hueSaturationControlPtr(GameGlobals::Graphics::ColorIndex + sizeof(float) * 2).Write(blueHue);

		std::stringstream ss;
		ss << "Set blue hue to " << blueHue;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableBloomUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto bloom = VarBloom->ValueFloat;

		Pointer &atmoFogGlobalsPtr = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Bloom::TLSOffset)[0];
		atmoFogGlobalsPtr(GameGlobals::Bloom::EnableIndex).Write(1L);
		atmoFogGlobalsPtr(GameGlobals::Bloom::IntensityIndex).Write(bloom);

		std::stringstream ss;
		ss << "Set bloom intensity to " << bloom;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableDepthOfFieldUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto dof = VarDepthOfField->ValueFloat;

		Pointer &dofGlobals = ElDorito::Instance().Engine.GetMainTls(GameGlobals::DepthOfField::TLSOffset)[0];
		dofGlobals(GameGlobals::DepthOfField::EnableIndex).Write(true);
		dofGlobals(GameGlobals::DepthOfField::IntensityIndex).Write(dof);

		std::stringstream ss;
		ss << "Set depth of field intensity to " << dof;
		returnInfo = ss.str();

		return true;
	}

	bool GraphicsCommandProvider::VariableLetterboxUpdate(const std::vector<std::string>& Arguments, std::string& returnInfo)
	{
		auto enabled = VarLetterbox->ValueInt;

		Pointer &cinematicGlobals = ElDorito::Instance().Engine.GetMainTls(GameGlobals::Cinematic::TLSOffset)[0];
		cinematicGlobals(GameGlobals::Cinematic::LetterboxIndex).Write(enabled);

		std::stringstream ss;
		ss << (enabled ? "Enabled" : "Disabled") << " letterbox";
		returnInfo = ss.str();

		return true;
	}
}