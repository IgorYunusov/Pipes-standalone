#include "pch.h"
#include "Server.h"

using namespace std;

namespace Pipes
{
	BOOL						Server::Connected{};
	DWORD						Server::ThreadId{};
	HANDLE						Server::Pipe INVALID_HANDLE_VALUE, Server::HThread = nullptr;
	LPCTSTR						Server::PipeName = TEXT("\\\\.\\pipe\\myPipe");

	std::mutex					Server::Mtx;
	std::condition_variable		Server::Cv;
	string						Server::Request;
	string						Server::Response;

	HANDLE Server::Init()
	{
		Pipe = CreateNamedPipe(
			PipeName,						// pipe name 
			PIPE_ACCESS_DUPLEX,				// read/write access 
			PIPE_TYPE_MESSAGE |				// message type pipe 
			PIPE_READMODE_MESSAGE |			// message-read mode 
			PIPE_WAIT,						// blocking mode 
			PIPE_UNLIMITED_INSTANCES,		// max. instances  
			BUFFER_SIZE,					// output buffer size 
			BUFFER_SIZE,					// input buffer size 
			0,								// client time-out 
			nullptr);						// default security attribute 

		if (Pipe == INVALID_HANDLE_VALUE)
		{
			ErrorMsgBox::Show(L"Invalid Handle");
		}

		// Wait for the client to connect (nonzero return)
		Connected = ConnectNamedPipe(Pipe, nullptr) ?
			TRUE : GetLastError() == ERROR_PIPE_CONNECTED;

		if (Connected)
		{
			// Create a thread for this client. 
			HThread = CreateThread(
				nullptr,				// no security attribute 
				0,						// default stack size 
				InstanceThread,			// thread proc
				LPVOID(Pipe),			// thread parameter 
				0,						// not suspended 
				&ThreadId);				// returns thread ID 

			if (HThread == nullptr)
			{
				ErrorMsgBox::Show(L"CreateThread Failed");
			}
			else return HThread;
		}
		else
			Close();
		
		return nullptr;
	}

	void Server::Close()
	{
		CloseHandle(Pipe);
	}

	DWORD WINAPI InstanceThread(LPVOID lpvParam)
	{
		auto* const hHeap = GetProcessHeap();
		auto* const pchRequest = static_cast<TCHAR*>(HeapAlloc(hHeap, 0, BUFFER_SIZE * sizeof(TCHAR)));
		auto* const pchReply = static_cast<TCHAR*>(HeapAlloc(hHeap, 0, BUFFER_SIZE * sizeof(TCHAR)));

		DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;

		// Do some extra error checking since the app will keep running even if this
		// thread fails.

		if (lpvParam == nullptr)
		{
			ErrorMsgBox::Show(L"lpvParam null");
			if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
			if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
			return DWORD(-1);
		}

		if (pchRequest == nullptr)
		{
			ErrorMsgBox::Show(L"Request null");
			if (pchReply != nullptr) HeapFree(hHeap, 0, pchReply);
			return DWORD(-1);
		}

		if (pchReply == nullptr)
		{
			ErrorMsgBox::Show(L"Reply null");
			if (pchRequest != nullptr) HeapFree(hHeap, 0, pchRequest);
			return DWORD(-1);
		}

		// The thread's parameter is a handle to a pipe object instance. 
		auto* const hPipe = HANDLE(lpvParam);

		auto exitCode = 0;

		// Loop until done reading
		while (true)
		{
			// Read client requests from the pipe. This simplistic code only allows messages
			// up to BUFFER_SIZE characters in length.
			auto fSuccess = ReadFile(
				hPipe, // handle to pipe 
				pchRequest, // buffer to receive data 
				BUFFER_SIZE * sizeof(TCHAR), // size of buffer 
				&cbBytesRead, // number of bytes read 
				nullptr);					// not overlapped I/O

			if (fSuccess)
			{
				const auto tmp = wstring(pchRequest);
				Server::Request = string(tmp.begin(), tmp.end());

				std::unique_lock<std::mutex> lock(Server::Mtx);
				Server::Cv.wait(lock, [] { return !Server::Response.empty(); });
			}

			if (!fSuccess || cbBytesRead == 0)
			{
				// Client disconnected
				if (GetLastError() == ERROR_BROKEN_PIPE)
				{
					exitCode = Server::Request == "@close" ? 0 : 1;
					break;
				}

				ErrorMsgBox::Show(L"ReadFile Failed");

				break;
			}

			// Process the incoming message.
			GetAnswerToRequest(pchRequest, pchReply, &cbReplyBytes);

			// Write the reply to the pipe. 
			fSuccess = WriteFile(
				hPipe,			// handle to pipe 
				pchReply,		// buffer to write from 
				cbReplyBytes,	// number of bytes to write 
				&cbWritten,		// number of bytes written 
				nullptr);       // not overlapped I/O 

			if (!fSuccess || cbReplyBytes != cbWritten)
			{
				ErrorMsgBox::Show(L"WriteFile failed");
				break;
			}

			if (Server::Response == "#closing")
			{
				exitCode = 0;
				break;
			}
		}

		// Flush the pipe to allow the client to read the pipe's contents 
		FlushFileBuffers(hPipe);
		DisconnectNamedPipe(hPipe);
		Server::Close();

		HeapFree(hHeap, 0, pchRequest);
		HeapFree(hHeap, 0, pchReply);

		Server::Connected = false;
		return exitCode;
	}

	VOID GetAnswerToRequest(const wchar_t* pchRequest, LPTSTR pchReply, LPDWORD pchBytes)
	{
		const auto response = wstring(Server::Response.begin(), Server::Response.end());

		if(response != L"#closing")
			Server::Response = "";

		// Check the outgoing message to make sure it's not too long for the buffer.
		if (FAILED(StringCchCopy(pchReply, BUFFER_SIZE, response.c_str())))
		{
			*pchBytes = 0;
			pchReply[0] = 0;
			ErrorMsgBox::Show(L"StringCchCopy failed no msg");
			return;
		}
		*pchBytes = (lstrlen(pchReply) + 1) * sizeof(TCHAR);
	}
}
