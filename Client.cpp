#include "pch.h"
#include "Client.h"

namespace Pipes
{
	HANDLE		Client::Pipe{};
	TCHAR		Client::Buffer[BUFFER_SIZE]{};
	BOOL		Client::Success = FALSE;
	DWORD		Client::CbRead{}, Client::Mode{};
	LPCTSTR		Client::PipeName = TEXT("\\\\.\\pipe\\myPipe");
	BOOL		Client::Connected;
	
	void Client::Init()
	{
		const auto MAX_ATTEMPTS = 1000;
		
		auto connectAttempts = 0;
		
		while (true) 
		{
			Pipe = CreateFile(
				PipeName,			// pipe name 
				GENERIC_READ |		// read and write access 
				GENERIC_WRITE,
				0,					// no sharing 
				nullptr,			// default security attributes
				OPEN_EXISTING,		// opens existing pipe 
				0,					// default attributes 
				nullptr);           // no template file 

			// Break if the pipe handle is valid. 
			if (Pipe != INVALID_HANDLE_VALUE)
			{
				Connected = true;
				break;
			}

			// Exit if an error other than ERROR_PIPE_BUSY occurs. 
			if (GetLastError() != ERROR_PIPE_BUSY)
			{
				if (connectAttempts <= MAX_ATTEMPTS)
				{
					connectAttempts++;
					Sleep(1);
				}
				else
				{
					printf("Failed to connect to server!");
					return;
				}


				// Reduce the amount of debug prints with mod
				if(connectAttempts % 10 == 0)
					_tprintf(TEXT("Waiting for server initialization... GLE=%d\n"), GetLastError());

				continue;
			}

			// All pipe instances are busy, so wait for a second. 
			if (!WaitNamedPipe(PipeName, 1000))
			{
				printf("Could not open pipe: 1 second wait timed out.");
			}
		}

		cout << "Connection established." << endl;

		// The pipe connected; change to message-read mode. 
		Mode = PIPE_READMODE_MESSAGE;
		Success = SetNamedPipeHandleState(
			Pipe,    // pipe handle 
			&Mode,  // new pipe mode 
			nullptr,     // don't set maximum bytes 
			nullptr);    // don't set maximum time 
		if (!Success)
		{
			_tprintf(TEXT("SetNamedPipeHandleState failed. GLE=%d\n"), GetLastError());
		}
	}

	std::string Client::SendReceive(wchar_t* message)
	{
		// Send a message to the pipe server and read the response. 
		Success = TransactNamedPipe(
			Pipe,									// pipe handle 
			message,								// message to server
			(lstrlen(message) + 1) * sizeof(TCHAR), // message length 
			Buffer,									// buffer to receive reply
			BUFFER_SIZE * sizeof(TCHAR),				// size of read buffer
			&CbRead,								// bytes read
			nullptr);								// not overlapped 

		if (!Success && GetLastError() != ERROR_MORE_DATA)
		{
			printf("TransactNamedPipe failed.\n");
			return "-=Failed=-";
		}

		while (true)
		{
			// Break if TransactNamedPipe or ReadFile is successful
			if (Success)
				break;

			// Read from the pipe if there is more data in the message.
			Success = ReadFile(
				Pipe,						// pipe handle 
				Buffer,						// buffer to receive reply 
				BUFFER_SIZE * sizeof(TCHAR),	// size of buffer 
				&CbRead,					// number of bytes read 
				nullptr);					// not overlapped 

			// Exit if an error other than ERROR_MORE_DATA occurs.
			if (!Success && GetLastError() != ERROR_MORE_DATA)
				break;
		}

		const auto tmp = wstring(Buffer);
		return string(tmp.begin(), tmp.end());
	}

	void Client::Close()
	{
		CloseHandle(Pipe);
	}
}
