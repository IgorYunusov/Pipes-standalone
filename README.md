# Pipes-standalone
Named Pipes Server/Client

Inter-process communication library. Pipe client input will transact with server to get a response from Dll back to client exe.

Example Use:

Initialize
```cpp

// thread created on attach
DWORD Init(LPVOID hInstance)
{
	auto* const hPipe = Pipes::Server::Init();

	Window::Init();    // SetWindowLongPtr or initialize your WndProc subclass here

	while (!_safeToExit)
	{
		RunPipes();

		Sleep(10);    // sleep interval in between ticks
	}

	WaitForSingleObject(hPipe, INFINITE);    // wait for signal state or check handle exit code here

	Window::Close();    // restore window pointer

	FreeLibraryAndExitThread(HMODULE(hInstance), 0);    // exit
}

```

Run on Tick
```cpp

void RunPipes()
{
	if (!Pipes::Server::Request.empty())
	{
		if (Pipes::Server::Request != "@close")
		{
			// _luaCommand = Pipes::Server::Request;    // set communication request here such as a console lua call

			const auto command = Pipes::Server::Request;
			Pipes::Server::Request = "";    // after extraction don't let request linger

			// LuaBase::State = ShouldExecute;    // here you could trigger lua execution based on global _luaCommand
		}
		else
		{
			Pipes::Server::Response = "#closing";    // program exit call received
			Pipes::Server::Cv.notify_one();
		}
	}

	if (!Pipes::Server::Connected)
		_safeToExit = true;
	else
		Window::Send(WM_USER_TICK);    // push user message to WndProc callback
}

```

Handle Client
```cpp

// run client thread to connect and feed input (manually in this case)
void RunClient()
{
	Pipes::Client::Init();

	string input;
	do 
	{
		getline(cin, input);
		if (!input.empty()) 
		{
			std::tuple<string, string> response (input.substr(0, input.find('=')), input);
			
			// Convert input string to wchar_t*
			auto* const wInput = new wchar_t[input.length() + 1];
			std::copy(input.begin(), input.end(), wInput);
			wInput[input.length()] = 0;
			
			std::get<1>(response) = Pipes::Client::SendReceive(wInput);

			std::cout << std::get<0>(response) << ": " << std::get<1>(response) << std::endl;
		}
		
	} while (input != "@close");

	exit(0);
}

```
