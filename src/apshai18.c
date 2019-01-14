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

#include "defs.h"
#include "os.h"
#include "libc.h"
#include "utils.h"
#include "dungeon.h"
#include "plot.h"

// comment in to skip the sound intro when developing
#define SKIP

static void peformRAMTest()
{
    uchar a = 0;
    uchar n = TRSMemory;
    if (n >= 64) n -= 3; // dont test the top 3k screen RAM + KB

    // loop 1K at a time.
    printfat(0,1,"RAM TEST ");
    do
    {
        uchar b = a<<2;
        ++a;
        if (TRSMemory < 64) b += 0x40; 
        
        printfat(9,1, "%dK ", (int)a);
        if (!ramTest(b, 4)) break;
        --n;
    } while (n);

    if (!n)
        printf("OK\n");
    else
        printf("FAILED at %x\n", (uint)TRSMemoryFail);
}

static char getSingleCommand(const char* msg)
{
    char c;
    lastLine();
    outs(msg);
    c = getkey();
    if (islower(c)) c = _toupper(c);
    return c;
}

static void startGame()
{
    uchar v = 1;
    char c;

    cls();

    while (!generateDungeon()) ;

    for (;;)
    {
        if (v)
        {
            renderDungeon();
            renderPlayer();
            v = 0;
        }
        
        c = scanKey();
        if (c == ' ') break;
        if (islower(c)) c = _toupper(c);
        if (c == KEY_ARROW_RIGHT)
        {
            panXY(1,0);
            v = 1;
        }
        else if (c == KEY_ARROW_LEFT)
        {
            panXY(-1,0);
            v = 1;
        }
        else if (c == KEY_ARROW_UP)
        {
            panXY(0,-1);
            v = 1;
        }
        else if (c == KEY_ARROW_DOWN)
        {
            panXY(0,1);
            v = 1;
        }
        else if (c == 'I')
        {
            zoomIn();
            v = 1;
        }
        else if (c == 'O')
        {
            zoomOut();
            v = 1;
        }
        else if (c == 'A') turnLeft();
        else if (c == 'D') turnRight();
        else if (c == 'W') moveFoward();
    }
}

static void mainloop()
{
    cls();
    
    printf("TRS-80 Model %d (%dk RAM)\n", (int)TRSModel, (int)TRSMemory);

#ifdef SKIP
    {
        //int v;
        //printf("Stack %x\n", ((int)&v) + 4);
    }
#else

    // When you run this on a real TRS-80, you'll thank this RAM test!
    peformRAMTest();
#endif

    outs("\nAPSHAI 2018!\n");
    getSingleCommand("Enter to begin");

    for (;;)
    {
        char c;
        startGame();

        c = getSingleCommand("Play Again (Y/N)?");
        if (c == 'N') break; 
    }
}

void main()
{
    initModel();
    
    // initialise our own mini-clib
    libcInit();
    
    mainloop();
}
