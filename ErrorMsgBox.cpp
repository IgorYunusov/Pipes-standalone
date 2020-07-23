#include "pch.h"
#include "ErrorMsgBox.h"

void ErrorMsgBox::Show(LPCWSTR boxTitle)
{
	void* lpBuffer;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		LPTSTR(&lpBuffer),
		0,
		nullptr);

	MessageBox(nullptr, LPCTSTR(lpBuffer), boxTitle, MB_OK);
	LocalFree(lpBuffer);
}
