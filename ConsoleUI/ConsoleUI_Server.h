#pragma once
/* ============================================================
   ConsoleUI_Server : Net::CServer 하나만 넘기면 서버 화면을 그려 주는 편의 함수
   - ConsoleUI → NetLib 의존은 이 파일에만 둠 (ConsoleUI.h 자체는 NetLib을 모름)
   - 필요한 CServer 함수: Get_PortNum() const, Is_Accept() const,
                          Get_ClientKeys() const, Get_Client(ClientKey) const → const CClient*,
                          Get_Logs() const → 순회 가능한 NetLog 컨테이너 (std::deque 등. std::queue는 순회가 안 됨)
   ============================================================ */
#include "Net_Header.h"
#include "ConsoleUI.h"

namespace ConsoleUI
{
	/* NetLog 한 개 → "시간\t번호\tIP\t포트\t내용" (ConsoleUI가 열 정렬 + 색을 입혀 그림)
	   예) [04:18:27] | #3 | 127.0.0.1 | 50211 | 접속 */
	inline std::wstring Format_NetLog(const Net::NetLog& tLog)
	{
		wchar_t szTime[16]{};
		swprintf_s(szTime, L"[%02d:%02d:%02d]", tLog.tTime.wHour, tLog.tTime.wMinute, tLog.tTime.wSecond);

		const std::wstring strNo = (tLog.iKey != Net::NONE_CLIENTKEY) ? L"#" + std::to_wstring(tLog.iKey) : L"-";
		const wchar_t* szText = L"";
		switch (tLog.eType)
		{
		case Net::NET_LOG_TYPE::Connect:	szText = L"접속"; break;
		case Net::NET_LOG_TYPE::Disconnect:	szText = L"접속 종료"; break;
		case Net::NET_LOG_TYPE::Reject:		szText = L"접속 거절 (요청 OFF)"; break;
		default: break;
		}
		return std::wstring(szTime) + L"\t" + strNo + L"\t" + Widen(tLog.strIP) + L"\t" + std::to_wstring(tLog.iPortNum) + L"\t" + szText;
	}

	/* 창 크기가 바뀌었거나 서버에 새 로그가 생겼으면 true
	   (로그 개수는 상한에서 멈출 수 있으므로 "개수 + 마지막 로그의 시각·키"로 변화를 판단) */
	inline bool Need_Redraw(const Net::CServer& Server)
	{
		static size_t			s_iLastCount{ 0 };
		static SYSTEMTIME		s_tLastTime{};
		static Net::ClientKey	s_iLastKey{ Net::NONE_CLIENTKEY };

		const auto& Logs = Server.Get_Logs();
		bool isNewLog = false;
		if (!Logs.empty())
		{
			const Net::NetLog& tBack = Logs.back();
			isNewLog = Logs.size() != s_iLastCount || tBack.iKey != s_iLastKey
					|| memcmp(&tBack.tTime, &s_tLastTime, sizeof(SYSTEMTIME)) != 0;
			s_iLastCount = Logs.size(); s_tLastTime = tBack.tTime; s_iLastKey = tBack.iKey;
		}
		const bool isResized = Need_Redraw();	// ConsoleUI.h의 창 크기 확인 (같이 불러야 크기 기록이 갱신됨)
		return isNewLog || isResized;
	}

	inline void Render_ServerMain(const Net::CServer& Server)	// 읽기만 함 (서버 상태를 바꾸지 않음)
	{
		Render_ServerMain(Server.Get_PortNum(), Server.Is_Accept(), Server.Get_ClientKeys().size(),
			/* i번째 접속자 */
			[&](size_t i)
			{
				const Net::ClientKey iKey = Server.Get_ClientKeys()[i];
				const Net::CClient* pClient = Server.Get_Client(iKey);
				return pClient ? ClientView{ iKey, pClient->Get_IP(), pClient->Get_PortNum() }
							   : ClientView{ iKey };
			},
			/* 최근 로그 n줄 (오래된 것 → 최신 순) */
			[&](size_t iMaxRows)
			{
				const auto& Logs = Server.Get_Logs();
				const size_t iCount = (std::min)(Logs.size(), iMaxRows);
				std::vector<std::wstring> vecLines;
				vecLines.reserve(iCount);
				for (auto iter = Logs.end() - static_cast<std::ptrdiff_t>(iCount); iter != Logs.end(); ++iter)
					vecLines.push_back(Format_NetLog(*iter));
				return vecLines;
			});
	}
}
