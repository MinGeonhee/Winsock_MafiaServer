// 보안 테스트 : 우리 클라 프로세스 메모리에서 "직업 7개 배열"을 찾음 (치트엔진의 값 검색과 같은 방식)
// 직업 enum 은 uint16 (1 마피아 … 7 배신자), 7인 판이면 1 ~ 7 이 한 번씩 → 연속한 uint16 7개가 1~7 순열인 곳을 찾음
#include <windows.h>
#include <cstdio>
#include <vector>
int wmain(int argc, wchar_t** argv)
{
	const DWORD pid = wcstoul(argv[1], nullptr, 10);
	HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (!h) { printf("OpenProcess 실패 %lu\n", GetLastError()); return 1; }
	static const char* JOB[] = { "?", "마피아", "대통령", "탐정", "경호원", "영매", "군인", "배신자" };
	MEMORY_BASIC_INFORMATION mbi{};
	int iFound = 0;
	for (BYTE* p = nullptr; VirtualQueryEx(h, p, &mbi, sizeof(mbi)) == sizeof(mbi); p += mbi.RegionSize)
	{
		if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) || !(mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_WRITECOPY))) continue;
		std::vector<BYTE> buf(mbi.RegionSize);
		SIZE_T rd = 0;
		if (!ReadProcessMemory(h, mbi.BaseAddress, buf.data(), buf.size(), &rd)) continue;
		for (SIZE_T i = 0; i + 14 <= rd; i += 2)
		{
			const uint16_t* v = reinterpret_cast<const uint16_t*>(&buf[i]);
			int seen = 0; bool ok = true;
			for (int k = 0; k < 7; ++k) { if (v[k] < 1 || v[k] > 7 || (seen >> v[k] & 1)) { ok = false; break; } seen |= 1 << v[k]; }
			if (!ok) continue;
			bool isTable = true; for (int k = 0; k < 7; ++k) if (v[k] != k + 1) isTable = false;
			if (isTable) continue;			// 1 2 3 … 7 순서 그대로 = enum 상수 표 (오탐)
			printf("[%p] ", static_cast<BYTE*>(mbi.BaseAddress) + i);
			for (int k = 0; k < 7; ++k) printf("%d번=%s ", k + 1, JOB[v[k]]);
			printf("\n");
			if (++iFound >= 5) { CloseHandle(h); return 0; }
		}
	}
	if (!iFound) printf("직업 배열을 찾지 못함\n");
	CloseHandle(h);
}
