#include "ModuleVoIP.hpp"
#include "TeamspeakClient.hpp"
#include "TeamspeakServer.hpp"
#include <teamspeak/public_definitions.h>
#include <teamspeak/public_errors.h>
#include <teamspeak/clientlib_publicdefinitions.h>
#include <teamspeak/clientlib.h>

Modules::ModuleVoIP VoipModule;

namespace
{
	bool VariablePushToTalkUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		unsigned int error;
		uint64 scHandlerID = VoIPGetscHandlerID();
		if (scHandlerID != NULL)
			if (VoipModule.VarVoIPPushToTalk->ValueInt == 0)
			{
				if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "vad", "true")) != ERROR_ok)
				{
					returnInfo = "Error setting voice activation detection. Error: " + std::to_string(error);
					return false;
				}

				if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "voiceactivation_level", VoipModule.VarVoIPVADLevel->ValueString.c_str())) != ERROR_ok)
				{
					returnInfo = "Error setting voice activation level. Error: " + std::to_string(error);
					return false;
				}

				if ((error = ts3client_setClientSelfVariableAsInt(scHandlerID, CLIENT_INPUT_DEACTIVATED, INPUT_ACTIVE)) != ERROR_ok)
				{
					char* errorMsg;
					if (ts3client_getErrorMessage(error, &errorMsg) != ERROR_ok)
						ts3client_freeMemory(errorMsg);
					else
						returnInfo = errorMsg;
					return false;
				}

				if (ts3client_flushClientSelfUpdates(scHandlerID, NULL) != ERROR_ok)
				{
					char* errorMsg;
					if (ts3client_getErrorMessage(error, &errorMsg) != ERROR_ok)
						ts3client_freeMemory(errorMsg);
					else
						returnInfo = errorMsg;
					return false;
				}
			}
			else
			{
				if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "vad", "false")) != ERROR_ok)
				{
					returnInfo = "Error setting voice activation detection. Error: " + std::to_string(error);
					return false;
				}

				if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "voiceactivation_level", "-50")) != ERROR_ok)
				{
					returnInfo = "Error setting voice activation level. Error: " + std::to_string(error);
					return false;
				}
			}

		returnInfo = VoipModule.VarVoIPPushToTalk->ValueInt ? "Enabled VoIP PushToTalk, disabled voice activation detection" : "Disabled VoIP PushToTalk, enabled voice activation detection.";
		return true;
	}

	bool VariableVolumeModifierUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		unsigned int error;
		uint64 scHandlerID = VoIPGetscHandlerID();
		if (scHandlerID != NULL)
			if ((error = ts3client_setPlaybackConfigValue(scHandlerID, "volume_modifier", VoipModule.VarVoIPVolumeModifier->ValueString.c_str())) != ERROR_ok)
			{
				returnInfo = "Unable to set volume modifier, are you connected to a server? Error: " + std::to_string(error);
				return false;
			}

		returnInfo = "Set VoIP Volume Modifier to " + VoipModule.VarVoIPVolumeModifier->ValueString;
		return true;
	}

	bool VariableAGCUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		unsigned int error;
		uint64 scHandlerID = VoIPGetscHandlerID();
		if (scHandlerID != NULL)
			if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "agc", VoipModule.VarVoIPAGC->ValueInt ? "true" : "false")) != ERROR_ok)
			{
				returnInfo = "Unable to set Automatic Gain Control, are you connected to a server? Error: " + std::to_string(error);
				return false;
			}

		returnInfo = VoipModule.VarVoIPAGC->ValueInt ? "Enabled VoIP automatic gain control" : "Disabled VoIP automatic gain control";
		return true;
	}

	bool VariableEchoCancellationUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		unsigned int error;
		uint64 scHandlerID = VoIPGetscHandlerID();
		if (scHandlerID != NULL)
			if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "echo_canceling", VoipModule.VarVoIPEchoCancellation->ValueInt ? "true" : "false")) != ERROR_ok)
			{
				returnInfo = "Unable to set echo cancellation. Error: " + std::to_string(error);
				return false;
			}

		returnInfo = VoipModule.VarVoIPEchoCancellation->ValueInt ? "Enabled VoIP echo cancellation" : "Disabled VoIP echo cancellation";
		return true;
	}

	bool VariableVADLevelUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		unsigned int error;
		uint64 scHandlerID = VoIPGetscHandlerID();
		if (scHandlerID != NULL)
			if ((error = ts3client_setPreProcessorConfigValue(scHandlerID, "voiceactivation_level", VoipModule.VarVoIPVADLevel->ValueString.c_str())) != ERROR_ok)
			{
				returnInfo = "Error setting voice activation level. Error: " + std::to_string(error);
				return false;
			}

		returnInfo = "Set voice activation level to " + VoipModule.VarVoIPVADLevel->ValueString;
		return true;
	}

	bool VariableServerEnabledUpdate(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		//TODO: Check if host, kill client too. StopTeamspeakClient();
		//TODO: Figure out why this doesn't stop the teamspeak server when setting to 0....
		if (VoipModule.VarVoIPServerEnabled->ValueInt == 0)
			StopTeamspeakServer();

		returnInfo = VoipModule.VarVoIPServerEnabled->ValueInt ? "VoIP Server will start when a new lobby is created" : "Disabled VoIP auto-startup.";
		return true;
	}

	bool isServer = false;
	DWORD __stdcall StartClient(LPVOID)
	{
		if (isServer)
			while (!IsTeamspeakServerRunning())
				Sleep(1000); // wait for server to finish starting

		return StartTeamspeakClient(&VoipModule);
	}

	DWORD __stdcall StartServer(LPVOID)
	{
		auto retVal = StartTeamspeakServer(&VoipModule);
		isServer = false;
		return retVal;
	}

	void CallbackServerStop(void* param)
	{
		StopTeamspeakClient();
		StopTeamspeakServer();
		isServer = false;
	}

	// TODO5: kick players from TS
	/*void CallbackServerPlayerKick(void* param)
	{
		PlayerKickInfo* playerInfo = reinterpret_cast<PlayerKickInfo*>(param);
		IRCModule.UserKick(IRCModule.GenerateIRCNick(playerInfo->Name, playerInfo->UID));
	}*/

	void CallbackClientStart(void* param)
	{
		// todo: a way to specify to the client which IP/port to connect to, instead of reading it from memory
		CreateThread(0, 0, StartClient, 0, 0, 0);
	}

	void CallbackServerStart(void* param)
	{
		isServer = true;

		// Start the Teamspeak VoIP Server since this is the host
		CreateThread(0, 0, StartServer, 0, 0, 0);

		// Join the Teamspeak VoIP Server so the host can talk
		CallbackClientStart(param);
	}

}

namespace Modules
{
	ModuleVoIP::ModuleVoIP() : ModuleBase("VoIP")
	{
		engine->OnEvent("Core", "Server.Start", CallbackServerStart);
		engine->OnEvent("Core", "Server.Stop", CallbackServerStop);
		//engine->OnEvent("Core", "Server.PlayerKick", CallbackServerPlayerKick);
		engine->OnEvent("Core", "Game.Joining", CallbackClientStart);
		engine->OnEvent("Core", "Game.Leave", CallbackServerStop);

		VarVoIPPushToTalk = AddVariableInt("PushToTalk", "voip_ptt", "Requires the user to press a key to talk", eCommandFlagsArchived, 1, VariablePushToTalkUpdate);
		VarVoIPPushToTalk->ValueIntMin = 0;
		VarVoIPPushToTalk->ValueIntMax = 1;

		VarVoIPVolumeModifier = AddVariableInt("VolumeModifier", "voip_vm", "Modify the volume of other speakers (db)."
			"0 = no modification, negative values make the signal quieter,"
			"greater than zero boost the signal louder. High positive values = really bad audio quality."
			, eCommandFlagsArchived, 6, VariableVolumeModifierUpdate);

		VarVoIPVolumeModifier->ValueInt64Min = -15;
		VarVoIPVolumeModifier->ValueInt64Max = 30;

		VarVoIPAGC = AddVariableInt("AGC", "voip_agc", "Automatic gain control keeps volumes level with each other. Less really soft, and less really loud.", eCommandFlagsArchived, 1, VariableAGCUpdate);
		VarVoIPAGC->ValueIntMin = 0;
		VarVoIPAGC->ValueIntMax = 1;

		VarVoIPEchoCancellation = AddVariableInt("EchoCancellation", "voip_ec", "Reduces echo over voice chat", eCommandFlagsArchived, 1, VariableEchoCancellationUpdate);
		VarVoIPEchoCancellation->ValueIntMin = 0;
		VarVoIPEchoCancellation->ValueIntMax = 1;

		VarVoIPVADLevel = AddVariableFloat("VoiceActivationLevel", "voip_vadlevel", "A high voice activation level means you have to speak louder into the microphone"
			"in order to start transmitting. Reasonable values range from -50 to 50. Default is -45."
			, eCommandFlagsArchived, -45.0f, VariableVADLevelUpdate);

		VarVoIPVADLevel->ValueFloatMin = -50.0f;
		VarVoIPVADLevel->ValueFloatMax = 50.0f;

		VarVoIPServerEnabled = AddVariableInt("ServerEnabled", "voip_server", "Enables or disables the VoIP Server.", eCommandFlagsArchived, 1, VariableServerEnabledUpdate);
		VarVoIPServerEnabled->ValueIntMin = 0;
		VarVoIPServerEnabled->ValueIntMax = 1;

		VarVoIPServerPort = AddVariableInt("ServerPort", "voip_port", "The port number to listen for VoIP connections on."
			"Tries listening on ports in the range [ServerPort..ServerPort+10].", static_cast<CommandFlags>(eCommandFlagsArchived | eCommandFlagsReplicated), 11794);
		VarVoIPServerPort->ValueIntMin = 0;
		VarVoIPServerPort->ValueIntMax = 0xFFFF;

		VarVoIPTalk = AddVariableInt("Talk", "voip_Talk", "Enables or disables talking (for push to talk)", eCommandFlagsNone, 0);
		VarVoIPTalk->ValueIntMin = 0;
		VarVoIPTalk->ValueIntMax = 1;
	}
}