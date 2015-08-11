#pragma once
#include <string>

enum class HttpRequestError
{
	None,
	InvalidUrl,
	HttpOpenFailed,
	HttpOpenFailed2,
	HttpConnectFailed,
	HttpSetCredentialsFailed,
	HttpOpenRequestFailed,
	HttpDownloadFailed,
};

struct HttpRequest
{
	HttpRequestError Error;
	DWORD LastError;
	std::wstring ResponseHeader;
	std::vector<BYTE> ResponseBody;
};

enum class UPnPErrorType
{
	None,
	DiscoveryError,
	IGDError,
	PortMapError
};

enum UPnPDiscoveryErrors
{
	UPNPDISCOVER_SUCCESS = 0,
	UPNPDISCOVER_UNKNOWN_ERROR = -1,
	UPNPDISCOVER_SOCKET_ERROR = -101,
	UPNPDISCOVER_MEMORY_ERROR = -102
};

enum UPnPIGDErrors
{
	UPNP_IGD_NONE = 0,
	UPNP_IGD_VALID_CONNECTED = 1,
	UPNP_IGD_VALID_NOT_CONNECTED = 2,
	UPNP_IGD_INVALID = 3
};

enum UPnPPortMapErrors
{
	UPNPCOMMAND_SUCCESS = 0,
	UPNPCOMMAND_UNKNOWN_ERROR = -1,
	UPNPCOMMAND_INVALID_ARGS = -2,
	UPNPCOMMAND_HTTP_ERROR = -3,
	UPNPCOMMAND_INVALID_RESPONSE = -4,
};

struct UPnPResult
{
	UPnPErrorType ErrorType;
	int ErrorCode;

	UPnPResult(UPnPErrorType type, int code)
	{
		ErrorType = type;
		ErrorCode = code;
	}
};

/*
if you want to make changes to this interface create a new IUtils002 class and make them there, then edit Utils class to inherit from the new class + this older one
for backwards compatibility (with plugins compiled against an older ED SDK) we can't remove any methods, only add new ones to a new interface version
*/

class IUtils001
{
public:
	virtual std::string RSAReformatKey(const std::string& key, bool isPrivateKey) = 0;
	virtual bool RSACreateSignature(const std::string& privateKey, void* data, size_t dataSize, std::string& signature) = 0;
	virtual bool RSAVerifySignature(const std::string& pubKey, const std::string& signature, void* data, size_t dataSize) = 0;
	virtual bool RSAGenerateKeyPair(int numBits, std::string& privKey, std::string& pubKey) = 0;

	virtual std::string Base64Encode(unsigned char const* bytes_to_encode, unsigned int in_len) = 0;
	virtual std::string Base64Decode(const std::string& encoded_string) = 0;
	virtual int Base64DecodeBinary(char* b64message, unsigned char* buffer, size_t* length) = 0;

	virtual void RemoveCharsFromString(std::string& str, char* charsToRemove) = 0;

	virtual void HexStringToBytes(const std::string& in, void *const data, size_t length) = 0;
	virtual void BytesToHexString(void *const data, const size_t dataLength, std::string& dest) = 0;

	virtual std::string ToLower(const std::string& str) = 0;

	virtual void ReplaceCharacters(std::string& str, char replace, char with) = 0;
	virtual bool ReplaceString(std::string& str, const std::string& replace, const std::string& with) = 0;

	virtual std::wstring WidenString(const std::string& str) = 0;
	virtual std::string ThinString(const std::wstring& str) = 0;

	virtual std::vector<std::string> SplitString(const std::string& stringToSplit, char delim = ' ') = 0;
	virtual std::string Trim(const std::string& string, bool fromEnd = true) = 0;
	virtual std::vector<std::string> Wrap(const std::string& string, size_t lineLength) = 0;

	virtual HttpRequest HttpSendRequest(const std::wstring& uri, const std::wstring& method, const std::wstring& userAgent, const std::wstring& username, const std::wstring& password, const std::wstring& headers, void* body, DWORD bodySize) = 0;
	virtual UPnPResult UPnPForwardPort(bool tcp, int externalport, int internalport, const std::string& ruleName) = 0;
};

#define UTILS_INTERFACE_VERSION001 "Utils001"

/* use this class if you're updating IUtils after we've released a build
also update the IUtils typedef and UTILS_INTERFACE_LATEST define
and edit Engine::CreateInterface to include this interface */

/*class IUtils002 : public IUtils001
{

};

#define UTILS_INTERFACE_VERSION002 "Utils002"*/

typedef IUtils001 IUtils;
#define UTILS_INTERFACE_LATEST UTILS_INTERFACE_VERSION001
