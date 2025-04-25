#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>  
#include <chrono>
#include <ctime>
#include <iomanip>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <json/json.h>
#include "SCommTouchLib.h"

#pragma comment(lib, "winhttp.lib")

// use for Commtouch dll
CSCommTouchLib m_ctLib;
bool m_bInitDll = false;
std::unordered_map<int, std::string> categoryMap = {
    {0, "Unknown"},
    {1, "Advertising & Pop-ups"},
    {2, "Alcohol and Tobacco"},
    {3, "Anonymizers"},
    {4, "Arts"},
    {5, "Business"},
    {6, "Transportation"},
    {7, "Chat"},
    {9, "Forums and Newsgroups"},
    {10, "Compromised Sites"},
    {11, "IT and Technology"},
    {12, "Criminal Activities"},
    {13, "Dating and Personal Ads"},
    {14, "Download Sites"},
    {15, "Education"},
    {16, "Entertainment"},
    {17, "Finance"},
    {18, "Gambling"},
    {19, "Games"},
    {20, "Government"},
    {21, "Hate and Intolerance"},
    {22, "Health and Medicine"},
    {23, "Drugs"},
    {24, "Job Search"},
    {26, "Downloading & Streaming"},
    {27, "News"},
    {28, "Non-profit Organizations and NGOs"},
    {29, "Nudity"},
    {30, "Personal Sites"},
    {31, "Phishing and Fraud"},
    {32, "Politics"},
    {33, "Pornography"},
    {34, "Real Estate"},
    {35, "Religion"},
    {36, "Restaurants"},
    {37, "Search Engines and Portals"},
    {38, "E-commerce"},
    {39, "Social Networks"},
    {40, "Spam"},
    {41, "Sports"},
    {42, "Malware"},
    {44, "Online Translation"},
    {45, "Travel"},
    {46, "Violence"},
    {47, "Weapons"},
    {48, "Webmail"},
    {49, "General"},
    {50, "Leisure and Relaxation"},
    {61, "Botnets"},
    {62, "Cults"},
    {63, "Fashion and Beauty"},
    {64, "Greeting Cards"},
    {65, "Hacking"},
    {67, "Illegal Software"},
    {68, "Image Sharing"},
    {69, "Computer Security"},
    {70, "Instant Messaging"},
    {71, "Network Errors"},
    {72, "Parked Domains"},
    {73, "Peer to Peer"},
    {74, "Private IP Addresses"},
    {75, "Exam Cheating"},
    {76, "Sex Education"},
    {77, "Tasteless Sites"},
    {78, "Abused Children Images"},
    {1000, "Technical Category"},
    {1001, "Child Allowed Category"},
    {1002, "Child Forbidden Category"},
    {1003, "Categorization Unavailable"}
};

static inline void ltrim(std::string& s) {
    s.erase(s.begin(),
        std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
            }));
}

static inline void rtrim(std::string& s) {
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
            }).base(),
                s.end());
}

static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// RAII wrapper for HINTERNET
struct WinHttpHandle {
    HINTERNET handle;
    WinHttpHandle(HINTERNET h = nullptr) : handle(h) {}
    ~WinHttpHandle() {
        if (handle) WinHttpCloseHandle(handle);
    }
    // Không cho copy
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
    // Cho phép move
    WinHttpHandle(WinHttpHandle&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }
    WinHttpHandle& operator=(WinHttpHandle&& other) noexcept {
        if (this != &other) {
            if (handle) WinHttpCloseHandle(handle);
            handle = other.handle;
            other.handle = nullptr;
        }
        return *this;
    }
    explicit operator bool() const { return handle != nullptr; }
};

std::string getCategoryCyren(int id) {
    auto it = categoryMap.find(id);
    if (it != categoryMap.end()) {
        return it->second;
    }
    else {
        return "Unknown Category";
    }
}

// BitDefender API Configuration
std::wstring BITDEFENDER_HOST = L"";
std::wstring BITDEFENDER_ENDPOINT = L"";
std::wstring BITDEFENDER_AUTH_TOKEN = L"";

// F-Secure API Configuration
std::wstring FSECURE_HOST_URL = L"";
std::wstring FSECURE_HOST_ENDPOINT_START = L"";
std::wstring FSECURE_HOST_ENDPOINT_END = L"";
std::wstring FSECURE_PPGID = L"";
std::wstring FSECURE_TRANSACTION = L"";

// F-Secure API Get Token
std::wstring FSECURE_USERNAME = L"";
std::wstring FSECURE_PASSWORD = L"";
std::wstring FSECURE_TOKEN_URL = L"";
std::wstring FSECURE_TOKEN_ENDPOINT = L"";
std::wstring FSECURE_TOKEN_BODY = L"";
std::wstring FSECURE_TOKEN = L"";
std::wstring FSECURE_TOKEN_HEADER = L"";

// For http request
HINTERNET hSession;

std::wstring trim(const std::wstring& str) {
    const std::wstring whitespace = L" \t\n\r";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::wstring::npos) return L"";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

std::map<std::wstring, std::map<std::wstring, std::wstring>> parseIniFile(const std::wstring& filename) {
    std::wifstream file(filename);
    file.imbue(std::locale(std::locale(), new std::codecvt_utf8<wchar_t>));

    std::map<std::wstring, std::map<std::wstring, std::wstring>> iniData;
    std::wstring section, line;

    while (std::getline(file, line)) {
        // Remove BOM if it's at the beginning
        if (!line.empty() && line[0] == 0xFEFF) {
            line.erase(0, 1);
        }

        line = trim(line);
        if (line.empty() || line[0] == L';') continue;

        // Section
        if (line.front() == L'[' && line.back() == L']') {
            section = trim(line.substr(1, line.size() - 2));
        }
        // Key-Value pair
        else {
            size_t pos = line.find(L'=');
            if (pos != std::wstring::npos) {
                std::wstring key = trim(line.substr(0, pos));
                std::wstring value = trim(line.substr(pos + 1));

                // Remove surrounding quotes from value if present
                if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"') {
                    value = value.substr(1, value.size() - 2);
                }

                iniData[section][key] = value;
            }
        }
    }
    return iniData;
}

std::string wstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::wstring stringToWstring(const std::string& str) {
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size_needed <= 0) return L""; 

    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size_needed);

    if (size_needed > 0) {
        wstr.resize(size_needed - 1); 
    }
    else {
        wstr.clear();  
    }
    return wstr;
}

std::string getTimestampedFilename(const std::string& prefix, const std::string& extension) {
    auto now = std::chrono::system_clock::now();
    std::time_t timeNow = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &timeNow);  // Windows
#else
    localtime_r(&timeNow, &localTime);  // Linux/macOS
#endif
    std::ostringstream oss;
    oss << prefix << std::put_time(&localTime, "_%Y-%m-%d_%H-%M-%S") << extension;
    return oss.str();
}
static const std::wstring base64_chars =
L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
L"abcdefghijklmnopqrstuvwxyz"
L"0123456789+/";

std::wstring base64EncodeW(const std::wstring& input) {
    std::wstring result;
    int val = 0, valb = -6;
    for (wchar_t wc : input) {
        unsigned char c = static_cast<unsigned char>(wc);  // chỉ giữ byte thấp
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back(L'=');
    return result;
}

std::wstring createAuthHeaderW(const std::wstring& username, const std::wstring& password) {
    std::wstring combined = username + L":" + password;
    std::wstring encoded = base64EncodeW(combined);
    return L"Authorization: Basic " + encoded + L"\r\n";
}

std::string sendHttpRequest(const std::wstring& host,
    const std::wstring& endpoint,
    const std::wstring& method,
    const std::wstring& headers,
    const std::string& body = "")
{
    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) return "";

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        method.c_str(),
        endpoint.c_str(),
        NULL, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);
    if (!hRequest) return "";

    if (!WinHttpSendRequest(hRequest, headers.c_str(), -1,
        (LPVOID)body.c_str(), (DWORD)body.length(),
        (DWORD)body.length(), 0)) {
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, NULL)) {
        return "";
    }

    DWORD size = 0;
    WinHttpQueryDataAvailable(hRequest, &size);
    std::string response;
    if (size > 0)
    {
        std::vector<char> buffer(size + 1);
        DWORD bytesRead = 0;
        WinHttpReadData(hRequest, buffer.data(), size, &bytesRead);
        buffer[bytesRead] = '\0';

        response = buffer.data();
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);    

    return response;
}

Json::Value parseJsonResponse(const std::string& response) {
    Json::Value jsonData;
    Json::CharReaderBuilder readerBuilder;
    std::unique_ptr<Json::CharReader> reader(readerBuilder.newCharReader());

    std::string errors;
    bool parsingSuccessful = reader->parse(response.c_str(), response.c_str() + response.size(), &jsonData, &errors);

    if (!parsingSuccessful) {
        std::cerr << "JSON parsing error: " << errors << std::endl;
        return Json::Value();
    }

    return jsonData;
}

//Send request to BitDefender API
std::vector<std::string> getBitDefenderCategories(const std::string& url) {
    std::string body = "{\"url\": \"" + url + "\"}";
    std::wstring headers = L"Content-Type: application/json\r\n"
        L"Auth-Token: " + BITDEFENDER_AUTH_TOKEN;

    std::string response = sendHttpRequest(BITDEFENDER_HOST, BITDEFENDER_ENDPOINT, L"POST", headers, body);

    if (response.empty()) return {};

    if (response.find("expired") != std::string::npos) {
        std::cerr << "BitDefender API: " << response << "\n";
        return {};
    }

    // Parse JSON response
    Json::Value jsonData = parseJsonResponse(response);

    if (jsonData.isMember("categories")) {
        std::vector<std::string> categories;
        for (const auto& cat : jsonData["categories"]) {
            categories.push_back(cat.asString());
        }
        return categories;
    }
    return {};
}

// Send request to F-Secure API
std::vector<std::string> getFSecureCategories(const std::string& url) {
    std::wstring endpoint = FSECURE_HOST_ENDPOINT_START + stringToWstring(url) + FSECURE_HOST_ENDPOINT_END;
    std::wstring headers = L"X-PPGID: " + FSECURE_PPGID + L"\r\n"
        L"X-Transaction: " + FSECURE_TRANSACTION + L"\r\n"
        L"Authorization: " + FSECURE_TOKEN;

    std::string response = sendHttpRequest(FSECURE_HOST_URL, endpoint, L"GET", headers);

    if (response.empty()) return {};

    // Parse JSON response
    Json::Value jsonData = parseJsonResponse(response);

    if (jsonData.isMember("categories")) {
        std::vector<std::string> categories;
        for (const auto& cat : jsonData["categories"]) {
            categories.push_back(cat.asString());
        }
        return categories;
    }
    return {};
}

// Get token - authorization F-Secure
std::string getNewFSecureToken()
{
    std::wstring authHeader = createAuthHeaderW(FSECURE_USERNAME, FSECURE_PASSWORD);
    std::string body = wstringToString(FSECURE_TOKEN_BODY);
        
    WinHttpHandle hSession(
        WinHttpOpen(L"WinHTTP Example/1.0",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0));
    if (!hSession) {
        std::cerr << "WinHttpOpen failed\n";
        return "";
    }

    WinHttpHandle hConnect(
        WinHttpConnect(hSession.handle, FSECURE_TOKEN_URL.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0));
    if (!hConnect) {
        std::cerr << "WinHttpConnect failed\n";
        return "";
    }

    WinHttpHandle hRequest(
        WinHttpOpenRequest(hConnect.handle, L"POST", FSECURE_TOKEN_ENDPOINT.c_str(),
            NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE));
    if (!hRequest) {
        std::cerr << "WinHttpOpenRequest failed\n";
        return "";
    }


    std::wstring headers = L"Content-Type: application/json\r\n" + authHeader;
    if (!WinHttpSendRequest(hRequest.handle,
        headers.c_str(),
        static_cast<DWORD>(headers.length()),
        (LPVOID)body.c_str(),
        static_cast<DWORD>(body.length()),
        static_cast<DWORD>(body.length()),
        NULL)) {
        std::cerr << "WinHttpSendRequest failed\n";
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest.handle, NULL)) {
        std::cerr << "WinHttpReceiveResponse failed\n";
        return "";
    }

    DWORD size = 0;
    if (!WinHttpQueryDataAvailable(hRequest.handle, &size)) {
        std::cerr << "WinHttpQueryDataAvailable failed\n";
        return "";
    }
    std::vector<char> buffer(size + 1);
    DWORD bytesRead = 0;
    if (!WinHttpReadData(hRequest.handle, buffer.data(), size, &bytesRead)) {
        std::cerr << "WinHttpReadData failed\n";
        return "";
    }
    buffer[bytesRead] = '\0';
    std::string response = buffer.data();
    
    // JSON parsing with jsoncpp.windows
    Json::CharReaderBuilder reader;
    Json::Value root;
    std::string errs;

    std::istringstream ss(response);
    if (!Json::parseFromStream(reader, ss, &root, &errs)) {
        std::cerr << "Failed to parse JSON: " << errs << std::endl;
        return "";
    }

    // get "authorization"
    std::string auth_omit, auth_url;
    Json::Value nordnet = root["allowed_apis"]["nordnet_url_protection"];

    if (!nordnet.isNull()) {
        if (!nordnet["omit"]["headers"]["authorization"].isNull()) {
            auth_omit = nordnet["omit"]["headers"]["authorization"].asString();
        }
        if (!nordnet["url"]["headers"]["authorization"].isNull()) {
            auth_url = nordnet["url"]["headers"]["authorization"].asString();
        }
    }

    return auth_url;
}
std::string getCyrenCategories(const std::string& url)
{
    if (!m_bInitDll)
        return "";

    auto result = m_ctLib.ClassifyUrl(url.c_str());
    if (result != S_OK)
        return "";

    CONST USHORT* m_puCategories = NULL;
    USHORT nCount = 5;
    m_ctLib.GetCategories(&m_puCategories, &nCount);
    std::string temp;
    for (auto i = 0; i < nCount; ++i)
    {
        if(i != 0)
            temp += "|";
        temp += getCategoryCyren(m_puCategories[i]);
    }
    //m_ctLib.CloseCategories();

    return temp;
}

void processUrls(const std::string& inputFile) {
    std::ifstream inFile(inputFile);
    std::string outputFilename = getTimestampedFilename("output", ".csv");
    std::ofstream outFile(outputFilename);
    std::string url;

    // Write CSV header
    outFile << "URL,BitDefender,F-Secure,Cyren\n";

    std::string strToken = getNewFSecureToken();
    FSECURE_TOKEN = stringToWstring(strToken);

    while (std::getline(inFile, url)) {
        std::cout << "[Processing] URL: " << url << std::endl;
        trim(url);

        // ---- Measure time for BitDefender ----
        auto startBD = std::chrono::high_resolution_clock::now();
        auto bdCategories = getBitDefenderCategories(url);
        auto endBD = std::chrono::high_resolution_clock::now();
        auto timeBD = std::chrono::duration_cast<std::chrono::milliseconds>(endBD - startBD).count();

        // ---- Measure time for F-Secure ----
        auto startFS = std::chrono::high_resolution_clock::now();
        auto fsCategories = getFSecureCategories(url);
        auto endFS = std::chrono::high_resolution_clock::now();
        auto timeFS = std::chrono::duration_cast<std::chrono::milliseconds>(endFS - startFS).count();

        // ---- Measure time for Cyren ----
        auto startCR = std::chrono::high_resolution_clock::now();
        auto crCategories = getCyrenCategories(url);
        auto endCR = std::chrono::high_resolution_clock::now();
        auto timeCR = std::chrono::duration_cast<std::chrono::milliseconds>(endCR - startCR).count();

        // Join categories with | and add time in ()
        std::string bdJoined = "(" + std::to_string(timeBD) + "ms) ";
        for (size_t i = 0; i < bdCategories.size(); ++i) {
            bdJoined += bdCategories[i];
            if (i != bdCategories.size() - 1) bdJoined += "|";
        }

        std::string fsJoined = "(" + std::to_string(timeFS) + "ms) ";
        for (size_t i = 0; i < fsCategories.size(); ++i) {
            fsJoined += fsCategories[i];
            if (i != fsCategories.size() - 1) fsJoined += "|";
        }

        std::string crJoined = "(" + std::to_string(timeCR) + "ms) " + crCategories;

        // Write to CSV
        outFile << "\"" << url << "\","
            << "\"" << bdJoined << "\","
            << "\"" << fsJoined << "\","
            << "\"" << crJoined << "\"\n";
    }

    std::cout << "Output saved to output.csv\n";
}



// Entry point
int main() {  
    // Init Cyren lib
    HRESULT result = S_OK;
    result = m_ctLib.Load("asapsdk64.dll");
    if (result == S_OK)
    {
        result = m_ctLib.Init("DirTemp=C:\\Windows\\TEMP\\;WebSecZeroLatencyMode=0;ServerAddress=webres%d.nordnet.ctmail.com;LicenseKey=0001O190E1022T01080P:89e568b726b84f608cb6ed5ea6e06fa6;SleepMode=0;");
        if (S_OK == result)
        {
            m_bInitDll = true;
        }
    }
    
    // Init http session
    hSession = WinHttpOpen(L"WinHTTP Example/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return 0;

    // Get API information
    auto config = parseIniFile(L"config.ini");

    //information for API Bitdefender
    BITDEFENDER_HOST = config[L"BitDefender"][L"HOST_URL"];
    BITDEFENDER_ENDPOINT = config[L"BitDefender"][L"ENDPOINT"];
    BITDEFENDER_AUTH_TOKEN = config[L"BitDefender"][L"AUTH_TOKEN"];

    //information for API Fsecure
    FSECURE_HOST_URL = config[L"FSecure"][L"HOST_URL"];
    FSECURE_HOST_ENDPOINT_START = config[L"FSecure"][L"HOST_ENDPOINT_START"];
    FSECURE_HOST_ENDPOINT_END = config[L"FSecure"][L"HOST_ENDPOINT_END"];
    FSECURE_PPGID = config[L"FSecure"][L"PPGID"];
    FSECURE_TRANSACTION = config[L"FSecure"][L"TRANSACTION"];

    //information for API Get Fsecure token
    FSECURE_TOKEN_URL = config[L"FSecure"][L"TOKEN_URL"];
    FSECURE_TOKEN_ENDPOINT = config[L"FSecure"][L"TOKEN_ENDPOINT"];
    FSECURE_USERNAME = config[L"FSecure"][L"USERNAME"];
    FSECURE_PASSWORD = config[L"FSecure"][L"PASSWORD"];
    FSECURE_TOKEN_BODY = config[L"FSecure"][L"TOKEN_BODY"];    

    processUrls("input.txt");

    if(hSession)
        WinHttpCloseHandle(hSession);
         
    return 0;
}
