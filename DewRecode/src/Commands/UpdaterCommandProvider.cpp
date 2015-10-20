#include "UpdaterCommandProvider.hpp"
#include "../ElDorito.hpp"

#include <rapidjson/document.h>

#include <fstream>

namespace Updater
{
	const std::string updatePublicKey = "MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA+F9s5IpIvH/lpFgcvqACianoqFGAOo0nPJTjfpFdhbYQinW5L5RLSJm8l9Q1qvyUTLoIWMLvz3Kd4eDfquoDFywj/N8bCQXYgitroLCBIiuZ8BwRlhTAYt0MNW5/QHr8RK90HbmSOWF7WoWFndHk+9bdTpdeLpxB7A1vpER7qKVWDU8zziYOXn+/87WU8Z5DvPQpJr1z9W5CBsAjL8ATkMaHiSbqbUituNt1lz2dXl33FUz6EBYyOzPDLrpY6a7sZu7FjgWG9ie4WIOt8XcTSO9Jn1AnGvpCaDyO2HRlMbioSDSR+U1nqk3nkqjWI/4H8DlEQxjbBL7VtmFiFdfkm2Ae3bg0QJKOlsmXg4jX+l3fWKoC5eZoJpexK4fUQmnmkGftw2ZHbUWwUYU8E3XBp97YaXsIKQjcsBrZROLe1E2EPKpRO8RlHdYwwFwSvW1Yv3Ua98Bz8DDyiBzpdXx9sgFaYMzeuNt2uGV2/pCbhugdJ0Di62QBfRKXty1GEFtmPK8+Jrv1OAzVaBXJ7U4AON104BA1pXVfh0QWTfyfB6fpHoVmSQzI7hYdyY9x10JZJzoi/4Y6Bj4lOj4IF7BTxcToZCUopRdFsvbj1otEeya60K5LEuL40uhZK0F3tPi6nzH+NMzVZGRQCz2zgjf5oFbs7xGYfxw5+p3toUcT5csCAwEAAQ==";

	void UpdaterCommandProvider::RegisterVariables(ICommandManager* manager)
	{
		VarBranch = manager->Add(Command::CreateVariableString("Updater", "Branch", "update_branch", "The branch to use for updates.", eCommandFlagsArchived, "stable"));
		VarCheckOnLaunch = manager->Add(Command::CreateVariableInt("Updater", "CheckOnLaunch", "autoupdate", "Check for updates on game launch.", eCommandFlagsArchived, 1));
		VarCheckOnLaunch->ValueIntMin = 0;
		VarCheckOnLaunch->ValueIntMax = 1;

		manager->Add(Command::CreateCommand("Updater", "Check", "update_check", "Checks the update servers for updates.", eCommandFlagsNone, BIND_COMMAND(this, &UpdaterCommandProvider::CommandCheck)));
		manager->Add(Command::CreateCommand("Updater", "SignManifest", "update_sign", "Signs an update manifest with the given private key.", eCommandFlagsNone, BIND_COMMAND(this, &UpdaterCommandProvider::CommandSignManifest), { "[filePath] The file to sign", "[keyPath] The private key to sign with", "[writeToSigFile:bool] If true, writes signature to filepath.sig" }));
	}

	void UpdaterCommandProvider::RegisterCallbacks(IEngine* engine)
	{
		engine->OnEvent("Core", "Engine.MainMenuShown", BIND_CALLBACK(this, &UpdaterCommandProvider::CheckForUpdatesCallback));
	}

	bool UpdaterCommandProvider::CommandSignManifest(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		if (Arguments.size() < 2)
		{
			context.WriteOutput("Usage: Updater.SignManifest [manifestPath] [keyPath] [writeToSigFile]");
			return false;
		}

		auto manifestPath = Arguments[0];
		auto keyPath = Arguments[1];
		bool writeToSigFile = false;
		if (Arguments.size() > 2)
			writeToSigFile = Arguments[2] == "true" || Arguments[2] == "1";

		std::ifstream in(manifestPath, std::ios::in | std::ios::binary);
		if (!in || !in.is_open())
		{
			context.WriteOutput("Unable to open manifest file.");
			return false;
		}
		in.close();

		std::ifstream in2(keyPath, std::ios::in | std::ios::binary);
		if (!in2 || !in2.is_open())
		{
			context.WriteOutput("Unable to open key file.");
			return false;
		}

		std::string contents;
		in2.seekg(0, std::ios::end);
		contents.resize((unsigned int)in2.tellg());
		in2.seekg(0, std::ios::beg);
		in2.read(&contents[0], contents.size());
		in2.close();

		auto formattedKey = ElDorito::Instance().Utils.RSAReformatKey(contents, true);
		auto signature = SignManifest(manifestPath, formattedKey);
		if (signature.empty())
		{
			context.WriteOutput("Manifest signature creation failed.");
			return false;
		}
		if (!writeToSigFile)
		{
			context.WriteOutput(signature);
			return true;
		}

		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		ZeroMemory(fname, _MAX_FNAME);
		ZeroMemory(ext, _MAX_EXT);

		_splitpath_s(manifestPath.c_str(), 0, 0, 0, 0, fname, _MAX_FNAME, ext, _MAX_EXT);

		std::string manifestFileName = std::string(fname);
		if (strlen(ext) > 0)
			manifestFileName = manifestFileName + std::string(ext);

		std::ofstream out(manifestPath + ".sig", std::ios::out | std::ios::binary);
		if (!out || !out.is_open())
		{
			context.WriteOutput("Unable to open .sig file for writing.");
			return false;
		}

		out << "{\"" << manifestFileName << "\": \"" << signature << "\"}";
		out.close();
		context.WriteOutput("Wrote manifest signature to " + manifestPath + ".sig");
		return true;
	}

	std::string UpdaterCommandProvider::SignManifest(const std::string& manifestPath, const std::string& privateKey)
	{
		std::ifstream in2(manifestPath, std::ios::in | std::ios::binary);
		if (!in2 || !in2.is_open())
			return "";

		std::string contents;
		in2.seekg(0, std::ios::end);
		contents.resize((unsigned int)in2.tellg());
		in2.seekg(0, std::ios::beg);
		in2.read(&contents[0], contents.size());
		in2.close();

		std::string signature;
		if (!ElDorito::Instance().Utils.RSACreateSignature(privateKey, (void*)contents.c_str(), contents.length(), signature))
			return "";

		return signature;
	}

	bool UpdaterCommandProvider::CommandCheck(const std::vector<std::string>& Arguments, ICommandContext& context)
	{
		std::string branch = VarBranch->ValueString;
		if (Arguments.size() > 0)
			branch = Arguments[0];
		
		context.WriteOutput(CheckForUpdates(branch));
		return true;
	}

	void CallbackUpdate(const std::string& manifest, const std::string& branch)
	{
		//..	if (result.compare("Yes"))
		//		return;

		// launch update helper, pointing to the json file

		auto& dorito = ElDorito::Instance();
		auto params = dorito.Utils.WidenString("-manifest \"" + manifest + "\" -branch \"" + branch + "\"");
		dorito.Utils.ExecuteProcess(L"DewritoUpdateHelper.exe", params, 0);
		std::exit(0);
	}

	void UpdaterCommandProvider::CheckForUpdatesCallback(void* param)
	{
		if (VarCheckOnLaunch->ValueInt)
			ElDorito::Instance().Logger.Log(LogSeverity::Debug, "UpdateCheck", CheckForUpdates(VarBranch->ValueString));
	}

	std::string UpdaterCommandProvider::CheckForUpdates(const std::string& branch)
	{
		std::stringstream ss;
		std::vector<std::string> announceEndpoints;

		auto& dorito = ElDorito::Instance();

		dorito.Utils.GetEndpoints(announceEndpoints, "update");
		if (announceEndpoints.size() <= 0)
			return "No update endpoints found.";

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
				ss << "Invalid update channels response from " << server << std::endl << std::endl;
				continue;
			}

			auto respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid update channels response from " << server << std::endl << std::endl;
				continue;
			}

			// parse the json response
			std::string resp = std::string(req.ResponseBody.begin(), req.ResponseBody.end());
			rapidjson::Document json;
			if (json.Parse<0>(resp.c_str()).HasParseError() || !json.IsObject())
			{
				ss << "Invalid update channels response from " << server << std::endl << std::endl;
				continue;
			}

			if (!json.HasMember("Channels"))
			{
				ss << "Update manifest channels from " << server << " is missing data." << std::endl << std::endl;
				continue;
			}

			auto& result = json["Channels"];
			if (!result.HasMember(branch.c_str()))
			{
				ss << "Update manifest channels from " << server << " is missing branch \"" << branch << "\"" << std::endl << std::endl;
				continue;
			}

			auto& branchInfo = result[branch.c_str()];
			if (!branchInfo.HasMember("Version") || !branchInfo.HasMember("ReleaseNo"))
			{
				ss << "Update manifest channels from " << server << " is missing data for branch \"" << branch << "\"" << std::endl << std::endl;
				continue;
			}

			auto branchVersion = branchInfo["Version"].GetString();
			auto branchReleaseNo = branchInfo["ReleaseNo"].GetInt();

			// read in current version info from version.json
			std::string currentBranch = "";
			int currentReleaseNo = 0;

			std::ifstream in("version.json", std::ios::in | std::ios::binary);
			if (in && in.is_open())
			{
				std::string contents;
				in.seekg(0, std::ios::end);
				contents.resize((unsigned int)in.tellg());
				in.seekg(0, std::ios::beg);
				in.read(&contents[0], contents.size());
				in.close();

				rapidjson::Document json;
				if (!json.Parse<0>(contents.c_str()).HasParseError() && json.IsObject())
				{
					if (json.HasMember("Branch"))
						currentBranch = json["Branch"].GetString();
					if (json.HasMember("ReleaseNo"))
						currentReleaseNo = json["ReleaseNo"].GetInt();
				}
			}

			if (currentBranch == branch && currentReleaseNo >= branchReleaseNo)
			{
				ss << "Update manifest from master server " << server << " [" << branch << "] is current (" << branchVersion << "-" << branchReleaseNo << ")" << std::endl << std::endl;
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
				ss << "Exception during update manifest request to " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Invalid update manifest response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Invalid update manifest response from " << server << " [" << branch << "]" << std::endl << std::endl;
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
				ss << "Exception during update manifest sig request to " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			// make sure the server replied with 200 OK
			if (req.ResponseHeader.length() < expected.length())
			{
				ss << "Update manifest sig response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			respHdr = req.ResponseHeader.substr(0, expected.length());
			if (respHdr.compare(expected))
			{
				ss << "Update manifest sig response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			std::string manifestSig = std::string(req.ResponseBody.begin(), req.ResponseBody.end());
			rapidjson::Document sigJson;
			if (sigJson.Parse<0>(manifestSig.c_str()).HasParseError() || !sigJson.IsObject())
			{
				ss << "Update manifest sig JSON response from " << server << " [" << branch << "]" << std::endl << std::endl;
				continue;
			}

			if (!sigJson.HasMember("manifest.json"))
			{
				ss << "Update manifest sig JSON response from " << server << " [" << branch << "]" " is missing data." << std::endl << std::endl;
				continue;
			}

			std::string manifestSigB64 = sigJson["manifest.json"].GetString();

			auto sigValid = dorito.Utils.RSAVerifySignature(dorito.Utils.RSAReformatKey(updatePublicKey, false), manifestSigB64, (void*)manifest.c_str(), manifest.length());
			if (!sigValid)
			{
				ss << "Update manifest from server " << server << " [" << branch << "]" " has invalid signature." << std::endl << std::endl;
				continue;
			}

			// manifest is validated & update is needed, prompt user?
			//dorito.Engine.ShowMessageBox("Update available", "An update for ElDewrito is available, do you want to update now?\nCurrent version: " + currentVersion + ", Newer version: " + branchVersion, manifestJsonPath, { "Yes", "No" }, CallbackUpdate);

			CallbackUpdate(manifestJsonPath, branch);
		}

		return ss.str();
	}
}