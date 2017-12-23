/*
* The MIT License
* Copyright 2017 naPalm / PapaRadroach
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute,
* sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
* subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
* LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
* THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once
#include "stdafx.h"

class Logging
{
public:
	static void LogMessage(const char* format, ...)
	{
		char messageBuf[1024] = {0};
		va_list argptr;
		va_start(argptr, format);
		vsnprintf(messageBuf, 1024, format, argptr);
		va_end(argptr);

		HANDLE hFile;
		DWORD dwBytesWritten = 0;
		BOOL bErrorFlag = FALSE;

		hFile = CreateFile(L"./placeinredstandalone.log",                // name of the write
			FILE_APPEND_DATA,          // open for writing
			0,                      // do not share
			NULL,                   // default security
			OPEN_ALWAYS,             // create new file only
			FILE_ATTRIBUTE_NORMAL,  // normal file
			NULL);                  // no attr. template

		if(hFile == INVALID_HANDLE_VALUE)
		{
			return;
		}

		bErrorFlag = WriteFile(
			hFile,           // open file handle
			messageBuf,      // start of data to write
			strlen(messageBuf),  // number of bytes to write
			&dwBytesWritten, // number of bytes that were written
			NULL);            // no overlapped structure

		CloseHandle(hFile);
	}
};