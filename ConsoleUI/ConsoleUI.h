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

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <mutex>
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

	inline void Set_CursorVisible(bool isVisible)
	{
		Write(isVisible ? L"\x1b[?25h" : L"\x1b[?25l");
	}

	/* 쌓여 있는 키 입력만 꺼냄. 없으면 바로 반환해서 게임 루프가 멈추지 않음
	   누르고 있는 동안 들어오는 자동 반복은 무시 → 한 번 누르면 한 번만 반환 (Key Down 순간만) */
	inline std::vector<KEY_EVENT_RECORD> Poll_Keys()
	{
		static bool s_isHeld[256]{};	// 가상 키 코드별로 지금 눌려 있는지
		std::vector<KEY_EVENT_RECORD> vecKeys;
		HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

		DWORD dwCount{ 0 };
		if (!GetNumberOfConsoleInputEvents(hIn, &dwCount) || dwCount == 0)
			return vecKeys;

		INPUT_RECORD tRecords[64]{};
		DWORD dwRead{ 0 };
		if (!ReadConsoleInputW(hIn, tRecords, 64, &dwRead))
			return vecKeys;

		for (DWORD i = 0; i < dwRead; ++i)
		{
			if (tRecords[i].EventType != KEY_EVENT) continue;

			const KEY_EVENT_RECORD& tKey = tRecords[i].Event.KeyEvent;
			bool& isHeld = s_isHeld[tKey.wVirtualKeyCode & 0xFF];

			if (tKey.bKeyDown && isHeld == false)
				vecKeys.push_back(tKey);	// 떼어져 있다가 막 눌린 순간만
			isHeld = (tKey.bKeyDown != FALSE);
		}
		return vecKeys;
	}

	/* 화면 맨 위 제목 줄 (모든 화면 공통) */
	inline std::wstring Title_Line(const std::wstring& strSub)
	{
		using namespace Color;
		std::wstring str;
		str += L"\n  "; str += Red; str += Bold; str += L"🔪 MAFIA SERVER"; str += Reset;
		str += Gray; str += L"   "; str += strSub; str += L"\x1b[K\n  "; str += std::wstring(48, L'─'); str += Reset; str += L"\x1b[K\n\x1b[K\n";
		return str;
	}

	/* 서버 준비 중 로딩 (진행 바, dwMs 동안) */
	inline void Run_Loading(uint16_t iPort, DWORD dwMs)
	{
		using namespace Color;
		constexpr int	iBarLen = 30;
		constexpr DWORD	dwStep = 50;
		const wchar_t	szSpin[] = L"|/-\\";

		Set_CursorVisible(false);
		Clear();
		for (DWORD dwTime = 0; dwTime <= dwMs; dwTime += dwStep)
		{
			const int iFill = static_cast<int>(iBarLen * dwTime / dwMs);
			const int iPercent = static_cast<int>(100 * dwTime / dwMs);

			std::wstring str = L"\x1b[H" + Title_Line(L"7인 마피아 게임 서버");
			str += L"  "; str += Yellow; str += szSpin[(dwTime / dwStep) % 4]; str += Reset;
			str += L" 서버 준비 중  "; str += Gray; str += L"(포트 "; str += std::to_wstring(iPort); str += L")"; str += Reset; str += L"\x1b[K\n\n";
			str += L"  "; str += Green; str += std::wstring(iFill, L'█'); str += Gray; str += std::wstring(iBarLen - iFill, L'░'); str += Reset;
			str += L"  "; str += std::to_wstring(iPercent); str += L"%\x1b[K\n";
			Write(str);
			Sleep(dwStep);
		}
	}

	/* ---------------- 글자 폭 (한글·이모지 = 2칸) ---------------- */
	inline int Char_Width(char32_t c)
	{
		if ((c >= 0x1100 && c <= 0x115F) || (c >= 0x2E80 && c <= 0xA4CF) || (c >= 0xAC00 && c <= 0xD7A3) ||
			(c >= 0xF900 && c <= 0xFAFF) || (c >= 0xFE30 && c <= 0xFE4F) || (c >= 0xFF00 && c <= 0xFF60) ||
			(c >= 0xFFE0 && c <= 0xFFE6) || (c >= 0x1F300 && c <= 0x1FAFF))
			return 2;
		return 1;
	}

	/* 화면 폭 iWidth 칸에 맞춰 자르고 남는 칸은 공백으로 채움 (색 코드 없는 순수 문자열용) */
	inline std::wstring Fit(const std::wstring& str, int iWidth)
	{
		std::wstring strOut;
		int iUsed{ 0 };
		for (size_t i = 0; i < str.size(); ++i)
		{
			char32_t c = str[i];
			size_t iLen = 1;
			if (c >= 0xD800 && c <= 0xDBFF && i + 1 < str.size())	// 서로게이트 페어 (이모지)
			{
				c = 0x10000 + ((c - 0xD800) << 10) + (str[i + 1] - 0xDC00);
				iLen = 2;
			}
			const int iW = Char_Width(c);
			if (iUsed + iW > iWidth) break;
			strOut.append(str, i, iLen);
			iUsed += iW;
			i += iLen - 1;
		}
		strOut.append(static_cast<size_t>(iWidth - iUsed), L' ');
		return strOut;
	}

	inline std::wstring Widen(const std::string& str)	// IP 같은 ASCII 문자열용
	{
		return std::wstring(str.begin(), str.end());
	}

	/* ---------------- 로그 (어느 스레드에서 불러도 안전) ---------------- */
	namespace Detail
	{
		inline std::mutex				g_LogLock;
		inline std::deque<std::wstring>	g_Logs;
		inline std::atomic<bool>		g_isNewLog{ false };
		inline SHORT					g_iLastCols{ 0 }, g_iLastRows{ 0 };
		constexpr size_t				MAX_LOGS = 300;
	}

	/* 로그 한 줄 추가. 앞에 [시:분:초]가 자동으로 붙음 */
	inline void Add_Log(const std::wstring& strLog)
	{
		SYSTEMTIME tTime{};
		GetLocalTime(&tTime);
		wchar_t szTime[16]{};
		swprintf_s(szTime, L"[%02d:%02d:%02d] ", tTime.wHour, tTime.wMinute, tTime.wSecond);

		std::lock_guard<std::mutex> lock(Detail::g_LogLock);
		Detail::g_Logs.push_back(szTime + strLog);
		if (Detail::g_Logs.size() > Detail::MAX_LOGS)
			Detail::g_Logs.pop_front();
		Detail::g_isNewLog = true;
	}

	/* 새 로그가 들어왔거나 창 크기가 바뀌었으면 true (확인하면 다시 false) */
	inline bool Need_Redraw()
	{
		CONSOLE_SCREEN_BUFFER_INFO tInfo{};
		bool isResized = false;
		if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &tInfo))
		{
			const SHORT iCols = tInfo.srWindow.Right - tInfo.srWindow.Left + 1;
			const SHORT iRows = tInfo.srWindow.Bottom - tInfo.srWindow.Top + 1;
			isResized = (iCols != Detail::g_iLastCols || iRows != Detail::g_iLastRows);
			Detail::g_iLastCols = iCols; Detail::g_iLastRows = iRows;
		}
		return Detail::g_isNewLog.exchange(false) || isResized;
	}

	/* 서버 화면 왼쪽 목록에 보여줄 접속자 한 명 */
	struct ClientView
	{
		uint64_t	iKey{ 0 };
		std::string	strIP;
		uint16_t	iPort{ 0 };
	};

	/* 서버 메인 화면: 상단 상태 + 왼쪽 접속자 목록 + 오른쪽 실시간 로그 */
	inline void Render_ServerMain(uint16_t iPort, bool isAccepting, const std::vector<ClientView>& vecClients = {})
	{
		using namespace Color;

		CONSOLE_SCREEN_BUFFER_INFO tInfo{};
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &tInfo);
		const int iCols = tInfo.srWindow.Right - tInfo.srWindow.Left + 1;
		const int iRows = tInfo.srWindow.Bottom - tInfo.srWindow.Top + 1;

		std::wstring str = L"\x1b[H" + Title_Line(L"포트 " + std::to_wstring(iPort) + L"에서 실행 중");

		str += L"  "; str += Bold; str += L"서버 접속 요청"; str += Reset; str += L"   ";
		if (isAccepting)
		{
			str += Green; str += Bold; str += L"● ON "; str += Reset;
			str += Gray; str += L" 새 클라이언트의 접속을 받는 중"; str += Reset;
		}
		else
		{
			str += Red; str += Bold; str += L"○ OFF"; str += Reset;
			str += Gray; str += L" 새 접속을 받지 않음"; str += Reset;
		}
		str += L"\x1b[K\n\x1b[K\n";

		/* 아래 두 칸: 왼쪽 접속자 | 오른쪽 로그 */
		constexpr int	iLeftW = 30;
		const int		iRightW = (std::max)(10, iCols - iLeftW - 7);
		const int		iBodyRows = (std::max)(3, iRows - 10);	// 위 6줄, 아래 안내 2줄, 여유 2줄

		str += L"  "; str += Bold; str += Fit(L"접속자 (" + std::to_wstring(vecClients.size()) + L")", iLeftW); str += Reset;
		str += Gray; str += L" │ "; str += Reset; str += Bold; str += L"로그"; str += Reset; str += L"\x1b[K\n";
		str += L"  "; str += Gray; str += std::wstring(iLeftW, L'─'); str += L"─┼─"; str += std::wstring(iRightW, L'─'); str += Reset; str += L"\x1b[K\n";

		std::vector<std::wstring> vecLogs;
		{
			std::lock_guard<std::mutex> lock(Detail::g_LogLock);
			const size_t iCount = (std::min)(Detail::g_Logs.size(), static_cast<size_t>(iBodyRows));
			vecLogs.assign(Detail::g_Logs.end() - iCount, Detail::g_Logs.end());	// 최신 로그가 아래로
		}

		for (int iRow = 0; iRow < iBodyRows; ++iRow)
		{
			std::wstring strLeft;
			if (iRow < static_cast<int>(vecClients.size()))
			{
				const ClientView& tClient = vecClients[iRow];
				strLeft = L"#" + std::to_wstring(tClient.iKey) + L"  " + Widen(tClient.strIP) + L":" + std::to_wstring(tClient.iPort);
			}
			const std::wstring strRight = iRow < static_cast<int>(vecLogs.size()) ? vecLogs[iRow] : L"";

			str += L"  "; str += Fit(strLeft, iLeftW);
			str += Gray; str += L" │ "; str += Reset;
			str += Fit(strRight, iRightW); str += L"\x1b[K\n";
		}

		str += L"  "; str += Gray; str += std::wstring(iLeftW, L'─'); str += L"─┴─"; str += std::wstring(iRightW, L'─'); str += Reset; str += L"\x1b[K\n";
		str += L"  "; str += Yellow; str += L"[F5]"; str += Reset; str += L" 접속 요청 On / Off\x1b[K";
		str += L"\x1b[J";	// 아래쪽 이전 내용 지우기
		Write(str);
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
