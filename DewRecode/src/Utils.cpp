#include "Utils.hpp"
#include <string>
#include <algorithm>
#include <vector>
#include <sstream>
#include <functional>
#include <cctype>
#include <codecvt>
#include <iomanip>
#include <fstream>
#include <winhttp.h>
#include <Natupnp.h>

#define MINIUPNP_STATICLIB
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/upnpcommands.h>

#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/pem.h>
#include <openssl/sha.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include "ElDorito.hpp"

static const std::string base64_chars =
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz"
			"0123456789+/";


static inline bool is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string PublicUtils::RSAReformatKey(const std::string& key, bool isPrivateKey)
{
	size_t pos = 0;
	std::string returnKey;
	while (pos < key.length())
	{
		int toCopy = key.length() - pos;
		if (toCopy > 64)
			toCopy = 64;
		returnKey += key.substr(pos, toCopy);
		returnKey += "\n";
		pos += toCopy;
	}
	std::string keyType = (isPrivateKey ? "RSA PRIVATE KEY" : "PUBLIC KEY"); // public keys don't have RSA in the name some reason

	std::stringstream ss;
	ss << "-----BEGIN " << keyType << "-----\n" << returnKey << "-----END " << keyType << "-----\n";
	return ss.str();
}

bool PublicUtils::RSACreateSignature(const std::string& privateKey, void* data, size_t dataSize, std::string& signature)
{
	// privateKey has to be reformatted with -----RSA PRIVATE KEY----- header/footer and newlines after every 64 chars
	// before calling this function

	BIO* privKeyBuff = BIO_new_mem_buf((void*)privateKey.c_str(), privateKey.length());
	if (!privKeyBuff)
		return false;

	RSA* rsa = PEM_read_bio_RSAPrivateKey(privKeyBuff, 0, 0, 0);
	if (!rsa)
		return false;

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha;
	SHA256_Init(&sha);
	SHA256_Update(&sha, data, dataSize);
	SHA256_Final(hash, &sha);

	void* sigret = malloc(RSA_size(rsa));
	unsigned int siglen = 0;
	int retVal = RSA_sign(NID_sha256, (unsigned char*)hash, SHA256_DIGEST_LENGTH, (unsigned char*)sigret, &siglen, rsa);

	RSA_free(rsa);
	BIO_free_all(privKeyBuff);

	if (retVal != 1)
		return false;

	signature = Base64Encode((unsigned char*)sigret, siglen);
	return true;
}

bool PublicUtils::RSAVerifySignature(const std::string& pubKey, const std::string& signature, void* data, size_t dataSize)
{
	BIO* pubKeyBuff = BIO_new_mem_buf((void*)pubKey.c_str(), pubKey.length());
	if (!pubKeyBuff)
		return false;

	RSA* rsa = PEM_read_bio_RSA_PUBKEY(pubKeyBuff, 0, 0, 0);
	if (!rsa)
		return false;

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha;
	SHA256_Init(&sha);
	SHA256_Update(&sha, data, dataSize);
	SHA256_Final(hash, &sha);

	size_t length = 0;
	if (Base64DecodeBinary((char*)signature.c_str(), NULL, &length) != 1 || length == 0)
		return false;

	unsigned char* sigBuf = (unsigned char*)malloc(length);
	if (Base64DecodeBinary((char*)signature.c_str(), sigBuf, &length) != 0)
		return false;

	int retVal = RSA_verify(NID_sha256, (unsigned char*)hash, SHA256_DIGEST_LENGTH, sigBuf, length, rsa);
	free(sigBuf);
	RSA_free(rsa);
	BIO_free_all(pubKeyBuff);

	return retVal == 1;
}

static int genrsa_cb(int p, int n, BN_GENCB* cb)
{
	char c = '*';

	if (p == 0)
		c = '.';
	if (p == 1)
		c = '+';
	if (p == 2)
		c = '*';
	if (p == 3)
		c = '\n';
	BIO_write((BIO*)cb->arg, &c, 1);
	(void)BIO_flush((BIO*)cb->arg);

	return 1;
}

bool PublicUtils::RSAGenerateKeyPair(int numBits, std::string& privKey, std::string& pubKey)
{
	// TODO: add some error checking
	RSA* rsa = RSA_new();
	BIGNUM* bn = BN_new();
	BN_GENCB cb;
	BIO* bio_err = NULL;
	BN_GENCB_set(&cb, genrsa_cb, bio_err);
	BN_set_word(bn, RSA_F4);
	RSA_generate_key_ex(rsa, numBits, bn, &cb);

	BIO* privKeyBuff = BIO_new(BIO_s_mem());
	BIO* pubKeyBuff = BIO_new(BIO_s_mem());
	PEM_write_bio_RSAPrivateKey(privKeyBuff, rsa, 0, 0, 0, 0, 0);
	PEM_write_bio_RSA_PUBKEY(pubKeyBuff, rsa); // RSA_PUBKEY includes some data that RSAPublicKey doesn't have

	char* privKeyData;
	char* pubKeyData;
	auto privKeySize = BIO_get_mem_data(privKeyBuff, &privKeyData);
	auto pubKeySize = BIO_get_mem_data(pubKeyBuff, &pubKeyData);

	privKey = std::string(privKeyData, privKeySize);
	pubKey = std::string(pubKeyData, pubKeySize);

	BIO_free_all(privKeyBuff);
	BIO_free_all(pubKeyBuff);
	BN_free(bn);
	RSA_free(rsa);
	return true;
}

std::string PublicUtils::Base64Encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';

	}

	return ret;
}

std::string PublicUtils::Base64Decode(const std::string& encoded_string)
{
	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_]))
	{
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i == 4)
		{
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

// Calculates the length of a decoded string
size_t calcDecodeLength(const char* b64input)
{
	size_t len = strlen(b64input),
		padding = 0;

	if (b64input[len - 1] == '=' && b64input[len - 2] == '=') //last two chars are =
		padding = 2;
	else if (b64input[len - 1] == '=') //last char is =
		padding = 1;

	return (len * 3) / 4 - padding;
}

// Decodes a base64 encoded string, buffer should be allocated before calling
// If buffer is null it'll just give the decoded data length
int PublicUtils::Base64DecodeBinary(char* b64message, unsigned char* buffer, size_t* length)
{
	int decodeLen = calcDecodeLength(b64message);
	if (!buffer)
	{
		*length = decodeLen;
		return 1;
	}

	BIO* bio = BIO_new_mem_buf(b64message, -1);
	BIO* b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // do not use newlines to flush buffer
	*length = BIO_read(bio, buffer, strlen(b64message));
	if (*length != decodeLen) // length should equal decodeLen, else something went horribly wrong
		return 2;

	BIO_free_all(bio);

	return 0;
}

bool PublicUtils::Hash32(const std::string& str, uint32_t* out)
{
	// Generate a SHA-1 digest and take the first 4 bytes
	unsigned char digest[SHA_DIGEST_LENGTH];
	SHA_CTX context;
	if (!SHA1_Init(&context))
		return false;
	if (!SHA1_Update(&context, str.c_str(), str.length()))
		return false;
	if (!SHA1_Final(digest, &context))
		return false;
	*out = digest[0] << 24 | digest[1] << 16 | digest[2] << 8 | digest[3];
	return true;
}

void PublicUtils::RemoveCharsFromString(std::string& str, char* charsToRemove)
{
	for (unsigned int i = 0; i < strlen(charsToRemove); ++i)
		str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end());
}

void PublicUtils::HexStringToBytes(const std::string& in, void* const data, size_t length)
{
	unsigned char* byteData = reinterpret_cast<unsigned char*>(data);

	std::stringstream hexStringStream; hexStringStream >> std::hex;
	for (size_t strIndex = 0, dataIndex = 0; strIndex < (length * 2); ++dataIndex)
	{
		// Read out and convert the string two characters at a time
		const char tmpStr[3] = { in[strIndex++], in[strIndex++], 0 };

		// Reset and fill the string stream
		hexStringStream.clear();
		hexStringStream.str(tmpStr);

		// Do the conversion
		int tmpValue = 0;
		hexStringStream >> tmpValue;
		byteData[dataIndex] = static_cast<unsigned char>(tmpValue);
	}
}

void PublicUtils::BytesToHexString(void* const data, const size_t dataLength, std::string& dest)
{
	unsigned char* byteData = reinterpret_cast<unsigned char*>(data);
	std::stringstream hexStringStream;

	hexStringStream << std::hex << std::setfill('0');
	for (size_t index = 0; index < dataLength; ++index)
		hexStringStream << std::setw(2) << static_cast<int>(byteData[index]);
	dest = hexStringStream.str();
}

bool PublicUtils::IsNull(void* const data, const size_t dataLength)
{
	for (size_t i = 0; i < dataLength; i++)
		if (((BYTE*)data)[i] != 0)
			return false;

	return true;
}

void PublicUtils::ReplaceCharacters(std::string& str, char find, char replaceWith)
{
	for( unsigned i = 0; i < str.length(); ++i )
		if( str[i] == find )
			str[i] = replaceWith;
}

bool PublicUtils::ReplaceString(std::string& str, const std::string& replace, const std::string& with)
{
	size_t start_pos = str.find(replace);
	bool found = false;
	while (start_pos != std::string::npos)
	{
		str.replace(start_pos, replace.length(), with);
		start_pos += with.length();
		found = true;
		start_pos = str.find(replace, start_pos);
	}
	return found;
}

std::wstring PublicUtils::WidenString(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> utf16conv;
	return utf16conv.from_bytes(s);
}

std::string PublicUtils::ThinString(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> utf8conv;
	return utf8conv.to_bytes(str);
}

std::string PublicUtils::ToLower(const std::string& str)
{
	std::string retValue(str);
	std::transform(retValue.begin(), retValue.end(), retValue.begin(), ::tolower);
	return retValue;
}

std::vector<std::string> PublicUtils::SplitString(const std::string& stringToSplit, char delim)
{
	std::vector<std::string> retValue;
	std::stringstream ss(stringToSplit);
	std::string item;
	while( std::getline(ss, item, delim) )
	{
		retValue.push_back(item);
	}
	return retValue;
}

// trim from start
static inline std::string& ltrim(std::string& s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string& rtrim(std::string& s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string& trim(std::string& s)
{
	return ltrim(rtrim(s));
}

std::string PublicUtils::Trim(const std::string& string, bool fromEnd)
{
	std::string retValue(string);
	if( fromEnd ) // From End
	{
		retValue.erase(std::find_if(retValue.rbegin(), retValue.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), retValue.end());
	}
	else // From Start
	{
		retValue.erase(retValue.begin(), std::find_if(retValue.begin(), retValue.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	}

	return retValue;
}

std::vector<std::string> PublicUtils::Wrap(const std::string& string, size_t lineLength)
{
	std::stringstream sstream;
	sstream << string;

	std::vector<std::string> lines;
	while( sstream.good() ) // Copy all lines into string
	{
		std::string currentLine;
		std::getline(sstream, currentLine);
		lines.push_back(currentLine);
	}

	std::vector<std::string> retValue;

	for( unsigned i = 0; i < lines.size(); ++i )
	{
		std::string currentLine(lines[i]);
		while( currentLine.length() > lineLength )
		{
			int wordEnd = lineLength;
			while( wordEnd > 0 && !std::isspace(currentLine[wordEnd]) )
				--wordEnd;
			if( wordEnd <= 0 )
				wordEnd = lineLength;

			retValue.push_back(currentLine.substr(0, wordEnd));
			currentLine = currentLine.substr(wordEnd, std::string::npos);
		}

		retValue.push_back(currentLine);
	}

	return retValue;
}

HttpRequest PublicUtils::HttpSendRequest(const std::wstring& uri, const std::wstring& method, const std::wstring& userAgent, const std::wstring& username, const std::wstring& password, const std::wstring& headers, void* body, DWORD bodySize)
{
	DWORD dwSize;
	DWORD dwDownloaded;
	DWORD headerSize = 0;
	BOOL  bResults = FALSE;
	HINTERNET hSession;
	HINTERNET hConnect;
	HINTERNET hRequest;

	HttpRequest retVal;

	retVal.ResponseHeader.resize(0);
	retVal.ResponseBody.resize(0);

	URL_COMPONENTSW urlComp = {};
	urlComp.dwStructSize = sizeof(urlComp);

	// Set required component lengths to non-zero so that they are cracked.
	urlComp.dwSchemeLength = static_cast<DWORD>(-1);
	urlComp.dwHostNameLength = static_cast<DWORD>(-1);
	urlComp.dwUrlPathLength = static_cast<DWORD>(-1);

	if (!WinHttpCrackUrl(uri.c_str(), uri.length(), 0, &urlComp))
	{
		retVal.Error = HttpRequestError::InvalidUrl;
		return retVal;
	}

	std::wstring scheme;
	std::wstring hostname;
	std::wstring path;
	scheme.resize(urlComp.dwSchemeLength);
	hostname.resize(urlComp.dwHostNameLength);
	path.resize(urlComp.dwUrlPathLength);
	if (!urlComp.lpszScheme)
		scheme = L"http";
	else
		memcpy((void*)scheme.data(), urlComp.lpszScheme, urlComp.dwSchemeLength * sizeof(wchar_t));

	if (!urlComp.lpszHostName)
		hostname = L"localhost";
	else
		memcpy((void*)hostname.data(), urlComp.lpszHostName, urlComp.dwHostNameLength * sizeof(wchar_t));

	if (!urlComp.lpszUrlPath)
		path = L"/";
	else
		memcpy((void*)path.data(), urlComp.lpszUrlPath, urlComp.dwUrlPathLength * sizeof(wchar_t));

	DWORD dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
	LPCWSTR pszProxyW = WINHTTP_NO_PROXY_NAME;
	LPCWSTR pszProxyBypassW = WINHTTP_NO_PROXY_BYPASS;
	WINHTTP_CURRENT_USER_IE_PROXY_CONFIG iecfg;
	if (WinHttpGetIEProxyConfigForCurrentUser(&iecfg))
	{
		if (iecfg.fAutoDetect)
		{
			hSession = WinHttpOpen(userAgent.c_str(), WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!hSession)
			{
				retVal.Error = HttpRequestError::HttpOpenFailed;
				return retVal;
			}

			WINHTTP_AUTOPROXY_OPTIONS proxy;
			proxy.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
			proxy.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
			proxy.fAutoLogonIfChallenged = true;
			std::wstring fullUrl = scheme + L"://" + hostname + L":" + std::to_wstring(urlComp.nPort);
			WINHTTP_PROXY_INFO info;
			if (WinHttpGetProxyForUrl(hSession, fullUrl.c_str(), &proxy, &info))
			{
				pszProxyW = info.lpszProxy;
				pszProxyBypassW = info.lpszProxyBypass;
				dwAccessType = info.dwAccessType;
			}
		}
		else
		{
			dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
			pszProxyW = iecfg.lpszProxy;
			pszProxyBypassW = iecfg.lpszProxyBypass;
		}
	}

	if (pszProxyW == 0 || wcslen(pszProxyW) <= 0)
	{
		// no proxy server set
		dwAccessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
		pszProxyW = WINHTTP_NO_PROXY_NAME;
		pszProxyBypassW = WINHTTP_NO_PROXY_BYPASS;
	}

	hSession = WinHttpOpen(userAgent.c_str(), dwAccessType, pszProxyW, pszProxyBypassW, 0);
	if (!hSession)
	{
		retVal.Error = HttpRequestError::HttpOpenFailed2;
		return retVal;
	}

	WinHttpSetTimeouts(hSession, 5 * 1000, 5 * 1000, 5 * 1000, 5 * 1000);

	hConnect = WinHttpConnect(hSession, hostname.c_str(), urlComp.nPort, 0);
	if (!hConnect)
	{
		retVal.Error = HttpRequestError::HttpConnectFailed;
		return retVal;
	}

	hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
	if (!hRequest)
	{
		retVal.Error = HttpRequestError::HttpOpenRequestFailed;
		return retVal;
	}

	if (!username.empty() || !password.empty())
		if (!WinHttpSetCredentials(hRequest, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, username.c_str(), password.c_str(), 0))
		{
			retVal.Error = HttpRequestError::HttpSetCredentialsFailed;
			return retVal;
		}

	LPCWSTR addtHdrs = WINHTTP_NO_ADDITIONAL_HEADERS;
	DWORD length = 0;
	if (!headers.empty())
	{
		addtHdrs = headers.c_str();
		length = (DWORD)-1;
	}

	bResults = WinHttpSendRequest(hRequest, addtHdrs, length, body, bodySize, bodySize, 0);

	if (bResults)
		bResults = WinHttpReceiveResponse(hRequest, NULL);
	if (bResults)
	{
		bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, WINHTTP_NO_OUTPUT_BUFFER, &headerSize, WINHTTP_NO_HEADER_INDEX);
		if ((!bResults) && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
		{
			retVal.ResponseHeader.resize(headerSize / sizeof(wchar_t));
			if (retVal.ResponseHeader.empty())
			{
				bResults = TRUE;
			}
			else
			{
				bResults = WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, &retVal.ResponseHeader[0], &headerSize, WINHTTP_NO_HEADER_INDEX);
				if (!bResults)
					headerSize = 0;

				retVal.ResponseHeader.resize(headerSize / sizeof(wchar_t));
			}
		}
	}
	if (bResults)
	{
		do
		{
			// Check for available data.
			dwSize = 0;
			bResults = WinHttpQueryDataAvailable(hRequest, &dwSize);
			if (!bResults)
			{
				std::stringstream ss;
				ss << "Error " << GetLastError() << "in WinHttpQueryDataAvailable.";
				OutputDebugStringA(ss.str().c_str());
				break;
			}

			if (dwSize == 0)
				break;

			do
			{
				// Allocate space for the buffer.
				DWORD dwOffset = retVal.ResponseBody.size();
				retVal.ResponseBody.resize(dwOffset + dwSize);

				// Read the data.
				bResults = WinHttpReadData(hRequest, &retVal.ResponseBody[dwOffset], dwSize, &dwDownloaded);
				if (!bResults)
				{
					std::stringstream ss;
					ss << "Error " << GetLastError() << " in WinHttpReadData.";
					OutputDebugStringA(ss.str().c_str());
					dwDownloaded = 0;
				}

				retVal.ResponseBody.resize(dwOffset + dwDownloaded);

				if (dwDownloaded == 0)
					break;

				dwSize -= dwDownloaded;
			} while (dwSize > 0);
		} while (true);
	}

	// Close any open handles.
	if (hRequest) WinHttpCloseHandle(hRequest);
	if (hConnect) WinHttpCloseHandle(hConnect);
	if (hSession) WinHttpCloseHandle(hSession);

	if (bResults)
	{
		retVal.Error = HttpRequestError::None;
		return retVal;
	}

	retVal.LastError = GetLastError();
	retVal.Error = HttpRequestError::HttpDownloadFailed;
	return retVal;
}

UPnPResult PublicUtils::UPnPForwardPort(bool tcp, int externalport, int internalport, const std::string& ruleName)
{
	struct UPNPUrls urls;
	struct IGDdatas data;
	char lanaddr[16];

	if (upnpDiscoverError != UPNPDISCOVER_SUCCESS || upnpDevice == nullptr)
		return UPnPResult(UPnPErrorType::DiscoveryError, upnpDiscoverError);

	int ret = UPNP_GetValidIGD(upnpDevice, &urls, &data,
		lanaddr, sizeof(lanaddr));

	if (ret != UPNP_IGD_VALID_CONNECTED)
		return UPnPResult(UPnPErrorType::IGDError, ret);

	ret = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype, 
		std::to_string(externalport).c_str(), std::to_string(internalport).c_str(), 
		lanaddr, ruleName.c_str(), tcp ? "TCP" : "UDP", NULL, NULL);

	UPnPErrorType type = UPnPErrorType::None;
	if (ret != UPNPCOMMAND_SUCCESS)
		type = UPnPErrorType::PortMapError;

	return UPnPResult(type, ret);
}

void PublicUtils::GetEndpoints(std::vector<std::string>& destVect, const std::string& endpointType)
{
	std::ifstream in("dewrito.json", std::ios::in | std::ios::binary);
	if (in && in.is_open())
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize((unsigned int)in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();

		rapidjson::Document json;
		if (json.Parse<0>(contents.c_str()).HasParseError() || !json.IsObject())
			return;

		if (!json.HasMember("masterServers"))
			return;

		auto& mastersArray = json["masterServers"];
		for (auto it = mastersArray.Begin(); it != mastersArray.End(); it++)
			if ((*it).HasMember(endpointType.c_str()))
				destVect.push_back((*it)[endpointType.c_str()].GetString());
	}
}

size_t PublicUtils::ExecuteProcess(const std::wstring& fullPathToExe, std::wstring& parameters, size_t secondsToWait)
{
	// taken from https://stackoverflow.com/questions/9970241/how-do-you-run-external-programs-with-parameters-without-the-cmd-window-showing
	size_t iMyCounter = 0, iReturnVal = 0, iPos = 0;
	DWORD dwExitCode = 0;
	std::wstring sTempStr = L"";

	/* - NOTE - You should check here to see if the exe even exists */

	/* Add a space to the beginning of the Parameters */
	if (parameters.size() != 0)
		if (parameters[0] != L' ')
			parameters.insert(0, L" ");

	/* The first parameter needs to be the exe itself */
	sTempStr = fullPathToExe;
	iPos = sTempStr.find_last_of(L"\\");
	sTempStr.erase(0, iPos + 1);
	parameters = sTempStr.append(parameters);

	/* CreateProcessW can modify Parameters thus we allocate needed memory */
	wchar_t * pwszParam = new wchar_t[parameters.size() + 1];
	if (pwszParam == 0)
		return 1;

	const wchar_t* pchrTemp = parameters.c_str();
	wcscpy_s(pwszParam, parameters.size() + 1, pchrTemp);

	/* CreateProcess API initialization */
	STARTUPINFOW siStartupInfo;
	PROCESS_INFORMATION piProcessInfo;
	memset(&siStartupInfo, 0, sizeof(siStartupInfo));
	memset(&piProcessInfo, 0, sizeof(piProcessInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);

	if (CreateProcessW(const_cast<LPCWSTR>(fullPathToExe.c_str()), pwszParam, 0, 0, false, CREATE_DEFAULT_ERROR_MODE, 0, 0, &siStartupInfo, &piProcessInfo) != false)
		dwExitCode = WaitForSingleObject(piProcessInfo.hProcess, (secondsToWait * 1000)); // Watch the process.
	else
		iReturnVal = GetLastError(); // CreateProcess failed

	/* Free memory */
	delete[]pwszParam;
	pwszParam = 0;

	/* Release handles */
	CloseHandle(piProcessInfo.hProcess);
	CloseHandle(piProcessInfo.hThread);

	return iReturnVal;
}

std::string PublicUtils::ColorToHex(float col[3])
{
	std::stringstream stream;
	stream << "#";
	for (int i = 0; i < 3; i++)
	{
		int val = (int)(255.f * col[i]);
		stream << std::setfill('0') << std::setw(2) << std::hex << val;
	}
	return stream.str();
}

PublicUtils::PublicUtils()
{
	WSADATA wsaData;

	WSAStartup(MAKEWORD(2, 0), &wsaData);
	// do the discovery in the constructor (ie. when the game starts)
	upnpDevice = upnpDiscover(2000, NULL, NULL, 0, 0, &upnpDiscoverError);
}

PublicUtils::~PublicUtils()
{
	if (upnpDevice)
		freeUPNPDevlist(upnpDevice);
}