#include "UpdaterCommandProvider.hpp"
#include "../ElDorito.hpp"

#include <rapidjson/document.h>

namespace Updater
{
	void UpdaterCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarUpdaterBranch = manager->Add(Command::CreateVariableString("Updater", "Branch", "update_branch", "The branch to use for updates.", eCommandFlagsArchived, "main"));

		manager->Add(Command::CreateCommand("Updater", "Check", "update_check", "Checks the update servers for updates.", eCommandFlagsNone, BIND_COMMAND(this, &UpdaterCommandProvider::CommandCheck)));
	}

	bool UpdaterCommandProvider::CommandCheck(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		std::string branch = VarUpdaterBranch->ValueString;
		if (Arguments.size() > 0)
			branch = Arguments[0];
		
		context.WriteOutput(CheckForUpdates(branch));
		return true;
	}


	void CallbackUpdate(const std::string& boxTag, const std::string& result)
	{
		if (result.compare("Yes"))
			return;

		// launch update helper, pointing to the json file

		auto& dorito = ElDorito::Instance();

		dorito.Utils.ExecuteProcess(L"eldewrito_update_helper", dorito.Utils.WidenString("-manifest " + boxTag), 0);
		std::exit(0);
	}

	const std::string updateMasterKey = "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";

	std::string UpdaterCommandProvider::CheckForUpdates(const std::string& branch)
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;

		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(announceEndpoints, "update");

		std::string currentVer = dorito.Engine.GetDoritoVersionString();

		for (auto server : announceEndpoints)
		{
			HttpRequest req;
			try
			{
				req = dorito.Utils.HttpSendRequest(dorito.Utils.WidenString(server + "/channels.json"), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(currentVer), L"", L"", L"", NULL, 0);
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server " << server << " (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...) // TODO: find out what exception is being caused
			{
				ss << "Exception during master server update request to " << server << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			std::wstring expected = L"HTTP/1.1 200 OK";
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server update response from " << server << std::endl << std::endl;
				continue;
			}

			auto respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server update response from " << server << std::endl << std::endl;
				continue;
			}

			// parse the json response
			std::string resp = std::string(req.ResponseBody.begin(), req.ResponseBody.end());
			rapidjson::Document json;
			if (json.Parse<0>(resp.c_str()).HasParseError() || !json.IsObject())
			{
				ss << "Invalid master server JSON response from " << server << std::endl << std::endl;
				continue;
			}

			if (!json.HasMember("channels"))
			{
				ss << "Master server JSON response from " << server << " is missing data." << std::endl << std::endl;
				continue;
			}

			auto& result = json["channels"];
			if (!result.HasMember(branch.c_str()))
			{
				ss << "Master server JSON response from " << server << " is missing branch \"" << branch << "\"" << std::endl << std::endl;
				continue;
			}

			std::string branchVer = result[branch.c_str()].GetString();
			if (!branchVer.compare(currentVer))
			{
				ss << "Master server " << server << " [" << branch << "] is current." << std::endl << std::endl;
				continue;
			}

			// now get the update manifest & verify it
			std::string manifestJsonPath = server + "/" + branch + "/manifest.json";
			try
			{
				req = dorito.Utils.HttpSendRequest(dorito.Utils.WidenString(manifestJsonPath), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(currentVer), L"", L"", L"", NULL, 0);
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server " << server << " [" << branch << "] (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...) // TODO: find out what exception is being caused
			{
				ss << "Exception during master server update request to " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server update response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server update response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			// verify the response
			std::string manifest = std::string(req.ResponseBody.begin(), req.ResponseBody.end());

			try
			{
				req = dorito.Utils.HttpSendRequest(dorito.Utils.WidenString(server + "/" + branch + "/manifest.json.sig"), L"GET", L"ElDewrito/" + dorito.Utils.WidenString(currentVer), L"", L"", L"", NULL, 0);
				if (req.Error != HttpRequestError::None)
				{
					ss << "Unable to connect to master server sig " << server << " [" << branch << "] (error: " << (int)req.Error << "/" << req.LastError << "/" << std::to_string(GetLastError()) << ")" << std::endl << std::endl;
					continue;
				}
			}
			catch (...) // TODO: find out what exception is being caused
			{
				ss << "Exception during master server update sig request to " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid master server sig update response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid master server sig update response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			std::string manifestSig = std::string(req.ResponseBody.begin(), req.ResponseBody.end());
			rapidjson::Document sigJson;
			if (sigJson.Parse<0>(manifestSig.c_str()).HasParseError() || !sigJson.IsObject())
			{
				ss << "Invalid master server sig JSON response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			if (!sigJson.HasMember("manifest.json"))
			{
				ss << "Master server sig JSON response from " << server << " [" << branch << "]" " is missing data." << std::endl << std::endl;
				continue;
			}

			std::string manifestSigB64 = sigJson["manifest.json"].GetString();

			auto sigValid = dorito.Utils.RSAVerifySignature(updateMasterKey, manifestSigB64, (void*)manifest.c_str(), manifest.length());
			if (!sigValid)
			{
				ss << "Master server " << server << " [" << branch << "]" " has invalid signature." << std::endl << std::endl;
				continue;
			}

			// manifest is validated & update is needed, prompt user?
			dorito.Engine.ShowMessageBox("Update available", "An update for ElDewrito is available, do you want to update now?\nCurrent version: " + currentVer + ", Newer version: " + branchVer, manifestJsonPath, { "Yes", "No" }, CallbackUpdate);
		}

		return ss.str();
	}
}