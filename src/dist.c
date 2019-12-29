/**
 *
 *    _    __        _      __                           
 *   | |  / /____   (_)____/ /_      __ ____ _ _____ ___ 
 *   | | / // __ \ / // __  /| | /| / // __ `// ___// _ \
 *   | |/ // /_/ // // /_/ / | |/ |/ // /_/ // /   /  __/
 *   |___/ \____//_/ \__,_/  |__/|__/ \__,_//_/    \___/ 
 *                                                       
 *  Copyright (©) Voidware 2019.
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

/* distribute treasures & monsters */

#include "defs.h"
#include "rect.h"
#include "game.h"
#include "dist.h"

#ifdef STANDALONE

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

static uint64_t _seed = 4101842887655102017LL;

static void seed(const uint64_t s)
{
    _seed ^= s;
}

static unsigned int random()
{
    _seed ^= _seed >> 21;
    _seed ^= _seed << 35;
    _seed ^= _seed >> 4;
    return _seed*2685821657736338717LL;
}

static unsigned int randc(unsigned int n)
{
    return random() % n;
}

static unsigned int randn(unsigned int n)
{
    return random() % n;
}

// dummy
uchar roomCount, corridorCount;
Room rooms[MAX_ROOMS];
uchar treasureCount;
Treasure treasures[MAX_TREASURES];

#else

#include "os.h"

#endif


// include generator tables
#include "gen.h"

void distributeTreasure(Treasure* tr)
{
    char mark[MAX_ROOMS+1];
    uchar prob[MAX_ROOMS];

    assert(roomCount <= MAX_ROOMS);

    uchar i;
    uchar v;

    treasureCount = 0;

    mark[0] = 1; // don't place in room#1
    
    for (i = 1; i < roomCount; ++i)
    {
        // don't place in corridor
        mark[i] = (rooms[i].flags & room_corridor);
    }

    DPF1(verbose, "distribute treasures into %d rooms\n", roomCount - corridorCount);
    
    for (;;)
    {
        // find most valuable unallocated treasure
        TreasureGen* best = 0;
        TreasureGen* p;
        for (p = treasureGen; p->name; ++p)
        {
            if (p->count < 0) continue; // allocated
            
            // >= prefer later in table
            if (!best || p->value >= best->value)
            {
                best = p;
            }
        }

        if (!best) break; // done

        // probability gradient adjustment factor
        v = best->value/8;

        // adjust 
        --best->count;
        
        do
        {
            tr->id = (best - treasureGen) + BASE_ID_TREASURE;
            tr->gen = best;
            
            DPF3(verbose, "Allocating T%d, '%s' (%d); ", tr->id, best->name, best->value);

        retry: ;
            
            uchar y = 0;
            int sum = 0;
            char m;

            // start off negative so valuables do not get allocated
            // to early rooms
            m = 1 - (v >> 1);

            // build probability gradient for each room
            for (i = 0; i < roomCount; ++i)
            {
                y += v;
                while (y >= roomCount)
                {
                    y -= roomCount;
                    ++m;
                }

                prob[i] = 0;

                if (m > 0 && !mark[i]) // available?
                {
                    prob[i] = m;
                    sum += m;
                }

                DPF1(verbose > 1, "%d ", (int)prob[i]);
            }
            DPF1(verbose > 1, "sum=%d ", sum);

            if (!sum)
            {
                // release adjacent locations
                y = 0;
                for (i = 0; i < roomCount; ++i)
                {
                    if (mark[i] < 0)
                    {
                        mark[i] = 0;
                        ++y;
                    }
                }
                if (!y)
                {
                    DPF(verbose, "Failed to allocate room!\n");
                    goto done;
                }
                
                DPF(verbose, "* ");
                goto retry;
            }

            sum = randn(sum) + 1;  // 1..sum
            for (i = 0; i < roomCount; ++i)
            {
                sum -= prob[i];
                if (sum <= 0)
                {
                    DPF1(verbose, "to room %d\n", i+1);
                    mark[i] = tr->id;

                    // not allocated into room#1
                    assert(i);

                    // block adjacent rooms too
                    if (!mark[i-1]) mark[i-1] = -1;
                    if (!mark[i+1]) mark[i+1] = -1; // overflow ok, MAX_ROOMS+1

                    tr->room = i+1;
                    ++tr;
                    ++treasureCount;                    
                    
                    break;
                }
            }
            
        } while (--best->count >= 0); // finish with -1 => allocated
    }

 done: ;

    DPF1(verbose, "Total Treasures: %d\n", (int)treasureCount);
}



#ifdef STANDALONE

int main(int argc, char** argv)
{
    seed(time(0));

    int i;
    for (i = 1; i < argc; ++i)
    {
        if (!strcmp(argv[i], "-v"))
        {
            verbose = atoi(argv[++i]);
        }
    }

    // how many treasures
    int tcount = 0;
    TreasureGen* tg = treasureGen;
    while (tg->name)
    {
        int n = tg->count;
        if (!n) n = 1;
        tcount += n;
        ++tg;
    }

    DPF1(verbose, "distributing %d treasures\n", tcount);

    // fake arrangement of corridors
    roomCount = 50;
    corridorCount = 0;
    for (i = 0; i < roomCount; ++i)
    {
        int v = randc(2);
        rooms[i].flags = v;
        corridorCount += v;
        if (roomCount - corridorCount - 1 <= tcount) break; 
    }

    DPF1(verbose, "Total corrdidors %d\n", corridorCount);
    DPF1(verbose, "Total rooms %d\n", roomCount - corridorCount);

    distributeTreasure(treasures);
    return 0;
}

#endif
