/**
 *
 *    _    __        _      __                           
 *   | |  / /____   (_)____/ /_      __ ____ _ _____ ___ 
 *   | | / // __ \ / // __  /| | /| / // __ `// ___// _ \
 *   | |/ // /_/ // // /_/ / | |/ |/ // /_/ // /   /  __/
 *   |___/ \____//_/ \__,_/  |__/|__/ \__,_//_/    \___/ 
 *                                                       
 *  Copyright (©) Voidware 2018.
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *  deal in the Software without restriction, including without limitation the
 *  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *  sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 * 
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 * 
 *  THE SOFTWARE IS PROVIDED "AS IS," WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *  IN THE SOFTWARE.
 * 
 *  contact@voidware.com
 */

#ifdef _WIN32
#include <windows.h>
#else
#include "defs.h"
#include "os.h"
#endif

#include "libc.h"

#ifdef _WIN32
usmint BASE_OpenConsoleOutput()
{
    return (usmint)GetStdHandle(STD_OUTPUT_HANDLE);
}

usmint BASE_OpenConsoleInput()
{
    return (usmint)GetStdHandle(STD_INPUT_HANDLE);
}

void BASE_Read(usmint h, void* buf, unsigned int amt, usmint * nr)
{
    DWORD n;
    ReadFile((HANDLE)h, buf, amt, &n, 0);

    // remove whitespace on end to simulate loss of newline
    while (n && ((char*)buf)[n-1] == 0xd || ((char*)buf)[n-1] == 0xa) --n;

    // newline is lost, so add it back
    if (n >= amt) n = amt-1;
    ((char*)buf)[n] = '\n';
    ++n;

    *nr = n;
}

void BASE_Write(usmint h, const void* buf,
                usmint amt, usmint* nw)
{
    WriteFile((HANDLE)h, buf, amt, nw, 0);
}

void* BASE_MemoryAllocate(unsigned int amt)
{
    return HeapAlloc(GetProcessHeap(), 0, amt);
}

void BASE_MemoryFree(void* p)
{
    HeapFree(GetProcessHeap(), 0, p);
}

char* BASE_CommandLine()
{
    return GetCommandLine();
}

usmint BASE_OpenFile(const char* filename, usmint flags)
{
    return 0;
}

void BASE_CloseFile(usmint h)
{
}
#else

// trs80


usmint BASE_OpenConsoleOutput()
{
    return 1;
}

usmint BASE_OpenConsoleInput()
{
    return 0;
}

void BASE_Read(usmint h, void* buf, usmint amt, usmint * nr)
{
    // using our own getline
    *nr = getline2(buf, amt);
}

void BASE_Write(usmint h, const void* buf,
                usmint amt, usmint* nw)
{
    const char* p = (const char*)buf;
    while (amt)
    {
        --amt;
        outchar(*p++);
    }
}

void* BASE_MemoryAllocate(unsigned int amt)
{
    return 0;
}

void BASE_MemoryFree(void* p)
{
}

char* BASE_CommandLine()
{
    return 0;
}

usmint BASE_OpenFile(const char* filename, usmint flags)
{
    return 0;
}

void BASE_CloseFile(usmint h)
{
}


#endif // WIN32




