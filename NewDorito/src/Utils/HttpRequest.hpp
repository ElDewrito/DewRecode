#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

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

class HttpRequest
{
private:
	std::wstring _userAgent;
public:
	std::wstring ResponseHeader;
	std::vector<BYTE> ResponseBody;
	DWORD LastError;

	HttpRequest(const std::wstring &userAgent);
	HttpRequestError SendRequest(const std::wstring &uri, const std::wstring &method, const std::wstring &username, const std::wstring &password, const std::wstring &headers, void *body, DWORD bodySize);
};