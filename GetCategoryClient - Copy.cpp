#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <winhttp.h>

#pragma comment(lib, "winhttp.lib")

// Hàm gửi request đến API và nhận JSON response
std::string SendRequestToAPI(const std::string& url, const std::string& version) {
    HINTERNET hSession = WinHttpOpen(L"MFC App", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    HINTERNET hConnect = WinHttpConnect(hSession, L"localhost", 5000, 0);
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/api/v1/process_url", NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);

    std::wstring headers = L"Content-Type: application/json";
    std::string requestBody = "{\"url\": \"" + url + "\", \"version\": \"" + version + "\"}";

    BOOL bResults = WinHttpSendRequest(hRequest, headers.c_str(), -1L, (LPVOID)requestBody.c_str(), requestBody.length(), requestBody.length(), 0);
    WinHttpReceiveResponse(hRequest, NULL);

    DWORD dwSize = 0;
    WinHttpQueryDataAvailable(hRequest, &dwSize);
    std::vector<char> buffer(dwSize + 1, 0);
    DWORD dwDownloaded = 0;
    WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return std::string(buffer.begin(), buffer.end());
}

void ProcessURLs(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile);
    std::ofstream outFile(outputFile);
    std::string line;
    std::vector<std::string> ids;

    while (std::getline(inFile, line)) {
        std::string response = SendRequestToAPI(line, "1.0");
        std::size_t idPos = response.find("\"ID\":");
        if (idPos != std::string::npos) {
            std::size_t start = response.find_first_of("0123456789", idPos);
            std::size_t end = response.find_first_not_of("0123456789", start);
            ids.push_back(response.substr(start, end - start));
        }
    }

    for (size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) outFile << "|";
        outFile << ids[i];
    }
}

int main() {
    ProcessURLs("input.txt", "output.txt");
    std::cout << "Processing completed. Check output.txt" << std::endl;
    return 0;
}
