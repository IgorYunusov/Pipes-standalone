#pragma once
#include "pch.h"

namespace Pipes
{	
	DWORD WINAPI InstanceThread(LPVOID);
	VOID GetAnswerToRequest(const wchar_t*, LPTSTR, LPDWORD);
	
	class Server
	{
		static DWORD					ThreadId;
		static HANDLE					Pipe, HThread;
		static LPCTSTR					PipeName;
	public:
		static std::mutex				Mtx;
		static std::condition_variable	Cv;
		static std::string				Request;
		static std::string				Response;
		static BOOL						Connected;
		
		static HANDLE Init();
		static void Close();
	};
}
