#pragma once

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")

#include <cstdint>
#include <functional>
#include <array>


#include "Funtion.h"
#include "Enum.h"
#include "Struct.h"
#include "CSocket.h"
#include "CAddress.h"
#include "CEventQueue.h"
#include "CClient.h"
#include "CServer.h"




#ifdef _DEBUG
#define NET_LOG(fmt, ...) printf("[Net] " fmt "\n", __VA_ARGS__)
#else
#define NET_LOG(fmt, ...) ((void)0)
#endif