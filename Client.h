#pragma once
#include "pch.h"

using namespace std;

namespace Pipes
{
	class Client
	{
		static HANDLE		Pipe;
		static TCHAR		Buffer[BUFFER_SIZE];
		static BOOL			Success;
		static DWORD		CbRead, Mode;
		static LPCTSTR		PipeName;
	public:
		static BOOL			Connected;
		
		static void Init();
		static std::string SendReceive(wchar_t* message);
		static void Close();
	};
}