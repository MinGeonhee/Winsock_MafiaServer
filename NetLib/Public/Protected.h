#pragma once
/* 메모리 변조 탐지용 값 (클라에서 HP · 쿨타임 같은 기초 타입에 사용)
   - 저장 : 값 XOR 랜덤 키 → 메모리에는 원래 값이 그대로 남지 않음 (치트엔진 "100 검색 → 90 검색"이 안 통함)
   - 저장할 때마다 키를 새로 뽑음 → 같은 값이어도 메모리 모양이 매번 다름
   - 체크값 : 원래 값 + 키로 만든 값을 따로 저장 → 누가 한쪽만 바꾸면 꺼낼 때 안 맞아서 탐지
   - 한계 : 꺼낸 순간(Get)엔 레지스터 · 스택에 원래 값이 잠깐 남고, Get 자체를 후킹하면 못 막음
            → 진짜 중요한 값(재화 · 판정)은 서버가 가짐. 이건 "값 검색으로 바꾸기" 수준을 막는 용도 */
#include <cstdint>
#include <cstring>
#include <random>
#include <type_traits>
#include <functional>

namespace Net
{
	template <typename T>
	class Protected
	{
		static_assert(std::is_trivially_copyable_v<T>, "기초 타입(int · float 등)만 : 포인터 · 소멸자가 있는 객체를 XOR하면 깨짐");
		static_assert(sizeof(T) <= sizeof(uint64_t), "8바이트 이하만");

	public:
		/* 변조를 찾았을 때 부를 함수 (로그 · 서버 보고 등은 쓰는 쪽이 정함) */
		static inline std::function<void()> s_fnOnTampered{};

		Protected(T value = T{}) { Set(value); }

		void Set(T value)
		{
			uint64_t iRaw = 0;
			std::memcpy(&iRaw, &value, sizeof(T));		// 비트 그대로 (float도 XOR 가능하게)

			m_iKey		= Next_Key();
			m_iEncoded	= iRaw ^ m_iKey;				// XOR : 같은 키로 한 번 더 XOR하면 원래대로 (a ^ k ^ k == a)
			m_iCheck	= Mix(iRaw, m_iKey);
		}

		T Get() const
		{
			const uint64_t iRaw = m_iEncoded ^ m_iKey;	// 복구
			if (Mix(iRaw, m_iKey) != m_iCheck && s_fnOnTampered) s_fnOnTampered();

			T value;
			std::memcpy(&value, &iRaw, sizeof(T));
			return value;
		}

		/* int처럼 쓰기 : hp = hp - 10, if (hp > 0) … */
		operator T() const					{ return Get(); }
		Protected& operator=(T value)		{ Set(value); return *this; }
		Protected& operator+=(T value)		{ Set(Get() + value); return *this; }
		Protected& operator-=(T value)		{ Set(Get() - value); return *this; }

	private:
		static uint64_t Next_Key()
		{
			static thread_local std::mt19937_64 s_Random{ std::random_device{}() };
			return s_Random() | 1;						// 0이 나오면 XOR해도 그대로라 피함
		}
		static uint64_t Mix(uint64_t iRaw, uint64_t iKey)
		{
			return (iRaw + iKey) * 0x9E3779B97F4A7C15ull;	// 값과 키를 섞은 체크값 (곱하는 수 = 황금비 상수, 비트를 고르게 흩음)
		}

		uint64_t	m_iKey{ 0 };
		uint64_t	m_iEncoded{ 0 };
		uint64_t	m_iCheck{ 0 };
	};
}
