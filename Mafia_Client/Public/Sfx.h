#pragma once
/* 효과음 : wav 파일 없이 처음 쓸 때 메모리에서 소리를 만들어 재생 (exe만 있으면 됨)
   PlaySound는 한 번에 한 소리만 나므로, 새 소리가 나오면 앞 소리는 끊김 (효과음이 짧아서 문제없음) */
#include <Windows.h>
#include <mmsystem.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>
#pragma comment(lib, "winmm.lib")

namespace Sfx
{
	enum class ID { NIGHT, DAY, VOTE_START, VOTE_TICK, GUN, TING, HIT, THUD, FLASH, WIN_MAFIA, WIN_CITIZEN, BLIP, MOVE, COUNT, COUNT_LAST, END };

	namespace Detail
	{
		constexpr int RATE = 22050;
		struct Note { float fHz; int iMs; float fVol; bool isNoise; };	/* fHz 0 = 쉼표 */

		inline std::vector<uint8_t> Make(std::initializer_list<Note> notes)
		{
			std::vector<uint8_t> vecPcm;
			uint32_t iNoise = 0x1234567u;
			for (const Note& n : notes)
			{
				const int iCount = RATE * n.iMs / 1000;
				for (int i = 0; i < iCount; ++i)
				{
					const float fFade = 1.f - static_cast<float>(i) / iCount;	/* 끝으로 갈수록 작아짐 */
					float fWave = 0.f;
					if (n.isNoise)		{ iNoise = iNoise * 1664525u + 1013904223u; fWave = ((iNoise >> 24) / 127.5f) - 1.f; }
					else if (n.fHz > 0)	{ fWave = (std::fmod(i * n.fHz / RATE, 1.f) < 0.5f) ? 1.f : -1.f; }		/* 사각파 (8비트 느낌) */
					vecPcm.push_back(static_cast<uint8_t>(128.f + fWave * n.fVol * fFade * 110.f));
				}
			}

			/* WAV 머리 44바이트 + 8비트 모노 PCM */
			const uint32_t iData = static_cast<uint32_t>(vecPcm.size()), iRiff = 36 + iData, iFmt = 16, iRate = RATE;
			const uint16_t iPcm = 1, iCh = 1, iAlign = 1, iBits = 8;
			std::vector<uint8_t> vecWav(44 + iData);
			uint8_t* p = vecWav.data();
			auto Put = [&](const void* src, size_t len) { memcpy(p, src, len); p += len; };
			Put("RIFF", 4); Put(&iRiff, 4); Put("WAVEfmt ", 8); Put(&iFmt, 4); Put(&iPcm, 2); Put(&iCh, 2);
			Put(&iRate, 4); Put(&iRate, 4); Put(&iAlign, 2); Put(&iBits, 2); Put("data", 4); Put(&iData, 4);
			Put(vecPcm.data(), iData);
			return vecWav;
		}

		inline const std::vector<uint8_t>& Get(ID eId)
		{
			static const std::array<std::vector<uint8_t>, static_cast<size_t>(ID::END)> SOUNDS = {
				Make({ { 330.f, 260, .7f, false }, { 247.f, 420, .7f, false } }),													/* NIGHT : 낮게 떨어지는 종 */
				Make({ { 523.f, 90, .6f, false }, { 659.f, 90, .6f, false }, { 784.f, 180, .6f, false } }),							/* DAY : 올라가는 세 음 */
				Make({ { 660.f, 90, .6f, false }, { 0.f, 40, 0.f, false }, { 660.f, 130, .6f, false } }),							/* VOTE_START */
				Make({ { 988.f, 120, .55f, false } }),																				/* VOTE_TICK : 누가 표를 넣을 때 */
				Make({ { 0.f, 170, 1.f, true } }),																					/* GUN */
				Make({ { 2093.f, 60, .6f, false }, { 2637.f, 170, .5f, false } }),													/* TING : 경호원이 막음 */
				Make({ { 0.f, 70, .9f, true }, { 110.f, 170, .9f, false } }),														/* HIT : 쓰러짐, 군인 버팀 */
				Make({ { 90.f, 160, 1.f, false } }),																				/* THUD : 도장 */
				Make({ { 0.f, 45, .6f, true }, { 0.f, 40, 0.f, false }, { 0.f, 45, .6f, true } }),									/* FLASH : 카메라 */
				Make({ { 392.f, 170, .7f, false }, { 311.f, 170, .7f, false }, { 262.f, 170, .7f, false }, { 196.f, 420, .7f, false } }),	/* WIN_MAFIA */
				Make({ { 523.f, 110, .6f, false }, { 659.f, 110, .6f, false }, { 784.f, 110, .6f, false }, { 1047.f, 380, .6f, false } }),	/* WIN_CITIZEN */
				Make({ { 1319.f, 45, .5f, false }, { 1760.f, 70, .5f, false } }),													/* BLIP : 결과, 시간 조절 알림 */
				Make({ { 988.f, 45, .7f, false }, { 1319.f, 75, .7f, false } }),																				/* MOVE : 대상 고르기 */
				Make({ { 880.f, 120, .22f, false } }),																				/* COUNT : 10초부터 매초 약하게 */
				Make({ { 1175.f, 90, .28f, false }, { 0.f, 50, 0.f, false }, { 1175.f, 90, .28f, false } }),						/* COUNT_LAST : 마지막 3초는 삐삐 */
			};
			return SOUNDS[static_cast<size_t>(eId)];
		}
	}

	/* 소리를 미리 만들어 두면 첫 재생 때 멈칫하지 않음 */
	inline void Preload() { for (int i = 0; i < static_cast<int>(ID::END); ++i) Detail::Get(static_cast<ID>(i)); }

	inline void Play(ID eId)
	{
		PlaySoundA(reinterpret_cast<LPCSTR>(Detail::Get(eId).data()), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
	}
}
