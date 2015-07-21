#include "PlayerUid.hpp"
#include "../../ElDorito.hpp"

#include <openssl/sha.h>

namespace
{
	// Player properties packet extension to send player UID
	class UidExtension : public Modules::Patches::PlayerPropertiesExtension<uint64_t>
	{
	protected:
		void BuildData(int playerIndex, uint64_t *out)
		{
			*out = ElDorito::Instance().Modules.PlayerUidPatches.GetUid();
		}

		void ApplyData(int playerIndex, void *session, const uint64_t &data)
		{
			*reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(session)+0x50) = data;
		}

		void Serialize(Blam::BitStream *stream, const uint64_t &data)
		{
			stream->WriteUnsigned(data, 64);
		}

		void Deserialize(Blam::BitStream *stream, uint64_t *out)
		{
			*out = stream->ReadUnsigned<uint64_t>(64);
		}
	};
	uint64_t GetPlayerUidHook(int unused)
	{
		return ElDorito::Instance().Modules.PlayerUidPatches.GetUid();
	}

	Pointer UidValidPtr = Pointer(0x19AB728); // true if the UID is set
	Pointer UidPtr = Pointer(0x19AB730);      // The local player's UID
}

namespace Modules
{
	PatchModulePlayerUid::PatchModulePlayerUid() : ModuleBase("Patches.PlayerUid")
	{
		ElDorito::Instance().Modules.NetworkPatches.PlayerPropertiesExtender.Add(std::make_shared<UidExtension>());
		AddModulePatches({},
		{
			// Override the "get UID" function to pull the UID from preferences
			Hook("GetPlayerUid", 0xA7E005, GetPlayerUidHook, HookType::Call),
		});
	}

	uint64_t PatchModulePlayerUid::GetUid()
	{
		EnsureValidUid();
		return UidPtr.Read<uint64_t>();
	}

	std::string PatchModulePlayerUid::GetFormattedPrivKey()
	{
		EnsureValidUid();
		auto& dorito = ElDorito::Instance();
		return dorito.Utils.RSAReformatKey(true, dorito.Modules.Player.VarPlayerPrivKey->ValueString);
	}

	void PatchModulePlayerUid::EnsureValidUid()
	{
		if (UidValidPtr.Read<bool>())
			return; // UID is already set

		auto& dorito = ElDorito::Instance();
		// Try to pull the UID from preferences
		std::string pubKey = dorito.Modules.Player.VarPlayerPubKey->ValueString;
		uint64_t uid = 0;
		if (pubKey.length() <= 0)
		{
			std::string privKey;
			dorito.Utils.RSAGenerateKeyPair(4096, privKey, pubKey);

			// strip headers/footers from PEM keys so they can be stored in cfg / transmitted over JSON easier
			// TODO: find if we can just store the private key and generate the pub key from it when its needed
			dorito.Utils.ReplaceString(privKey, "\n", "");
			dorito.Utils.ReplaceString(privKey, "-----BEGIN RSA PRIVATE KEY-----", "");
			dorito.Utils.ReplaceString(privKey, "-----END RSA PRIVATE KEY-----", "");
			dorito.Utils.ReplaceString(pubKey, "\n", "");
			dorito.Utils.ReplaceString(pubKey, "-----BEGIN PUBLIC KEY-----", "");
			dorito.Utils.ReplaceString(pubKey, "-----END PUBLIC KEY-----", "");

			commands->SetVariable(dorito.Modules.Player.VarPlayerPrivKey, privKey, std::string());
			commands->SetVariable(dorito.Modules.Player.VarPlayerPubKey, pubKey, std::string());

			dorito.Modules.Console.PrintToConsole("Keypair generation complete!");

			// save the keypair
			commands->Execute("WriteConfig");
		}

		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256_CTX sha256;
		SHA256_Init(&sha256);
		SHA256_Update(&sha256, pubKey.c_str(), pubKey.length());
		SHA256_Final(hash, &sha256);
		memcpy(&uid, hash, sizeof(uint64_t)); // use first 8 bytes of SHA256(pubKey) as UID

		UidPtr.Write<uint64_t>(uid);
		UidValidPtr.Write(true);
	}
}