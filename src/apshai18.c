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

#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>

#include "defs.h"
#include "os.h"
#include "utils.h"
#include "dungeon.h"
#include "plot.h"
#include "rect.h"
#include "game.h"
#include "dist.h"

// skip RAM test
#define SKIP

jmp_buf main_env;

static char getSingleCommand(const char* msg)
{
    lastLine();
    return getSingleChar(msg);
}

static void startGame()
{
    uchar v = 1;
    static char key;

    cls();

    while (!generateDungeon()) ;

    TPF("Placing Treasure...\n");
    distributeTreasure(treasures);

    for (;;)
    {
        if (v)
        {
            renderDungeon();
            renderPlayer();
            v = 0;
        }
        
        key = toupper(scanKeyMatrix(key));

        switch (key)
        {
        case KEY_ARROW_RIGHT:
            panXY(1,0);
            v = 1;
            break;
        case KEY_ARROW_LEFT:
            panXY(-1,0);
            v = 1;
            break;
        case KEY_ARROW_UP:
            panXY(0,-1);
            v = 1;
            break;
        case KEY_ARROW_DOWN:
            panXY(0,1);
            v = 1;
            break;
        case 'I':
            zoomIn();
            v = 1;
            break;
        case 'O':
            zoomOut();
            v = 1;
            break;
        case 'A':
            turnLeft();
            break;
        case 'D':
            turnRight();
            break;
        case 'W':
            moveFoward();
            break;
        }

        if (strchr("IOAD", key)) key = 0;
    }
}

static void mainloop()
{
    cls();
    
    printf_simple("TRS-80 Model %d (%dK RAM)\n", (int)TRSModel, (int)TRSMemory);

#ifdef SKIP
    {
        //int v;
        //printf_simple("Stack %x\n", ((int)&v) + 4);
    }
#else

    // When you run this on a real TRS-80, you'll thank this RAM test!
    peformRAMTest();
#endif

    outs("\nAPSHAI 2018!\n");
    getSingleCommand("Enter to begin");

    for (;;)
    {
        if (!setjmp(main_env))
        {
            startGame();
        }
        else
        {
            char c = getSingleCommand("Play Again? (Y/N)");
            if (c != 'Y') break;
        }
    }
}

int main()
{
    initModel();
    setStack();
    mainloop();
    revertStack();
    
    return 0;   // need this to ensure call to revert (else jp)
}
