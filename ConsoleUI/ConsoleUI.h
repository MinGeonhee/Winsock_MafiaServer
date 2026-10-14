#pragma once
/* ============================================================
   ConsoleUI : 콘솔 화면 출력 전담 (서버/클라 공용, 헤더 전용)
   - 색/커서 제어는 VT 시퀀스(ESC[...) 사용
   - 문자열은 UTF-16(wstring) + WriteConsoleW → 한글/이모지 출력
   ============================================================ */
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ConsoleUI
{
	namespace Color
	{
		inline constexpr const wchar_t* Reset	= L"\x1b[0m";
		inline constexpr const wchar_t* Bold	= L"\x1b[1m";
		inline constexpr const wchar_t* Gray	= L"\x1b[90m";
		inline constexpr const wchar_t* Red		= L"\x1b[91m";
		inline constexpr const wchar_t* Green	= L"\x1b[92m";
		inline constexpr const wchar_t* Yellow	= L"\x1b[93m";
		inline constexpr const wchar_t* Cyan	= L"\x1b[96m";
	}

	/* 문자열 한 번에 출력 (중간 상태가 화면에 안 보이게 항상 통째로 씀) */
	inline void Write(const std::wstring& str)
	{
		DWORD dwWritten{ 0 };
		WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str.c_str(), static_cast<DWORD>(str.size()), &dwWritten, nullptr);
	}

	inline void Init(const wchar_t* szTitle)
	{
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		DWORD dwMode{ 0 };
		if (GetConsoleMode(hOut, &dwMode))
			SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);	// ESC[ 시퀀스 해석 켜기

		SetConsoleTitleW(szTitle);
	}

	inline void Clear()
	{
		Write(L"\x1b[2J\x1b[3J\x1b[H");	// 화면 지우기 + 스크롤 기록 지우기 + 커서 맨 위로
	}

	/* 엔터까지 한 줄 입력 (앞뒤 공백 제거) */
	inline std::wstring Read_Line()
	{
		wchar_t szBuf[256]{};
		DWORD dwRead{ 0 };
		if (!ReadConsoleW(GetStdHandle(STD_INPUT_HANDLE), szBuf, 255, &dwRead, nullptr))
			return L"";

		std::wstring str(szBuf, dwRead);
		const size_t iBegin = str.find_first_not_of(L" \t\r\n");
		if (iBegin == std::wstring::npos) return L"";
		const size_t iEnd = str.find_last_not_of(L" \t\r\n");
		return str.substr(iBegin, iEnd - iBegin + 1);
	}

	/* "1024"~"65535" 숫자면 포트 반환, 아니면 0 (0번 포트는 어차피 못 씀) */
	constexpr uint16_t Parse_Port(std::wstring_view str)
	{
		if (str.empty() || str.size() > 5) return 0;

		uint32_t iValue{ 0 };
		for (wchar_t ch : str)
		{
			if (ch < L'0' || ch > L'9') return 0;
			iValue = iValue * 10 + (ch - L'0');
		}
		return (iValue >= 1024 && iValue <= 65535) ? static_cast<uint16_t>(iValue) : 0;
	}
	static_assert(Parse_Port(L"1234") == 1234 && Parse_Port(L"65535") == 65535
		&& Parse_Port(L"80") == 0 && Parse_Port(L"65536") == 0 && Parse_Port(L"12a4") == 0 && Parse_Port(L"") == 0);

	/* 이 PC의 IPv4 주소 목록 (클라이언트가 접속할 주소 안내용) */
	inline std::vector<std::wstring> Get_LocalIPv4()
	{
		std::vector<std::wstring> vecIP;

		WSADATA tData{};
		if (WSAStartup(MAKEWORD(2, 2), &tData) != 0)	// 참조 카운트 방식이라 서버의 Net_Init과 겹쳐도 됨
			return vecIP;

		char szHost[256]{};
		addrinfo tHint{};
		tHint.ai_family = AF_INET;
		tHint.ai_socktype = SOCK_STREAM;
		addrinfo* pResult{ nullptr };

		if (gethostname(szHost, sizeof(szHost)) == 0 && getaddrinfo(szHost, nullptr, &tHint, &pResult) == 0)
		{
			for (addrinfo* p = pResult; p != nullptr; p = p->ai_next)
			{
				wchar_t szIP[INET_ADDRSTRLEN]{};
				InetNtopW(AF_INET, &reinterpret_cast<sockaddr_in*>(p->ai_addr)->sin_addr, szIP, INET_ADDRSTRLEN);
				vecIP.emplace_back(szIP);
			}
			freeaddrinfo(pResult);
		}

		WSACleanup();
		return vecIP;
	}

	/* 시작 화면: IP 안내 + 포트 입력. 올바른 포트가 들어올 때까지 반복 */
	inline uint16_t Run_StartupScreen(uint16_t iDefaultPort, const std::wstring& strError = L"")
	{
		using namespace Color;

		const std::vector<std::wstring> vecIP = Get_LocalIPv4();
		const std::wstring strLine(48, L'─');
		std::wstring strMsg = strError;

		while (true)
		{
			std::wstring str;
			str += L"\n  "; str += Red; str += Bold; str += L"🔪 MAFIA SERVER"; str += Reset;
			str += Gray; str += L"   7인 마피아 게임 서버\n  "; str += strLine; str += Reset; str += L"\n\n";

			str += L"  "; str += Bold; str += L"이 PC의 IP 주소"; str += Reset;
			str += Gray; str += L"  (클라이언트에서 이 주소로 접속)\n"; str += Reset;
			for (const std::wstring& strIP : vecIP)
			{
				str += L"    "; str += Green; str += L"● "; str += Reset; str += strIP; str += L"\n";
			}
			str += L"    "; str += Green; str += L"● "; str += Reset; str += L"127.0.0.1";
			str += Gray; str += L"  (같은 PC에서 테스트)\n\n"; str += Reset;

			str += L"  "; str += Bold; str += L"사용할 포트 번호"; str += Reset;
			str += Gray; str += L"  (1024 ~ 65535, Enter = "; str += std::to_wstring(iDefaultPort); str += L")\n"; str += Reset;

			if (!strMsg.empty())
			{
				str += L"  "; str += Red; str += L"✖ "; str += strMsg; str += Reset; str += L"\n";
			}
			str += L"  "; str += Yellow; str += L"> "; str += Reset;

			Clear();
			Write(str);

			const std::wstring strInput = Read_Line();
			if (strInput.empty()) return iDefaultPort;

			if (const uint16_t iPort = Parse_Port(strInput); iPort != 0)
				return iPort;

			strMsg = L"'" + strInput + L"' 은(는) 사용할 수 없습니다. 1024 ~ 65535 사이 숫자를 입력하세요.";
		}
	}
}
