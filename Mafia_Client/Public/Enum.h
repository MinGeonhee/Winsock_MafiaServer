#pragma once


namespace MafiaClient
{
	constexpr size_t iLobbyListUpdateSecond = 10;


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

