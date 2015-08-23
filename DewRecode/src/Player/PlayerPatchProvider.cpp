#include "PlayerPatchProvider.hpp"
#include "../ElDorito.hpp"
#include "../Network/PlayerPropertiesExtension.hpp"
#include "PlayerCommandProvider.hpp"

#include <openssl/sha.h>

Player::PlayerPatchProvider* playerPatches;

namespace
{
	uint64_t GetPlayerUidHook(int unused);

	Pointer UidValidPtr = Pointer(0x19AB728); // true if the UID is set
	Pointer UidPtr = Pointer(0x19AB730);      // The local player's UID
}

namespace Player
{
	PlayerPatchProvider::PlayerPatchProvider()
	{
		playerPatches = this;
	}

	PatchSet PlayerPatchProvider::GetPatches()
	{
		auto usernamePtr = Utils::Misc::ConvertToVector<uint32_t>((uint32_t)&ElDorito::Instance().PlayerCommands->UserName);

		PatchSet patches("PlayerPatches", 
		{
			// patch Game_GetPlayerName to get the name from our field
			Patch("GetPlayerName", 0x442AA1, usernamePtr),
			// patch BLF save func to get the name from our field
			Patch("BLFGetPlayerName", 0x524E6A, usernamePtr),
		},
		{
			// Override the "get UID" function to pull the UID from preferences
			Hook("GetPlayerUid", 0xA7E005, GetPlayerUidHook, HookType::Call),
		});

		return patches;
	}

	uint64_t PlayerPatchProvider::GetUid()
	{
		EnsureValidUid();
		return UidPtr.Read<uint64_t>();
	}

	void PlayerPatchProvider::EnsureValidUid()
	{
		if (UidValidPtr.Read<bool>())
			return; // UID is already set

		auto& dorito = ElDorito::Instance();
		// Try to pull the UID from preferences
		std::string pubKey = dorito.PlayerCommands->VarPubKey->ValueString;
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

			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarPrivKey, privKey, std::string());
			dorito.CommandManager.SetVariable(dorito.PlayerCommands->VarPubKey, pubKey, std::string());

			dorito.Engine.PrintToConsole("Keypair generation complete!");

			// save the keypair
			dorito.CommandManager.Execute("WriteConfig");
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

namespace
{
	// Player properties packet extension to send player UID
	class UidExtension : public Network::PlayerPropertiesExtension<uint64_t>
	{
	protected:
		void BuildData(int playerIndex, uint64_t* out)
		{
			*out = playerPatches->GetUid();
		}

		void ApplyData(int playerIndex, void* session, const uint64_t& data)
		{
			*reinterpret_cast<uint64_t*>(static_cast<uint8_t*>(session)+0x50) = data;
		}

		void Serialize(Blam::BitStream* stream, const uint64_t& data)
		{
			stream->WriteUnsigned(data, 64);
		}

		void Deserialize(Blam::BitStream* stream, uint64_t* out)
		{
			*out = stream->ReadUnsigned<uint64_t>(64);
		}
	};

	uint64_t GetPlayerUidHook(int unused)
	{
		return playerPatches->GetUid();
	}
}