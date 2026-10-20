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
#include <cwctype>
#include <deque>
#include <functional>
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
	   기본: 누르고 있는 동안 들어오는 자동 반복은 무시 → 한 번 누르면 한 번만 반환 (Key Down 순간만)
	   isAllowRepeat = true : 글자 입력용. 누르고 있으면 계속 반환 (Backspace 꾹 누르기 등) */
	inline std::vector<KEY_EVENT_RECORD> Poll_Keys(bool isAllowRepeat = false)
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

			if (tKey.bKeyDown && (isHeld == false || isAllowRepeat))
				vecKeys.push_back(tKey);	// 떼어져 있다가 막 눌린 순간만 (isAllowRepeat면 반복 포함)
			isHeld = (tKey.bKeyDown != FALSE);
		}
		return vecKeys;
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

	/* ============================================================
	   클라이언트 화면 : 인트로 / 로그인 / 로비
	   ============================================================ */

	/* 순수 문자열(색 코드 없음)의 화면 칸 수 */
	inline int Width(const std::wstring& str)
	{
		int iWidth{ 0 };
		for (size_t i = 0; i < str.size(); ++i)
		{
			char32_t c = str[i];
			if (c >= 0xD800 && c <= 0xDBFF && i + 1 < str.size())
			{
				c = 0x10000 + ((c - 0xD800) << 10) + (str[i + 1] - 0xDC00);
				++i;
			}
			iWidth += Char_Width(c);
		}
		return iWidth;
	}

	inline std::wstring Spaces(int iCount) { return std::wstring(static_cast<size_t>((std::max)(0, iCount)), L' '); }

	inline void Get_ConsoleSize(int& iCols, int& iRows)
	{
		CONSOLE_SCREEN_BUFFER_INFO tInfo{};
		GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &tInfo);
		iCols = tInfo.srWindow.Right - tInfo.srWindow.Left + 1;
		iRows = tInfo.srWindow.Bottom - tInfo.srWindow.Top + 1;
	}

	/* 가운데 정렬용 왼쪽 여백 (strPlain: 색 코드 없는 문자열) */
	inline std::wstring Center_Pad(const std::wstring& strPlain, int iCols)
	{
		return Spaces((iCols - Width(strPlain)) / 2);
	}

	/* ---------------- MAFIA 로고 (38칸 × 6줄) ---------------- */
	namespace Detail
	{
		inline const wchar_t* const LOGO[6] = {
			L"███╗   ███╗ █████╗ ███████╗██╗ █████╗ ",
			L"████╗ ████║██╔══██╗██╔════╝██║██╔══██╗",
			L"██╔████╔██║███████║█████╗  ██║███████║",
			L"██║╚██╔╝██║██╔══██║██╔══╝  ██║██╔══██║",
			L"██║ ╚═╝ ██║██║  ██║██║     ██║██║  ██║",
			L"╚═╝     ╚═╝╚═╝  ╚═╝╚═╝     ╚═╝╚═╝  ╚═╝" };
		constexpr int LOGO_W = 38;
		inline const wchar_t* const SHADOW = L"\x1b[22;38;2;90;18;24m";	// 어두운 빨강 (선 부분)
		inline const wchar_t* const BLOCK  = L"\x1b[1;91m";					// 밝은 빨강 굵게 (█ 부분)
		inline const wchar_t* const SELECT = L"\x1b[48;2;110;30;36;97m";	// 선택된 줄 / 버튼 배경
		inline const wchar_t* const FIELD  = L"\x1b[48;5;237m";				// 입력 칸 배경
		inline const wchar_t* const CURSOR = L"\x1b[7m";					// 반전 (커서)
	}

	/* 로고 한 줄 (색 코드 포함, 38칸) */
	inline std::wstring Logo_Line(int iRow)
	{
		std::wstring str;
		int iMode = -1;		// 0 = 블록, 1 = 선, 2 = 공백 : 바뀔 때만 색 코드를 넣음
		for (const wchar_t* p = Detail::LOGO[iRow]; *p; ++p)
		{
			const int iNext = (*p == L'█') ? 0 : (*p == L' ') ? 2 : 1;
			if (iNext != iMode && iNext != 2)
				str += (iNext == 0) ? Detail::BLOCK : Detail::SHADOW;
			if (iNext != 2) iMode = iNext;
			str += *p;
		}
		return str + Color::Reset;
	}

	/* ---------------- 인트로 ---------------- */
	/* 로고 한 줄씩 → 칼 선 → 문구 타이핑 → '아무 키나 눌러 시작' 깜빡임. 키를 누르면 반환 */
	inline void Play_Intro()
	{
		using namespace Color;
		const std::wstring strSub = L"밤이 되었습니다. 마피아는 고개를 들어주세요.";
		const std::wstring strPrompt = L"▶ 아무 키나 눌러 시작";

		auto Draw = [&](int iLogoRows, int iBarLen, size_t iSubLen, bool isPrompt)
		{
			int iCols{ 0 }, iRows{ 0 };
			Get_ConsoleSize(iCols, iRows);
			const std::wstring strLeft = Spaces((iCols - Detail::LOGO_W) / 2);
			const int iTop = (std::max)(1, (iRows - 16) / 2);

			std::wstring str = L"\x1b[H";
			for (int i = 0; i < iTop; ++i) str += L"\x1b[K\n";
			for (int i = 0; i < 6; ++i)
				str += (i < iLogoRows ? strLeft + Logo_Line(i) : L"") + L"\x1b[K\n";
			str += L"\x1b[K\n";

			str += strLeft; str += Red; str += std::wstring(static_cast<size_t>(iBarLen), L'━'); str += Reset;
			if (iBarLen >= Detail::LOGO_W) str += L" 🔪";
			str += L"\x1b[K\n\x1b[K\n";

			const std::wstring strShown = strSub.substr(0, iSubLen);
			str += Center_Pad(strSub, iCols); str += Gray; str += strShown; str += Reset; str += L"\x1b[K\n\x1b[K\n\x1b[K\n";

			if (isPrompt) { str += Center_Pad(strPrompt, iCols); str += Yellow; str += Bold; str += strPrompt; str += Reset; }
			str += L"\x1b[K\x1b[J";
			Write(str);
		};

		Set_CursorVisible(false);
		Clear();
		Draw(0, 0, 0, false);					Sleep(600);
		for (int i = 1; i <= 6; ++i) { Draw(i, 0, 0, false); Sleep(120); }
		Draw(6, Detail::LOGO_W / 2, 0, false);	Sleep(120);
		Draw(6, Detail::LOGO_W, 0, false);		Sleep(500);
		for (size_t i = 1; i <= strSub.size(); i += 3) { Draw(6, Detail::LOGO_W, i, false); Sleep(40); }

		FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));	// 인트로 중에 눌린 키는 버림
		for (int iTick = 0; ; ++iTick)
		{
			if (iTick % 10 == 0)	// 0.5초마다 깜빡임
				Draw(6, Detail::LOGO_W, strSub.size(), (iTick / 10) % 2 == 0);
			if (!Poll_Keys().empty()) break;
			Sleep(50);
		}
	}

	/* ---------------- 입력 카드 (서버 접속 / 로그인 공용) ---------------- */
	namespace Detail
	{
		enum class FIELD_KIND { NICK, IP, PORT };

		struct FormField
		{
			const wchar_t*	szLabel;		// 7칸 폭으로 맞춘 라벨 ("닉네임 ", "서버 IP", "포트   ")
			std::wstring	strValue;
			FIELD_KIND		eKind;
		};

		struct FormText
		{
			std::wstring	strTitle;		// 카드 위 테두리 제목 (" 로 그 인 ")
			std::wstring	strInfo;		// 로고 아래 회색 한 줄 (비면 기본 부제)
			std::wstring	strButton;		// "  Enter  접속하기  "
			std::vector<std::pair<std::wstring, std::wstring>> vecHelp;	// { "[Tab]", "다음 칸" } ...
		};

		inline int g_iStatusRow{ 0 };		// Show_Status가 그릴 줄 (마지막으로 그린 카드의 메시지 줄)

		/* 칸마다 받을 수 있는 글자 제한 */
		inline bool Accept_Char(FIELD_KIND eKind, const std::wstring& strCur, wchar_t ch, size_t iMaxNickLen)
		{
			switch (eKind)
			{
			case FIELD_KIND::NICK:	return ch != L' ' && strCur.size() < iMaxNickLen;
			case FIELD_KIND::IP:	return (iswdigit(ch) || ch == L'.') && strCur.size() < 15;
			case FIELD_KIND::PORT:	return iswdigit(ch) && strCur.size() < 5;
			}
			return false;
		}

		/* 입력 카드 하나 띄우고 Enter(검사 통과) = true, Esc = false
		   fnValidate : 문제 있으면 메시지를 돌려주고 iFocus를 그 칸으로 (문제 없으면 빈 문자열) */
		inline bool Run_Form(const FormText& tText, std::vector<FormField>& vecFields, const std::wstring& strError, size_t iMaxNickLen,
							 const std::function<std::wstring(const std::vector<FormField>&, int&)>& fnValidate)
		{
			using namespace Color;
			constexpr int	BOX_W = 44, FIELD_W = 26;
			const int		iFieldCount = static_cast<int>(vecFields.size());

			int				iFocus{ 0 };
			std::wstring	strMsg = strError;
			bool			isDirty{ true };

			Set_CursorVisible(false);
			Clear();
			FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));

			while (true)
			{
				/* ---- 입력 ---- */
				for (const KEY_EVENT_RECORD& tKey : Poll_Keys(true))
				{
					isDirty = true;
					std::wstring& strCur = vecFields[iFocus].strValue;
					const wchar_t ch = tKey.uChar.UnicodeChar;

					if (tKey.wVirtualKeyCode == VK_ESCAPE) { Set_CursorVisible(true); return false; }
					else if (tKey.wVirtualKeyCode == VK_TAB || tKey.wVirtualKeyCode == VK_DOWN) iFocus = (iFocus + 1) % iFieldCount;
					else if (tKey.wVirtualKeyCode == VK_UP) iFocus = (iFocus + iFieldCount - 1) % iFieldCount;
					else if (tKey.wVirtualKeyCode == VK_BACK)
					{
						if (!strCur.empty()) strCur.pop_back();
						if (!strCur.empty() && strCur.back() >= 0xD800 && strCur.back() <= 0xDBFF) strCur.pop_back();	// 이모지 반쪽 정리
					}
					else if (tKey.wVirtualKeyCode == VK_RETURN)
					{
						strMsg = fnValidate ? fnValidate(vecFields, iFocus) : std::wstring();
						if (strMsg.empty()) { Set_CursorVisible(true); return true; }
					}
					else if (ch >= 32 && Accept_Char(vecFields[iFocus].eKind, strCur, ch, iMaxNickLen))
						strCur += ch;
				}

				/* ---- 그리기 (바뀐 게 있거나 창 크기가 바뀌었을 때만) ---- */
				if (isDirty || Need_Redraw())
				{
					isDirty = false;
					int iCols{ 0 }, iRows{ 0 };
					Get_ConsoleSize(iCols, iRows);
					const std::wstring strLogoLeft = Spaces((iCols - LOGO_W) / 2);
					const std::wstring strBoxLeft = Spaces((iCols - BOX_W - 2) / 2);
					const std::wstring strSubtitle = tText.strInfo.empty() ? L"7인 마피아 게임 · CMD Edition" : tText.strInfo;

					std::wstring str = L"\x1b[H\x1b[K\n";
					for (int i = 0; i < 6; ++i) str += strLogoLeft + Logo_Line(i) + L"\x1b[K\n";
					str += Center_Pad(strSubtitle, iCols); str += Gray; str += strSubtitle; str += Reset; str += L"\x1b[K\n\x1b[K\n";

					/* 상단 테두리: ┌──── 제목 ────┐ */
					const int iLeftLine = (BOX_W - Width(tText.strTitle)) / 2;
					str += strBoxLeft; str += Gray; str += L"┌"; str += std::wstring(iLeftLine, L'─'); str += Reset;
					str += Bold; str += tText.strTitle; str += Reset;
					str += Gray; str += std::wstring(BOX_W - iLeftLine - Width(tText.strTitle), L'─'); str += L"┐"; str += Reset; str += L"\x1b[K\n";

					auto Row = [&](const std::wstring& strColored, int iPlainWidth)
					{
						return strBoxLeft + Gray + L"│" + Reset + strColored + Spaces(BOX_W - iPlainWidth) + Gray + L"│" + Reset + L"\x1b[K\n";
					};

					str += Row(L"", 0);
					for (int i = 0; i < iFieldCount; ++i)
					{
						const FormField& tField = vecFields[i];
						const bool isFocus = (i == iFocus);
						std::wstring strLine = isFocus ? std::wstring(L"  ") + Yellow + L"▸ " + Reset + Bold + tField.szLabel + Reset
													   : std::wstring(L"    ") + Gray + tField.szLabel + Reset;
						strLine += L"  ";
						const int iTextW = Width(tField.strValue);
						if (isFocus)
							strLine += std::wstring(FIELD) + L" " + tField.strValue + CURSOR + L" " + Reset + FIELD + Spaces(FIELD_W - 2 - iTextW) + Reset;
						else
							strLine += L" " + Fit(tField.strValue, FIELD_W - 1);
						str += Row(strLine, 4 + Width(tField.szLabel) + 2 + FIELD_W);
					}
					str += Row(L"", 0);

					const int iButtonLeft = (BOX_W - Width(tText.strButton)) / 2;
					str += Row(Spaces(iButtonLeft) + SELECT + tText.strButton + Reset, iButtonLeft + Width(tText.strButton));
					str += Row(L"", 0);
					str += strBoxLeft; str += Gray; str += L"└"; str += std::wstring(BOX_W, L'─'); str += L"┘"; str += Reset; str += L"\x1b[K\n";

					/* 오류 메시지 + 조작 안내 */
					str += strMsg.empty() ? std::wstring(L"\x1b[K\n")
										  : Center_Pad(L"✖ " + strMsg, iCols) + Red + L"✖ " + strMsg + Reset + L"\x1b[K\n";
					std::wstring strHelpPlain, strHelpColored;
					for (size_t i = 0; i < tText.vecHelp.size(); ++i)
					{
						const std::wstring strGap = (i + 1 < tText.vecHelp.size()) ? L"    " : L"";
						strHelpPlain   += tText.vecHelp[i].first + L" " + tText.vecHelp[i].second + strGap;
						strHelpColored += std::wstring(Yellow) + tText.vecHelp[i].first + Reset + Gray + L" " + tText.vecHelp[i].second + strGap + Reset;
					}
					str += Center_Pad(strHelpPlain, iCols) + strHelpColored + L"\x1b[K\x1b[J";

					/* 행 번호 (1부터) : 빈 줄 1 + 로고 6 + 부제 1 + 빈 줄 1 + 위 테두리 1 + 빈 칸 1 = 11줄 다음부터 입력칸
					   입력칸 n개 → 빈 칸 / 버튼 / 빈 칸 / 아래 테두리 → 메시지 줄 = 12 + n + 4 */
					g_iStatusRow = 12 + iFieldCount + 4;

					/* (숨긴) 진짜 커서를 입력 중인 칸의 글자 끝으로 이동
					   → 한글 IME가 조합 중인 글자(ㅁ → 미 → 민)를 커서 위치에 띄우므로, 안 옮기면 맨 끝 안내문 옆에 보임
					   열: 박스 왼쪽 여백 + "│" + "  ▸ " + 라벨 + "  " + " " + 입력된 글자 */
					const int iCaretRow = 12 + iFocus;
					const int iCaretCol = static_cast<int>(strBoxLeft.size()) + 1 + 4 + Width(vecFields[iFocus].szLabel) + 2 + 1 + Width(vecFields[iFocus].strValue) + 1;
					str += L"\x1b[" + std::to_wstring(iCaretRow) + L";" + std::to_wstring(iCaretCol) + L"H";
					Write(str);
				}
				Sleep(16);
			}
		}
	}

	/* 카드 아래(오류 메시지 줄)에 상태 한 줄 표시 : "⋯ 서버에 접속하는 중", "⋯ 로그인 확인 중" 등
	   connect처럼 잠깐 멈추는 동안 멈춘 게 아니라는 걸 보여줌 */
	inline void Show_Status(const std::wstring& strMsg)
	{
		using namespace Color;
		int iCols{ 0 }, iRows{ 0 };
		Get_ConsoleSize(iCols, iRows);
		Set_CursorVisible(false);
		Write(L"\x1b[" + std::to_wstring(Detail::g_iStatusRow) + L";1H\x1b[K" + Center_Pad(strMsg, iCols) + Yellow + strMsg + Reset);
	}

	/* ---------------- 서버 접속 (IP / 포트) ---------------- */
	struct ConnectResult
	{
		bool			isOk{ false };		// Esc로 나가면 false
		std::string		strIP;
		uint16_t		iPort{ 0 };
	};

	inline ConnectResult Run_ServerConnect(const std::wstring& strDefaultIP = L"127.0.0.1", uint16_t iDefaultPort = 1234, const std::wstring& strError = L"")
	{
		std::vector<Detail::FormField> vecFields = {
			{ L"서버 IP", strDefaultIP,						Detail::FIELD_KIND::IP },
			{ L"포트   ", std::to_wstring(iDefaultPort),	Detail::FIELD_KIND::PORT } };
		const Detail::FormText tText{ L" 서 버 접 속 ", L"", L"  Enter  접속하기  ",
			{ { L"[Tab]", L"다음 칸" }, { L"[↑↓]", L"이동" }, { L"[Enter]", L"접속" }, { L"[Esc]", L"종료" } } };

		auto fnValidate = [](const std::vector<Detail::FormField>& vecF, int& iFocus) -> std::wstring
		{
			IN_ADDR tAddr{};
			if (InetPtonW(AF_INET, vecF[0].strValue.c_str(), &tAddr) != 1)	{ iFocus = 0; return L"서버 IP 형식이 올바르지 않습니다. (예: 192.168.0.5)"; }
			if (Parse_Port(vecF[1].strValue) == 0)							{ iFocus = 1; return L"포트는 1024 ~ 65535 사이 숫자입니다."; }
			return L"";
		};

		if (Detail::Run_Form(tText, vecFields, strError, 0, fnValidate) == false) return {};

		std::string strIP;		// 숫자와 '.'만 입력받았으므로 그대로 좁혀도 안전
		for (wchar_t c : vecFields[0].strValue) strIP += static_cast<char>(c);
		return { true, strIP, Parse_Port(vecFields[1].strValue) };
	}

	/* 접속 화면에서 Enter 직후, connect 하기 전에 호출 (실패하면 Windows가 재시도하느라 약 2초 멈춤) */
	inline void Show_Connecting(const std::string& strIP, uint16_t iPort)
	{
		Show_Status(L"⋯ " + Widen(strIP) + L":" + std::to_wstring(iPort) + L" 서버에 접속하는 중");
	}

	/* 접속 연출 : 돌아가는 아이콘 + 진행 바를 dwTotalMs까지 채운 뒤 "접속 완료"
	   connect가 너무 빨리 끝나 화면이 깜빡 넘어가는 걸 막는 용도
	   dwElapsedMs : connect에 이미 걸린 시간 (바가 그만큼 차 있는 상태에서 시작) */
	inline void Play_Connecting(const std::string& strIP, uint16_t iPort, DWORD dwTotalMs = 1500, DWORD dwElapsedMs = 0)
	{
		using namespace Color;
		constexpr int		BAR_W = 20;
		constexpr DWORD		FRAME_MS = 60;
		const wchar_t		szSpin[] = L"⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
		const std::wstring	strText = Widen(strIP) + L":" + std::to_wstring(iPort) + L" 서버에 접속하는 중";

		int iCols{ 0 }, iRows{ 0 };
		Get_ConsoleSize(iCols, iRows);
		Set_CursorVisible(false);

		const ULONGLONG ullStart = GetTickCount64() - dwElapsedMs;
		for (int iFrame = 0; ; ++iFrame)
		{
			const DWORD dwNow = static_cast<DWORD>(GetTickCount64() - ullStart);
			const int iFill = (std::min)(BAR_W, static_cast<int>(BAR_W * dwNow / (std::max)(dwTotalMs, 1ul)));

			/* "⠹ 127.0.0.1:1234 서버에 접속하는 중  ━━━━━━━━━━──────────" */
			const std::wstring strPlain = L"⠋ " + strText + L"  " + std::wstring(BAR_W, L'━');
			std::wstring str = L"\x1b[" + std::to_wstring(Detail::g_iStatusRow) + L";1H\x1b[K" + Center_Pad(strPlain, iCols);
			str += Yellow; str += szSpin[iFrame % 10]; str += L" "; str += strText; str += L"  ";
			str += Red;  str += std::wstring(iFill, L'━');
			str += Gray; str += std::wstring(BAR_W - iFill, L'─'); str += Reset;
			Write(str);

			if (dwNow >= dwTotalMs) break;
			Sleep(FRAME_MS);
		}

		/* 완료 표시 잠깐 */
		const std::wstring strDone = L"✔ " + Widen(strIP) + L":" + std::to_wstring(iPort) + L" 접속 완료";
		Write(L"\x1b[" + std::to_wstring(Detail::g_iStatusRow) + L";1H\x1b[K" + Center_Pad(strDone, iCols) + Green + strDone + Reset);
		Sleep(400);

		FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));	// 연출 중에 누른 키는 버림 (다음 화면으로 새지 않게)
	}

	/* ---------------- 로그인 (닉네임) : 이미 서버에 연결된 상태 ---------------- */
	struct LoginResult
	{
		bool			isOk{ false };		// Esc로 나가면 false
		std::wstring	strNickname;
	};

	/* strServerInfo : 로고 아래에 표시 ("서버 127.0.0.1:1234 · #3")
	   strDefaultNick : 실패 후 다시 띄울 때 이전에 입력한 닉네임 유지용 */
	inline LoginResult Run_Login(const std::wstring& strServerInfo, const std::wstring& strDefaultNick = L"", const std::wstring& strError = L"", size_t iMaxNickLen = 10)
	{
		std::vector<Detail::FormField> vecFields = { { L"닉네임 ", strDefaultNick, Detail::FIELD_KIND::NICK } };
		const Detail::FormText tText{ L" 로 그 인 ", strServerInfo, L"  Enter  로그인  ",
			{ { L"[Enter]", L"로그인" }, { L"[Esc]", L"접속 화면으로" } } };

		auto fnValidate = [](const std::vector<Detail::FormField>& vecF, int& iFocus) -> std::wstring
		{
			if (vecF[0].strValue.empty()) { iFocus = 0; return L"닉네임을 입력하세요."; }
			return L"";		// 글자 종류 / 중복 검사는 서버가 함
		};

		if (Detail::Run_Form(tText, vecFields, strError, iMaxNickLen, fnValidate) == false) return {};
		return { true, vecFields[0].strValue };
	}

	/* ---------------- 로비 (방 목록 선택 + 접속 중) ---------------- */
	struct RoomView
	{
		int				iNo{ 0 };			// 방 번호 (화면에 [ 1 ] 로 표시)
		std::wstring	strName;
		int				iCur{ 0 };
		int				iMax{ 7 };
		std::wstring	strState;			// "대기 중", "게임 중 · 낮 2일차" 등
		bool			isPlaying{ false };	// 게임 중이면 회색으로
	};

	struct LobbyView
	{
		std::wstring				strNickname;	// 내 닉네임 (접속 중 목록에서 "(나)" 표시)
		uint64_t					iMyKey{ 0 };	// 서버가 정해준 내 번호 (0 = 아직 못 받음)
		std::wstring				strServer;		// "127.0.0.1:1234"
		std::vector<std::wstring>	vecMembers;		// 접속 중인 사람 이름
		std::vector<RoomView>		vecRooms;
		int							iSelected{ 0 };	// 선택된 방 인덱스 (vecRooms 기준). ↑↓ 처리는 호출하는 쪽에서
	};

	/* 로비 화면 그리기. 바뀐 게 있을 때만 호출하면 됨 (Need_Redraw()와 같이 사용 가능) */
	inline void Render_Lobby(const LobbyView& tView)
	{
		using namespace Color;
		constexpr int ROOM_W = 64, MEMBER_W = 24, LIST_ROWS = 9;

		/* 상단: 로고 + 오른쪽 환영 문구 */
		const std::wstring strSide[6] = {
			L"",
			std::wstring(Bold) + tView.strNickname + Reset + L" 님, 어서 오세요   "
				+ (tView.iMyKey != 0 ? std::wstring(Yellow) + L"#" + std::to_wstring(tView.iMyKey) + Reset
									 : std::wstring(Gray) + L"번호 받는 중…" + Reset),
			std::wstring(Gray) + L"서버  " + tView.strServer + Reset,
			std::wstring(Green) + L"●" + Reset + Gray + L" 접속 중  " + std::to_wstring(tView.vecMembers.size()) + L"명" + Reset,
			L"", L"" };

		std::wstring str = L"\x1b[H\x1b[K\n";
		for (int i = 0; i < 6; ++i) str += L"  " + Logo_Line(i) + L"      " + strSide[i] + L"\x1b[K\n";
		str += L"  "; str += Gray; str += std::wstring(ROOM_W + MEMBER_W + 6, L'─'); str += Reset; str += L"\x1b[K\n\x1b[K\n";

		/* 박스 제목 줄 */
		auto Top = [&](const std::wstring& strTitle, int iW)
		{
			return std::wstring(Gray) + L"┌─" + Reset + Bold + strTitle + Reset + Gray + std::wstring(iW - 1 - Width(strTitle), L'─') + L"┐" + Reset;
		};
		str += L"  " + Top(L" 방 목록 (" + std::to_wstring(tView.vecRooms.size()) + L") ", ROOM_W) + L"  "
			 + Top(L" 접속 중 (" + std::to_wstring(tView.vecMembers.size()) + L") ", MEMBER_W) + L"\x1b[K\n";

		/* 선택한 방이 보이도록 스크롤 */
		const int iRoomCount = static_cast<int>(tView.vecRooms.size());
		const int iSel = (std::min)((std::max)(tView.iSelected, 0), (std::max)(iRoomCount - 1, 0));
		const int iFirst = (std::max)(0, iSel - (LIST_ROWS - 2));

		for (int iRow = -1; iRow < LIST_ROWS; ++iRow)	// -1 = 머리글 줄
		{
			std::wstring strRoom;
			if (iRow == -1)
				strRoom = std::wstring(Gray) + Fit(L"     번호    방 이름              인원    상태", ROOM_W) + Reset;
			else if (iFirst + iRow < iRoomCount)
			{
				const int iIndex = iFirst + iRow;
				const RoomView& tRoom = tView.vecRooms[iIndex];
				const std::wstring strNo = L"[ " + std::to_wstring(tRoom.iNo) + L" ]";
				const std::wstring strBody = Fit(strNo, 8) + Fit(tRoom.strName, 21) + Fit(std::to_wstring(tRoom.iCur) + L"/" + std::to_wstring(tRoom.iMax), 8) + tRoom.strState;

				if (iIndex == iSel)			strRoom = std::wstring(Detail::SELECT) + L" ▶  " + Fit(strBody, ROOM_W - 4) + Reset;
				else if (tRoom.isPlaying)	strRoom = std::wstring(Gray) + L"    " + Fit(strBody, ROOM_W - 4) + Reset;
				else						strRoom = std::wstring(L"    ") + Yellow + Bold + Fit(strNo, 8) + Reset + Fit(strBody.substr(Fit(strNo, 8).size()), ROOM_W - 12);
			}
			else
				strRoom = Spaces(ROOM_W);

			/* 접속 중 목록: 머리글 줄부터 이름을 채움 */
			std::wstring strMember = Spaces(MEMBER_W);
			const int iMember = iRow + 1;
			if (iMember < static_cast<int>(tView.vecMembers.size()))
			{
				const std::wstring& strName = tView.vecMembers[iMember];
				strMember = (strName == tView.strNickname)
					? std::wstring(L" ") + Bold + strName + Reset + Gray + L" (나)" + Reset + Spaces(MEMBER_W - 1 - Width(strName) - 5)
					: L" " + Fit(strName, MEMBER_W - 1);
			}

			str += L"  "; str += Gray; str += L"│"; str += Reset; str += strRoom;
			str += Gray; str += L"│  │"; str += Reset; str += strMember; str += Gray; str += L"│"; str += Reset; str += L"\x1b[K\n";
		}

		str += L"  "; str += Gray; str += L"└"; str += std::wstring(ROOM_W, L'─'); str += L"┘  └"; str += std::wstring(MEMBER_W, L'─'); str += L"┘"; str += Reset; str += L"\x1b[K\n";
		str += L"  ";
		str += Yellow; str += L"[↑↓]"; str += Reset; str += L" 선택   ";
		str += Yellow; str += L"[Enter]"; str += Reset; str += L" 입장   ";
		str += Yellow; str += L"[C]"; str += Reset; str += L" 방 만들기   ";
		str += Yellow; str += L"[R]"; str += Reset; str += L" 새로고침   ";
		str += Yellow; str += L"[Esc]"; str += Reset; str += L" 나가기";
		str += L"\x1b[K\x1b[J";
		Write(str);
	}

	/* ============================================================
	   서버 화면 : 시작(포트 입력) / 로딩 / 메인(접속자 + 로그)
	   클라이언트 화면과 같은 스타일 (MAFIA 로고 + 박스)
	   ============================================================ */

	/* 서버 화면 공통 상단: 왼쪽 로고 6줄 + 오른쪽 안내 6줄 */
	inline std::wstring Server_Header(const std::wstring (&strSide)[6])
	{
		std::wstring str = L"\x1b[H\x1b[K\n";
		for (int i = 0; i < 6; ++i)
			str += L"  " + Logo_Line(i) + L"      " + strSide[i] + L"\x1b[K\n";
		return str;
	}

	inline std::wstring Separator(int iWidth)
	{
		return std::wstring(L"  ") + Color::Gray + std::wstring(static_cast<size_t>(iWidth), L'─') + Color::Reset + L"\x1b[K\n";
	}

	/* 박스 윗변: ┌─ 제목 ───┐  (안쪽 폭 iW) */
	inline std::wstring Box_Top(const std::wstring& strTitle, int iW)
	{
		using namespace Color;
		return std::wstring(Gray) + L"┌─" + Reset + Bold + strTitle + Reset + Gray
			 + std::wstring(static_cast<size_t>((std::max)(0, iW - 1 - Width(strTitle))), L'─') + L"┐" + Reset;
	}

	/* 시작 화면: IP 안내 + 포트 입력. 올바른 포트가 들어올 때까지 반복 */
	inline uint16_t Run_StartupScreen(uint16_t iDefaultPort, const std::wstring& strError = L"")
	{
		using namespace Color;
		const std::vector<std::wstring> vecIP = Get_LocalIPv4();
		std::wstring strMsg = strError;

		while (true)
		{
			const std::wstring strSide[6] = {
				L"",
				std::wstring(Bold) + L"SERVER" + Reset + Gray + L"   7인 마피아 게임 서버" + Reset,
				std::wstring(Gray) + L"서버를 열 포트 번호를 입력해 주세요" + Reset,
				L"", L"", L"" };

			std::wstring str = Server_Header(strSide);
			str += Separator(60); str += L"\x1b[K\n";

			str += L"  "; str += Bold; str += L"이 PC의 IP 주소"; str += Reset;
			str += Gray; str += L"  (클라이언트에서 이 주소로 접속)"; str += Reset; str += L"\x1b[K\n";
			for (const std::wstring& strIP : vecIP)
			{
				str += L"    "; str += Green; str += L"● "; str += Reset; str += strIP; str += L"\x1b[K\n";
			}
			str += L"    "; str += Green; str += L"● "; str += Reset; str += L"127.0.0.1";
			str += Gray; str += L"  (같은 PC에서 테스트)"; str += Reset; str += L"\x1b[K\n\x1b[K\n";

			str += L"  "; str += Bold; str += L"사용할 포트 번호"; str += Reset;
			str += Gray; str += L"  (1024 ~ 65535, Enter = "; str += std::to_wstring(iDefaultPort); str += L")"; str += Reset; str += L"\x1b[K\n";
			if (!strMsg.empty())
			{
				str += L"  "; str += Red; str += L"✖ "; str += strMsg; str += Reset; str += L"\x1b[K\n";
			}
			str += L"  "; str += Yellow; str += L"> "; str += Reset; str += L"\x1b[J";

			Set_CursorVisible(true);
			Clear();
			Write(str);

			const std::wstring strInput = Read_Line();
			if (strInput.empty()) return iDefaultPort;

			if (const uint16_t iPort = Parse_Port(strInput); iPort != 0)
				return iPort;

			strMsg = L"'" + strInput + L"' 은(는) 사용할 수 없습니다. 1024 ~ 65535 사이 숫자를 입력하세요.";
		}
	}

	/* 서버 준비 중 로딩 (진행 바, dwMs 동안) */
	inline void Run_Loading(uint16_t iPort, DWORD dwMs)
	{
		using namespace Color;
		constexpr int	iBarLen = 40;
		constexpr DWORD	dwStep = 50;
		const wchar_t	szSpin[] = L"|/-\\";

		Set_CursorVisible(false);
		Clear();
		for (DWORD dwTime = 0; dwTime <= dwMs; dwTime += dwStep)
		{
			const int iFill = static_cast<int>(iBarLen * dwTime / dwMs);
			const int iPercent = static_cast<int>(100 * dwTime / dwMs);

			const std::wstring strSide[6] = {
				L"",
				std::wstring(Bold) + L"SERVER" + Reset + Gray + L"   포트 " + std::to_wstring(iPort) + Reset,
				std::wstring(Yellow) + szSpin[(dwTime / dwStep) % 4] + Reset + L" 서버 준비 중",
				L"", L"", L"" };

			std::wstring str = Server_Header(strSide);
			str += Separator(60); str += L"\x1b[K\n";
			str += L"  "; str += Red; str += std::wstring(static_cast<size_t>(iFill), L'━'); str += Gray; str += std::wstring(static_cast<size_t>(iBarLen - iFill), L'─'); str += Reset;
			str += L"  "; str += std::to_wstring(iPercent); str += L"%"; str += (iFill >= iBarLen ? L"  🔪" : L""); str += L"\x1b[K\x1b[J";
			Write(str);
			Sleep(dwStep);
		}
	}

	/* 표 형태 칸 폭: 접속자 박스 / 로그 박스 공통 */
	namespace Detail
	{
		constexpr int COL_TIME = 11, COL_NO = 5, COL_IP = 16, COL_PORT = 7;
	}

	/* 로그 한 줄 그리기 (폭 iWidth)
	   "시간\t번호\tIP\t포트\t내용" 처럼 탭으로 나뉜 줄은 열 정렬 + 색, 탭이 없으면 그대로 */
	inline std::wstring Log_Cell(const std::wstring& strLine, int iWidth)
	{
		using namespace Color;
		std::vector<std::wstring> vecCols;
		size_t iStart = 0, iTab = 0;
		while ((iTab = strLine.find(L'\t', iStart)) != std::wstring::npos) { vecCols.push_back(strLine.substr(iStart, iTab - iStart)); iStart = iTab + 1; }
		vecCols.push_back(strLine.substr(iStart));
		if (vecCols.size() != 5) return Fit(strLine, iWidth);

		const int iRest = (std::max)(0, iWidth - Detail::COL_TIME - Detail::COL_NO - Detail::COL_IP - Detail::COL_PORT);
		return std::wstring(Gray) + Fit(vecCols[0], Detail::COL_TIME) + Reset
			 + Yellow + Fit(vecCols[1], Detail::COL_NO) + Reset
			 + Fit(vecCols[2], Detail::COL_IP)
			 + Cyan + Fit(vecCols[3], Detail::COL_PORT) + Reset
			 + Fit(vecCols[4], iRest);
	}

	/* 서버 메인 화면: 상단 로고 + 상태 / 아래 [접속자 | 로그] 두 칸 박스 */
	/* 실제로 그리는 함수: 접속자 수 + "i번째 접속자의 표시 정보" 함수 (화면에 보이는 줄만 호출됨)
	   fnGetLogs : "최근 n줄의 로그 문장"을 돌려주는 함수. 비어 있으면 ConsoleUI::Add_Log로 쌓은 로그를 사용 */
	inline void Render_ServerMain(uint16_t iPort, bool isAccepting, size_t iClientCount, const std::function<ClientView(size_t)>& fnGetClient,
								  const std::function<std::vector<std::wstring>(size_t)>& fnGetLogs = nullptr)
	{
		using namespace Color;
		int iCols{ 0 }, iRows{ 0 };
		Get_ConsoleSize(iCols, iRows);

		constexpr int	LEFT_W = 28;
		const int		RIGHT_W = (std::max)(20, iCols - LEFT_W - 10);	// 여백 2 + 테두리 4 + 박스 사이 2 + 오른쪽 여유 2
		const int		iBodyRows = (std::max)(3, iRows - 14);			// 위 9줄 + 박스 위아래 2줄 + 안내 1줄 + 여유 2줄

		/* 상단 */
		const std::wstring strSide[6] = {
			L"",
			std::wstring(Bold) + L"SERVER" + Reset + Gray + L"   포트 " + std::to_wstring(iPort) + L"에서 실행 중" + Reset,
			isAccepting
				? std::wstring(Green) + L"●" + Reset + L" 접속 요청  " + Green + Bold + L"ON " + Reset + Gray + L"  새 접속을 받는 중" + Reset
				: std::wstring(Red) + L"○" + Reset + L" 접속 요청  " + Red + Bold + L"OFF" + Reset + Gray + L"  새 접속을 받지 않음" + Reset,
			std::wstring(Gray) + L"접속자 " + std::to_wstring(iClientCount) + L"명" + Reset,
			L"", L"" };

		std::wstring str = Server_Header(strSide);
		str += Separator(LEFT_W + RIGHT_W + 6); str += L"\x1b[K\n";

		/* 박스 윗변 */
		str += L"  " + Box_Top(L" 접속자 (" + std::to_wstring(iClientCount) + L") ", LEFT_W) + L"  " + Box_Top(L" 로그 ", RIGHT_W) + L"\x1b[K\n";

		/* 최신 로그가 아래로 오도록 마지막 iBodyRows개 */
		std::vector<std::wstring> vecLogs;
		if (fnGetLogs)
			vecLogs = fnGetLogs(static_cast<size_t>(iBodyRows - 1));	// 첫 줄은 머리글
		else
		{
			std::lock_guard<std::mutex> lock(Detail::g_LogLock);
			const size_t iCount = (std::min)(Detail::g_Logs.size(), static_cast<size_t>(iBodyRows - 1));
			vecLogs.assign(Detail::g_Logs.end() - iCount, Detail::g_Logs.end());
		}

		for (int iRow = 0; iRow < iBodyRows; ++iRow)
		{
			std::wstring strLeft = Spaces(LEFT_W), strRight;
			if (iRow == 0)	/* 머리글 */
			{
				strLeft = std::wstring(Gray) + L" " + Fit(L"번호", Detail::COL_NO) + Fit(L"IP", Detail::COL_IP) + Fit(L"포트", LEFT_W - 1 - Detail::COL_NO - Detail::COL_IP) + Reset;
				strRight = std::wstring(Gray) + L" " + (fnGetLogs
					? Fit(L"시간", Detail::COL_TIME) + Fit(L"번호", Detail::COL_NO) + Fit(L"IP", Detail::COL_IP) + Fit(L"포트", Detail::COL_PORT) + Fit(L"내용", RIGHT_W - 1 - Detail::COL_TIME - Detail::COL_NO - Detail::COL_IP - Detail::COL_PORT)
					: Spaces(RIGHT_W - 1)) + Reset;
			}
			else
			{
				const int iIndex = iRow - 1;
				if (iIndex < static_cast<int>(iClientCount))
				{
					const ClientView tClient = fnGetClient(static_cast<size_t>(iIndex));
					strLeft = std::wstring(L" ") + Yellow + Fit(L"#" + std::to_wstring(tClient.iKey), Detail::COL_NO) + Reset
							+ Fit(Widen(tClient.strIP), Detail::COL_IP)
							+ Cyan + Fit(std::to_wstring(tClient.iPort), LEFT_W - 1 - Detail::COL_NO - Detail::COL_IP) + Reset;
				}
				strRight = L" " + Log_Cell(iIndex < static_cast<int>(vecLogs.size()) ? vecLogs[iIndex] : L"", RIGHT_W - 1);
			}

			str += L"  "; str += Gray; str += L"│"; str += Reset; str += strLeft;
			str += Gray; str += L"│  │"; str += Reset; str += strRight; str += Gray; str += L"│"; str += Reset; str += L"\x1b[K\n";
		}

		/* 박스 아랫변 + 안내 */
		str += L"  "; str += Gray; str += L"└"; str += std::wstring(LEFT_W, L'─'); str += L"┘  └"; str += std::wstring(static_cast<size_t>(RIGHT_W), L'─'); str += L"┘"; str += Reset; str += L"\x1b[K\n";
		str += L"  "; str += Yellow; str += L"[F5]"; str += Reset; str += L" 접속 요청 On / Off";
		str += L"\x1b[K\x1b[J";
		Write(str);
	}

	/* ClientView 목록을 그대로 넘기는 버전 */
	inline void Render_ServerMain(uint16_t iPort, bool isAccepting, const std::vector<ClientView>& vecClients = {})
	{
		Render_ServerMain(iPort, isAccepting, vecClients.size(), [&](size_t i) { return vecClients[i]; });
	}

	/* 키 목록 + "키 → 표시 정보" 함수를 넘기는 버전 (ClientView vector를 따로 만들 필요 없음)
	   예) Render_ServerMain(port, accept, server->Get_ClientOrder(), [&](uint64_t iKey) { ... return ClientView{ iKey, ip, port }; }); */
	template<class FnGetView>
	inline void Render_ServerMain(uint16_t iPort, bool isAccepting, const std::vector<uint64_t>& vecKeys, FnGetView fnGetView)
	{
		Render_ServerMain(iPort, isAccepting, vecKeys.size(), [&](size_t i) { return ClientView(fnGetView(vecKeys[i])); });
	}
}
