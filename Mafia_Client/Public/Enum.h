#pragma once


namespace MafiaClient
{
	constexpr size_t iLobbyListUpdateSecond = 10;

	/* 단계가 바뀔 때 채팅창에 한 줄 : 단계마다 다른 문구 (화면용 이름표, 규칙 X) */
	inline const wchar_t* Phase_Message(Protocol::GAME_PHASE ePhase)	/* 헤더에 몸통이 있으니 inline (없으면 cpp마다 중복 정의 → 링크 에러) */
	{
		switch (ePhase)
		{
		case Protocol::GAME_PHASE::NIGHT:           return L"밤이 되었습니다";
		case Protocol::GAME_PHASE::LOADING_DAY:     return L"아침이 밝아옵니다";
		case Protocol::GAME_PHASE::DAY:             return L"토론 시간입니다";
		case Protocol::GAME_PHASE::VOTE:            return L"투표를 시작합니다";
		case Protocol::GAME_PHASE::ALLOCUTION:      return L"최후의 변론 시간입니다";
		case Protocol::GAME_PHASE::LAST_VOTE:       return L"찬반 투표를 시작합니다";
		case Protocol::GAME_PHASE::LOADING_NIGHT:   return L"밤이 찾아옵니다";
		default:                                    return L"";
		}
	}


	enum class SCENE
	{
		INTRO = 0,
		CONNECT,		/* 서버 IP / 포트 입력 → 접속 */
		LOGIN,			/* 연결된 서버에 닉네임으로 로그인 */
		LOBBY,
		ROOM,
		GAME,
		END
	};
}

