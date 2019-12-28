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

/*
 * The dungeon is generated according to a character based layout
 * size, eg 64x48 and these cells correspond to pixels accoring to (2x, y),
 * so that the map would fill a 128x48 screen at unit scale.
 *
 * When zoomed in the scale is 2x, 4x, 8x. Of course now only part of the
 * dungeon is visible and will be clipped accordingly. 
 */

#include "defs.h"
#include "dungeon.h"
#include "rect.h"
#include "utils.h"
#include "game.h"

#ifdef STANDALONE

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>

static int halt = 0;

// error print
#define EPF(_c, _x)  { if (_c) { printf(_x); halt=1;} }
#define EPF1(_c, _x, _a)  { if (_c) { printf(_x, _a); halt=1; } }
#define EPF2(_c, _x, _a, _b)  { if (_c) { printf(_x, _a, _b); halt=1; } }


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

#else

#include "os.h"
#include "plot.h"

#define EPF(_c, _x)
#define EPF1(_c, _x, _a)
#define EPF2(_c, _x, _a, _b)

#endif // STANDALONE

#define SMALL
#define DUN_WBITS 6U
#define DUN_WIDTH  (1<<DUN_WBITS)
#define DUN_HEIGHT 48

#define minRoomSizeX 4
#define maxRoomSizeX 6
#define minRoomSizeY 4
#define maxRoomSizeY 7

#define minCorridorLength 4
#define maxCorridorLength 7
#define maxCorridorConnect 10

// of 128
#define roomChance 64

// if all were rooms, this can be exceeded
#define MAX_EXITS ((NUM_FEATUES+1)*3+2)


#ifdef SMALL
typedef signed char Int;
typedef unsigned char Uint;
#else
typedef int Int;
typedef unsigned int Uint;
#endif

static Uint randomInt(Uint a, Uint b)
{
    // random x, a <= x <= b
    return randc(b - a + 1) + a;
}
 
#define SET_TILE(_x, _y, _c) _tiles[(_x) + ((_y) << DUN_WBITS)] = (_c)
#define GET_TILE(_x, _y) _tiles[(_x) + ((_y) << DUN_WBITS)]

// when x, y is 2x, 2y
#define GET_TILE2(_x, _y) _tiles[((_x)>>1) + ((_y) << (DUN_WBITS-1))]

#define TILE_BASE(_x, _y) (_tiles + (_x) + ((_y) << DUN_WBITS))


#define EXIT_IS_FINAL(_e) ((_e).flags & 1)
#define EXIT_SET_FINAL(_e) ((_e).flags |= 1)

typedef struct
{
    // entry
    Uint x;
    Uint y;
    Int d;

    Rect r;
    
    Uint w;
    Uint h;
    Uint len;

    // exit
    Uint ox;
    Uint oy;

    Uint room;
    
} Ent;

enum Tile
	{
		Unused		= 0,
		Floor		= ' ',
		ClosedDoor	= '+',
		UpStairs	= '<',
		DownStairs	= '>'
	};

enum FailCode
    {
        fail_void = 0,
        fail_max_room_exits = 1,
        fail_max_exits = 2,
    };


// these are valid during generation
static unsigned char* _tiles;

static Uint generationFailed;


Uint roomCount; // rooms+corridors
Uint corridorCount;
Room rooms[MAX_ROOMS];  // valid after generation
Uint treasureCount;
Treasure treasures[MAX_TREASURES];
Uint exitCount;
Exit exits[MAX_EXITS];

typedef struct
{
    Uint u;
    Uint v1;
    Uint v2;
} VHLine;

#define MAX_HLINES  ((NUM_FEATUES*2)+10)
#define MAX_VLINES  ((NUM_FEATUES*2)+10)

// x8 is max scale
#define MAX_SCALEBITS 3

// coordinates are stored 2x, 2y
static VHLine hlines[MAX_HLINES];
static VHLine vlines[MAX_VLINES];
static Uint hlineCount;
static Uint vlineCount;

// top left of viewport
static int viewx;
static int viewy;
static Uint scalebits;
static Uint entranceRoom;

Player player;

#define VIEW_W  128
#define VIEW_H  48


static void move(Ent* e)
{
    Int d = e->d;
    
    if (d & North)
    {
        if (!e->r.y1) return;
        --e->r.y1;
    }
    if (d & East)
    {
        if (e->r.x1 == DUN_WIDTH) return;
        ++e->r.x1;
    }
    if (d & South)
    {
        if (e->r.y1 == DUN_HEIGHT) return;
        ++e->r.y1;
    }
    if (d & West)
    {
        if (!e->r.x1) return;
        --e->r.x1;
    }
}

static BOOL checkRect(Ent* e)
{
    // also check for unsigned wrap-around

    unsigned char* tp;
    Uint dy;

    if (e->r.x1 <= 0 || e->r.y1 <= 0 || e->r.x2 >= DUN_WIDTH || e->r.y2 >= DUN_HEIGHT) return FALSE;
    
    tp = TILE_BASE(e->r.x1, e->r.y1);
    dy = e->h;
    while (dy)
    {
        unsigned char* t1 = tp;
        Uint dx = e->w;
        while (dx)
        {
            --dx;
            if (*t1++) return FALSE; // used
        }
        tp += DUN_WIDTH;
        --dy;
    }

    return TRUE;
}

static Uint findRoom(Uint x, Uint y)
{
    // return 1-based room index, 0 for fail
    
    Uint i;
    for (i = 0; i < roomCount; ++i)
    {
        if (rectContainsPoint(&rooms[i].r, x, y)) return i+1;
    }
    return 0;
}

static Int getConnectedRoom(Uint ri, Uint ei)
{
    // ri is 1-based room index
    // ei is 0-based exit index

    // return 1-based room index, 0=>nowhere
    // -1 => no exit
    
    Int r = -1; // invalid exit

    assert(ei < MAX_ROOM_EXITS);
    assert(ri > 0 && ri <= roomCount);

    Uint e = rooms[ri-1].exits[ei];  // 1-based exit index
    if (e)
    {
        assert(e <= exitCount);
        
        Exit* ep = exits + (e - 1);
        
        if (ep->room == ri) r = ep->otherroom;
        else if (ep->otherroom == ri) r = ep->room;
        
        EPF1(r < 0, "Room has no other %d\n", rooms[ri-1].no);
        
    }
    return r;
}

static BOOL roomsConnected(Uint ra, Uint rb)
{
    // is room `ra` connected to room `rb` ?
    // ra is 1-based room index
    // rb is 1-based room index

    Int i;
    for (i = 0; i < MAX_ROOM_EXITS; ++i)
    {
        Int j = getConnectedRoom(ra, i); // NB: can return 0
        if (j < 0) break;
        if (j == (Int)rb) return TRUE;
    }
    return FALSE;
}

static void labelByDistance(Uint r1)
{
    // r1 = 1-based index of room1

    Uint rstack[MAX_ROOMS];
    Uint top = 0;
    Uint bot = 0;
    rstack[bot++] = r1;

    while (top != bot)
    {
        Uint r = rstack[top++];
        Uint i;

        rooms[r-1].no = top; // start at 1
        
        for (i = 0; i < MAX_ROOM_EXITS; ++i)
        {
            Int j = getConnectedRoom(r, i);
            if (j < 0) break;

            Uint k;
            for (k = 0; j > 0 && k < bot; ++k)
            {
                if ((Int)rstack[k] == j) j = -1; // already listed
            }

            if (j > 0)
            {
                rstack[bot++] = j;
            }
        }
    }
}

static BOOL spontaneousExit2(Ent* e, Uint room1)
{
    BOOL v;
    
    e->r.x1 = e->x;
    e->r.y1 = e->y;

    move(e);
    v = (GET_TILE(e->r.x1, e->r.y1) == Floor);
    if (v)
    {
        Uint room2 = findRoom(e->r.x1, e->r.y1);
        v = room2 && !roomsConnected(room1, room2);

        if (v)
        {
            //DPF2(verbose, "spontaneous exit rooms %d->%d\n", room1, room2);
            e->room = room2;
        }
    }
        
    return v;
}

#if 0
static Uint scan(Ent* e, Uint v)
{
    Uint t;
        
    e->r.x1 = e->x;
    e->r.y1 = e->y;

    for (;;)
    {
        t = GET_TILE(e->r.x1, e->r.y1);
        if (t != v) break;
        if (!move(e)) break;
    }
    return t;
}
    
static BOOL spontaneousExit(Ent* e)
{
    BOOL v;
    
    e->r.x1 = e->x;
    e->r.y1 = e->y;

    move(e);
    v = (GET_TILE(e->r.x1, e->r.y1) == Floor);
    if (v)
    {
        Uint t;
        Ent e2;
        e2.x = e->x;
        e2.y = e->y;

        t = GET_TILE(e2.x, e2.y);
        if (t == (East|West))
        {
            e2.d = East;
            v = (scan(&e2, East|West) != ClosedDoor);
            if (v)
            {
                e2.d = West;
                v = (scan(&e2, East|West) != ClosedDoor); 
            }
        }
        else if (t == (North|South))
        {
            e2.d = North;
            v = (scan(&e2, North|South) != ClosedDoor);
            if (v)
            {
                e2.d = South;
                v = (scan(&e2, North|South) != ClosedDoor); 
            }
        }
        else
        {
            //if (t != ClosedDoor) printf("no exit from '%c' %02X at %d,%d,%d\n", (char)t, t, e->x, e->y, e->d);

            assert(t == ClosedDoor);
            v = FALSE;
        }
    }

    if (v)
    {
        e->room = findRoom(e->r.x1, e->r.y1);
        DPF(verbose && !e->room, "Cannot find spontaneous exit room\n");

        //DPF1(verbose, "spontaneous exit room %d\n", e->room);
    }
        
    return v;
}
#endif


static void addExit(Uint x, Uint y, Int d)
{
    if (exitCount == MAX_EXITS)
    {
        generationFailed = fail_max_exits;
    }
    else
    {
        Exit* e = exits + exitCount++;
        e->x = x;
        e->y = y;
        e->d = d;
        e->flags = 0;

        // when we add an exit, we have just placed a room or a corridor
        // and roomCount is currently valid.
        e->room = roomCount; // index into rooms array+1 (1-based)
        e->otherroom = 0;
    }
}

static void addExitX(Ent* e, Uint y, Int d)
{
    addExit(e->r.x1 + randc(e->w-2) + 2, y, d);    
}

static void addExitY(Ent* e, Uint x, Int d)
{
    addExit(x, e->r.y1 + randc(e->h-2) + 2, d);
}

static void addRoom(Ent* e, uchar flags)
{
    // keep track of the boxes for each room or corridor
    // these will need to be scaled up as usual
    Room* rp = rooms + roomCount;
    memset(rp, 0, sizeof(Room));
    rp->r = e->r;
    rp->flags = flags;
    e->room = ++roomCount; // 1-based
}

static void placeRect(Ent* e, uchar flags)
{
    // assume Already checked
    unsigned char* t1;
    unsigned char* t2;
    unsigned char* t3;
    Uint x, y;

    addRoom(e, flags);

    // expand up and back so we can paint the border
    --e->r.x1;
    --e->r.y1;
    
    t1 = TILE_BASE(e->r.x1, e->r.y1);
    t3 = t1;
    
    // top left
    *t1++ |= South|East;

    // top right
    t1[e->w] |= South|West;

    t2 = TILE_BASE(e->r.x1, e->r.y2);

    // bottom left
    *t2++ |= North|East;

    // bottom right
    t2[e->w] |= North|West;

    // top and bottom h-walls
    x = e->w;
    while (x)
    {
        --x;
        *t1++ |= East|West;
        *t2++ |= East|West;
    }

    t1 = t3;
    y = e->h;
    while (y)
    {
        --y;

        t1 += DUN_WIDTH;
        t2 = t1;

        // left v-wall
        *t2++ |= North|South;

        // inside floor
        x = e->w;
        while (x)
        {
            --x;
            *t2++ = Floor;
        }

        // right v-wall
        *t2 |= North|South;
    }
}

static void setRoomBox(Ent* e)
{
    // NB: dir can be Void

    Uint w = e->w;
    Uint h = e->h;
    Uint d = e->d;

    e->r.x1 = e->x;
    e->r.y1 = e->y;

    if (d & (North | South))
    {
        e->r.x1 -= w>>1;

        if (d == North) e->r.y1 -= h;
        else ++e->r.y1; // south
    }

    if (d & (East | West))
    {
        e->r.y1 -= h>>1;

        if (d == East) ++e->r.x1;
        else e->r.x1 -= w; // west
    }
        
    e->r.x2 = e->r.x1 + w;
    e->r.y2 = e->r.y1 + h;
}
    
static BOOL checkRoom(Ent* e)
{
    setRoomBox(e);
    return checkRect(e);
}

static void placeRoom(Ent* e)
{
    // assume already checked
        
    placeRect(e, room_normal);

    if (e->d != South) // north side
        addExitX(e, e->r.y1, North);
    if (e->d != North) // south side
        addExitX(e, e->r.y2, South);
    if (e->d != East) // west side
        addExitY(e, e->r.x1, West);
    if (e->d != West) // east side
        addExitY(e, e->r.x2, East);
}
    
static BOOL randomRoom(Ent* e)
{
    BOOL v;
    e->w = randomInt(minRoomSizeX, maxRoomSizeX);
    e->h = randomInt(minRoomSizeY, maxRoomSizeY);
    v = checkRoom(e);
    if (v) placeRoom(e);
    return v;
}

static void setCorridorBox(Ent* e)
{
    e->r.x1 = e->ox = e->x;
    e->r.y1 = e->oy = e->y;

    e->r.x2 = e->r.x1 + 1;
    e->r.y2 = e->r.y1 + 1;

    if (e->d == North)
    {
        e->r.y2 = e->r.y1;
        e->r.y1 -= e->len;
        e->oy = e->r.y1 - 1;
    }
    else if (e->d == East)
    {
        ++e->r.x1;
        e->r.x2 += e->len;
        e->ox = e->r.x2;
    }
    else if (e->d == South)
    {
        ++e->r.y1;
        e->r.y2 += e->len;
        e->oy = e->r.y2;
    }
    else // if (dir == West)
    {
        e->r.x2 = e->r.x1;
        e->r.x1 -= e->len;
        e->ox = e->r.x1 - 1;
    }

    e->w = e->r.x2 - e->r.x1;
    e->h = e->r.y2 - e->r.y1;
}
    
static Int checkCorridor(Ent* e)
{
    // cant fit or bad hit -1
    // ok 0
    // >0, hit but ok

    Uint t;

    setCorridorBox(e);
        
    // use bigger margin to avoid edges
    if (!checkRect(e)) return -1; // fail

    t = GET_TILE(e->ox, e->oy);

    // hit non-wall (eg corner?)
    if (t && t != (North|South) && t != (East|West)) return -1;

    // return 0 if nothing at end
    // return edge code if hit something
    return t;
}

static void placeCorridor(Ent* e)
{
    // assume already checked
        
    placeRect(e, room_corridor);
    addExit(e->ox, e->oy, -e->d);

    ++corridorCount;
}

static BOOL randomCorridor(Ent* e)
{
    // find longest corridor
    Int c = 0;
    
    for (e->len = minCorridorLength; e->len <= maxCorridorConnect; ++e->len)
    {
        c = checkCorridor(e);
        if (c) break;
    }

    if (e->len <= minCorridorLength) return FALSE;

    if (c > 0 && e->len <= maxCorridorConnect)
    {
        // corridor abuts to object
        // placeCorridor(e);
    }
    else
    {
        // len-1 >= minCorridorLength, was OK

        Uint i;
        Uint l = e->len-1;
        if (l > maxCorridorLength) l = maxCorridorLength;
            
        // try to place a random room on end. try a few times
        for (i = 0; i < 3; ++i)
        {
            Ent r;
                        
            e->len = randomInt(minCorridorLength, l);
            setCorridorBox(e);

            // can we fit a room here?
            r.x = e->ox;
            r.y = e->oy;
            r.d = e->d;
                
            if (randomRoom(&r)) break;
        }

        if (i == 3) return FALSE; // failed to make a corridor
    }
        
    placeCorridor(e); 
    return TRUE;
}

static Uint dist(Exit* e, Uint x, Uint y)
{
    Int dx = e->x - x;
    if (dx < 0) dx = -dx;
    Int dy = e->y - y;
    if (dy < 0) dy = -dy;
    return dx + dy - ((dx>dy ? dy : dx)>>1);
}

static void addRoomExit(Exit* e, Uint room)
{
    // this can fail if we do not have enough space to add
    // another exit.
    if (room)
    {
        Room* r = rooms + (room - 1);
        uchar* re = r->exits;
        Uint i = MAX_ROOM_EXITS;
        while (i > 0 && *re) { --i; ++re; }
        if (i)
            *re = (e - exits) + 1; // NB: divide!!  1-based
        else
        {
            generationFailed = fail_max_room_exits;
        }
    }
}

static void set_exit_door(Exit* e, Uint otherroom)
{
    // can set generationFailed
    
    EXIT_SET_FINAL(*e);
    SET_TILE(e->x, e->y, ClosedDoor);

    //DPF2(verbose, "exit from room %d to %d\n", e->room, otherroom);

    addRoomExit(e, e->room);

    if (otherroom)
    {
        EPF2(e->otherroom, "exit has already other room %d, trying to add %d\n", e->otherroom, otherroom);

        e->otherroom = otherroom;
        addRoomExit(e, otherroom);
    }
}

// -1 => above
// +1 => below
static Int clip;
//static Int clipx;
//static Int clipy;

static void scaleCoordMax(Coord* c)
{
    // scale coordinates to highest res
    c->y = (c->y << MAX_SCALEBITS) + (1<<(MAX_SCALEBITS-1));
    c->x <<= MAX_SCALEBITS+1;
}

static Int clipCoord(Coord* c)
{
    Int cl = 0;
    int v;

    v = c->x - viewx;
    if (v < 0)
    {
        cl = -1;
        v = 0;
    }
    else if (v > VIEW_W-1)
    {
        cl = 1;
        v = VIEW_W-1;
    }
    c->x = v;

    v = c->y - viewy;
    if (v < 0)
    {
        cl = -1;
        v = 0;
    }
    else if (v >= VIEW_H)
    {
        cl  = 1;
        v = VIEW_H-1;
    }
    c->y = v;
    return cl;
}

static Uint scaleClipX(Uint x)
{
    // NB: x is 2x
    // set `clip` if clipped. return clipped scaled x
    
    int x1;
    
    clip = 0;

    // the scaling needs to include a 0.5 factor so that the lines
    // can be conceptually mid pixel.
    // here we scale x additionally *2, compared to y

    // floor((x+0.5)*2^(scalebits+1)) - floor(0.5*2^(scalebits+1))
    // = (2x+1)*2^scalebits  - 2^scalebits
    // = 2x * 2^scalebits

    x1 = (((int)x)<<scalebits) - viewx;

    if (x1 < 0)
    {
        clip = -1;
        return 0;
    }
    if (x1 >= VIEW_W-1)
    {
        // NB: clip at width-1, because we are going to draw double vlines.
        // we allow max of W-1 to be included in plot.
        clip = 1;
        return VIEW_W-1;  // included in line if drawn
    }
    return (Uint)x1;
}

static Uint scaleClipY(Uint y)
{
    // NB: y input is 2y
    
    int y1;

    clip = 0;

    // the scaling needs to include a 0.5 factor so that the lines
    // can be conceptually mid pixel.
    // BUT y given is already 2y.

    // floor((y+0.5)*2^scalebits)
    
    if (scalebits)
    {
        // (2y + 1)*2^(scalebits-1)
        y1 = ((int)y + 1) << (scalebits-1);
    }
    else
    {
        // otherwise reduce to 1x
        y1 = y>>1;
    }

    // translate to view
    y1 -= viewy;
    
    if (y1 < 0)
    {
        clip = -1;
        return 0;
    }
    if (y1 >= VIEW_H)
    {
        clip = 1;
        return VIEW_H-1;  // included in line
    }
    return (Uint)y1;
}


static void finish()
{
    // this can cause generationFailed
    
    Exit* e;
    Uint n;

    Exit* entr = 0;
    Exit* exit = 0;
    Uint entrd = 0xff;
    Uint exitd = 0xff;

    DPF(verbose, "finishing...\n");

    // first pass to finalise entrance and exit.
    // go through remaining exits to see if any can be turned into
    // actual exits
    e = exits;
    n = exitCount;
    while (n)
    {
        Ent ent;

        // normalise direction
        if (e->d < 0) e->d = -e->d;

        ent.x = e->x;
        ent.y = e->y;
        ent.d = e->d;

        if (spontaneousExit2(&ent, e->room))
        {
            set_exit_door(e, ent.room);
        }
        else
        {
            uchar* tp = TILE_BASE(ent.x, ent.y);
            if (ent.d == West && (!ent.x || tp[-1] == Unused))
            {
                Uint t = dist(e, 0, DUN_HEIGHT);
                if (t < entrd)
                {
                    entrd = t;
                    entr = e;
                }
            }
            else if (ent.d == East && (ent.x == DUN_WIDTH || tp[1] == Unused))
            {
                Uint t = dist(e, DUN_WIDTH, 0);
                if (t < exitd)
                {
                    exitd = t;
                    exit = e;
                }
            }
        }

        ++e;
        --n;
    }

    assert(entr);
    assert(exit);

    // mark entrance and exit doors
    set_exit_door(entr, 0);
    set_exit_door(exit, 0);

    // assign room numbers
    labelByDistance(entr->room);

    entranceRoom = entr->room;

    // place player at start position
    player.room = entranceRoom;
    player.pos.x = entr->x + 1;
    player.pos.y = entr->y;
    player.dir = East;

    // coordinates are at max scale
    scaleCoordMax(&player.pos);
}

static BOOL createFeature()
{
    // return FALSE if cannot add another feature
    // or if generationFailed
    
    Uint cc = 0;
    do
    {
        Uint r;
        Exit* e;
        Ent ent;
        BOOL rm;
        BOOL v;

        // choose a random side of a random room or corridor
        do
        {
            r = randc(exitCount);
            e = exits + r;
        } while (EXIT_IS_FINAL(*e)); // find unused exit

        ent.x = e->x;
        ent.y = e->y;
        ent.d = e->d;

        // build a corridor or a room?
        if (ent.d < 0)
        {
            // from corridor, must have a room
            rm = TRUE;
            ent.d = -ent.d;
        }
        else
        {
            rm = randc(128) < roomChance;
        }

        //v = spontaneousExit(&ent);
        v = spontaneousExit2(&ent, e->room);

        if (!v)
        {
            if (rm) v = randomRoom(&ent);
            else v = randomCorridor(&ent);
        }

        if (v)
        {
            // mark exit as used
            // exit goes from e->room to ent.room
            set_exit_door(e, ent.room);
            //memmove(e, e + 1, (--_nexits - r)*sizeof(Exit));
            DPF1(verbose && cc > 9, "%d tries\n", cc + 1);
            return TRUE;
        }

        if (generationFailed) break;
    }
#ifdef SMALL
    while (++cc != 0);  // exit after 256
#else
    while (++cc != 256);
#endif    
 
    return FALSE;
}

void zoomIn()
{
    // increase scale
    if (scalebits < MAX_SCALEBITS)
    {
        ++scalebits;
        viewx <<= 1;
        viewy <<= 1;
    }
}

void zoomOut()
{
    if (scalebits)
    {
        --scalebits;
        viewx >>= 1;
        viewy >>= 1;
    }
}

void panXY(signed char x, signed char y)
{
    viewx += ((int)x)<<3;
    viewy += ((int)y)<<3;

    //viewx += x;
    //viewy += y;
}


static void createHLines()
{
    // the Hlines and Vlines are assumed to be in the middle of the pixel
    // and therefore the endpoints are included.
    
    Uint x, y;
    VHLine* hp = hlines;
    uchar* tp = _tiles;

    hlineCount = 0;

    // arrange for all x coordinates to be 2x and y to be 2y
    for (y = 0; y < DUN_HEIGHT*2; y+=2)
    {
        BOOL line = FALSE;

        for (x = 0; x < DUN_WIDTH*2; x+=2)
        {
            uchar c = *tp++;
            if (c < ' ')
            {
                c &= East|West;
                if (c == East)
                {
                    // start of line at (x,y)
                    hp->u = y;
                    hp->v1 = x;
                    line = TRUE;
                }
                else if (c == West)
                {
                    // end of line
                    hp->v2 = x;
                    ++hp;
                    ++hlineCount;
                    line = FALSE;
                }
            }
            else if (c == ClosedDoor)  // door
            {
                if (line)
                {
                    // break up line with door
                    hp->v2 = x-1;
                    ++hp;

                    hp->u = y;
                    hp->v1 = x+1;
                    ++hlineCount;
                }
            }
        }

        EPF1(line, "unclosed hline y=%d\n", y/2);
    
    }

    EPF1(hlineCount > MAX_HLINES, "too many hlines %d\n", hlineCount);
}

static void createVLines()
{
    Uint x, y;
    VHLine* vp = vlines;

    vlineCount = 0;

    // arrange for all x coordinates to be 2x and y to be 2y
    for (x = 0; x < DUN_WIDTH*2; x += 2)
    {
        BOOL line = FALSE;
        for (y = 0; y < DUN_HEIGHT*2; y += 2)
        {
            uchar c = GET_TILE2(x, y); // assumes x is 2x and y is 2y
            if (c < ' ')
            {
                c &= North|South;
                if (c == South)
                {
                    // start of vline
                    vp->u = x; // 2x
                    vp->v1 = y; // 2y
                    line = true;
                }
                else if (c == North)
                {
                    vp->v2 = y;  // 2y
                    ++vp;
                    ++vlineCount;
                    line = false;
                }
            }
            else if (c == ClosedDoor)
            {
                if (line)
                {
                    // break vline with door
                    vp->v2 = y-1;
                    ++vp;

                    vp->u = x;
                    vp->v1 = y+1;
                    ++vlineCount;
                }
            }
        }

        EPF(line, "unclosed vline\n");
    }

    EPF1(vlineCount > MAX_VLINES, "too many vlines %d\n", vlineCount);
}

static void init()
{
    memset(_tiles, Unused, DUN_WIDTH*DUN_HEIGHT);
    generationFailed = FALSE;
    exitCount = 0;
    roomCount = 0;
    corridorCount = 0;
}

BOOL generateDungeon()
{
    Ent e;

    // put data on stack
    unsigned char tiles[DUN_WIDTH * DUN_HEIGHT];

    // set pointers to this data
    _tiles = tiles;

    init();

    // place the first room in the centre
    e.x = DUN_WIDTH/2;
    e.y = DUN_HEIGHT/2;
    e.d = Void;
    randomRoom(&e);

    TPF("Generating Dungeon    ");
    
    for (;;)
    {
        if (roomCount >= NUM_FEATUES) break; // enough

        TPF1("\b\b\b%2d%%", (int)(roomCount<<1));
        if (!createFeature())
            
        {
            DPF(verbose, "cannot add any more features\n");
            break;
        }
    }

    TPF("\b\b\b100%%\n");

    if (!generationFailed)
    {
        finish();
        createHLines();
        createVLines();
    }

#ifdef STANDALONE
    static void printDungeon();
    if (verbose) printDungeon();
#endif

    return !generationFailed;
}



#ifndef STANDALONE

static void renderVH()
{
    // render both horizontal lines and vertical lines interleaved
    // so to reduce perceived delay slightly.
    
    VHLine* hp;
    VHLine* vp;
    
    Uint nh;
    Uint nv;

    Uint u;
    Uint v1, v2;

    // hlines are 2x
    hp = hlines;
    nh = hlineCount;
    vp = vlines;
    nv = vlineCount;

    do
    {
        if (nh)
        {
            --nh;
            u = scaleClipY(hp->u); // 2y
            if (clip > 0) nh = 0; // done, y >= H
            else if (!clip) // 0 <= y <= H-1
            {
                v1 = scaleClipX(hp->v1);  // 2x
                v2 = scaleClipX(hp->v2);
                if (v1 < v2)
                {
                    //plotHLine(x1, y, x2, 1);
                    plotSpan2(v1, u, v2-v1+2); //+2 ends included
                }
            }
            ++hp;
        }

        if (nv)
        {
            --nv;
            u = scaleClipX(vp->u); // pass in 2x
            if (clip > 0) nv = 0;   // done, x >= w-1
            else if (!clip)
            {
                v1 = scaleClipY(vp->v1); // 2y
                v2 = scaleClipY(vp->v2);
                if (v1 < v2)
                {
                    //plotVLine(x, y1, y2, 1);
                    //plotVLine(x+1, y1, y2, 1); // x+1 ok, since we clip at w-1

                    // plot both lines together, includes ends
                    plotVLine2(u, v1, v2);
                }
            }
            ++vp;
        }
    } while (nh || nv);
}

void renderDungeon()
{
    //uchar vbuf[64*24]; // video double buffer
    //pushVideo(vbuf);
    cls();

    // render the dungeon walls
    renderVH();
    
    //popVideo();
}

static const uchar playerSpriteE[] =
{
    0x04,0,1,
    0x12,0,6,
    0x04,0,
    0,
};

static const uchar playerSpriteW[] =
{
    0x04,0,6,
    0x02,0,1,
    0x14,0,
    0,
};

static const uchar playerSpriteN[] =
{
    0x02,0,4,
    0x02,0x22,0,6,
    0x02,0x22,0,
    0,
};

static const uchar playerSpriteS[] =
{
    0x02,0x22,0,6,
    0x02,0x22,0,4,
    0x02,0,
    0,
};

static void _renderCreature(Creature* cr, uchar col)
{
    if (scalebits == MAX_SCALEBITS)
    {
        Coord c;
        c = cr->pos;
        if (!clipCoord(&c))
        {
            char x = c.x-1;
            char y = c.y-1;
            
            const char* dp = 0;

            switch (cr->dir)
            {
            case North:
                dp = playerSpriteN;
                break;
            case East:
                x -= 2;
                dp = playerSpriteE;
                break;
            case South:
                x -= 2;
                dp = playerSpriteS;
                break;
            case West:
                dp = playerSpriteW;
                break;
            }

            if (dp) drawRLE(x, y, dp, col);
        }
    }
}

static uchar _turnLeft(uchar d)
{
    d >>= 1;
    if (!d) d = 8;
    return d;
}

static uchar _turnRight(uchar d)
{
    d <<= 1;
    if (d > 8) d = 1;
    return d;
}

void renderPlayer()
{
    _renderCreature(CPLAYER, 1);
}

void turnLeft()
{
    Creature* cr = CPLAYER;
    _renderCreature(cr, 0);
    cr->dir = _turnLeft(cr->dir);
    _renderCreature(cr, 1);
}

void turnRight()
{
    Creature* cr = CPLAYER;
    _renderCreature(cr, 0);
    cr->dir = _turnRight(cr->dir);
    _renderCreature(cr, 1);
}

void moveFoward()
{
    Coord c;
    c = player.pos;

    char dx = 0;
    char dy = 0;

    switch (player.dir)
    {
        case North:
            --c.y;
            dy = -1;
            break;
        case East:
            ++c.x;
            dx = 2;
            break;
        case South:
            ++c.y;
            dy = 1;
            break;
        case West:
            --c.x;
            dx = -3;
            break;
    }

    // coordinate of "head"
    Coord c2;
    c2.x = c.x + dx;
    c2.y = c.y + dy;
    
    if (!clipCoord(&c2))
    {
        Int dv;
        char v;
        if (player.dir & (East|West))
        {
            // check height of sprite
            for (dv = -1; dv <= 1; ++dv)
            {
                v = getPixel(c2.x, c2.y + dv);
                if (v == Floor) v = 0;
                if (v) break;
            }
        }
        else
        {
            // check width of sprite
            for (dv = -3; dv <= 2; ++dv)
            {
                v = getPixel(c2.x + dv, c2.y);
                if (v == Floor) v = 0;
                if (v) break;
            }
        }

        if (dv > 1)
        {
            _renderCreature(CPLAYER, 0);
            player.pos = c;
            _renderCreature(CPLAYER, 1);            
        }
    }
}

#endif /* !STANDALONE */

#ifdef STANDALONE

static int unicode = 0;

static int convertToPrint(int c)
{
    int tile = c;
    int u = c;
    
    if (!tile)
    {
        u = tile = '.';
    }
    else if (tile < ' ')
    {
        switch (tile)
        {
        case North | South: // vwall
            tile = 0xb3;  // single |
            u = 0x2502;
            break;
        case East | West: // hwall
            tile = 0xc4;
            u = 0x2500;
            break;
        case East | South: // top left
            tile = 0xda;
            u = 0x250c;
            break;
        case North | East: // bottom left
            tile = 0xC0;
            u = 0x2514;
            break;
        case West | South: // top right
            tile = 0xbf;
            u = 0x2510;
            break;
        case North | West: // bottom right
            tile = 0xd9;
            u = 0x2518;
            break;
        case West | East | South: // hwall with south T
            tile = 0xc2;
            u = 0x252c;
            break;
        case West | East | North: // hwall with north T
            tile = 0xc1;
            u = 0x2534;
            break;
        case North | South | West: // vwall left T
            tile = 0xb4;
            u = 0x2524;
            break;
        case North | South | East: // vwall right T
            tile = 0xc3;
            u = 0x251c;
            break;
        case North | South | East | West: // v & h intersection
            tile = 0xc5;
            u = 0x253c;
            break;
        default:
            u = tile = '?';
            break;
        }
    }
    return unicode ? u : tile;
}


    enum
    {
        UTFmax	= 4,		/* maximum bytes per rune */
        Runesync	= 0x80,		/* cannot represent part of a UTF sequence (<) */
        Runeself	= 0x80,		/* rune and UTF sequences are the same (<) */
        Runeerror	= 0xFFFD,	/* decoding error in UTF */
        Runemax	= 0x10FFFF,	/* maximum rune value */
    };

    enum 
    {
	Bit1	= 7,
	Bitx	= 6,
	Bit2	= 5,
	Bit3	= 4,
	Bit4	= 3,
	Bit5	= 2,

	T1	= ((1<<(Bit1+1))-1) ^ 0xFF,	/* 0000 0000 */
	Tx	= ((1<<(Bitx+1))-1) ^ 0xFF,	/* 1000 0000 */
	T2	= ((1<<(Bit2+1))-1) ^ 0xFF,	/* 1100 0000 */
	T3	= ((1<<(Bit3+1))-1) ^ 0xFF,	/* 1110 0000 */
	T4	= ((1<<(Bit4+1))-1) ^ 0xFF,	/* 1111 0000 */
	T5	= ((1<<(Bit5+1))-1) ^ 0xFF,	/* 1111 1000 */

	Rune1	= (1<<(Bit1+0*Bitx))-1,		/* 0000 0000 0111 1111 */
	Rune2	= (1<<(Bit2+1*Bitx))-1,		/* 0000 0111 1111 1111 */
	Rune3	= (1<<(Bit3+2*Bitx))-1,		/* 1111 1111 1111 1111 */
	Rune4	= (1<<(Bit4+3*Bitx))-1,
                                        /* 0001 1111 1111 1111 1111 1111 */

	Maskx	= (1<<Bitx)-1,			/* 0011 1111 */
	Testx	= Maskx ^ 0xFF,			/* 1100 0000 */

	Bad	= Runeerror,
    };

static int UtoUtf8(char *str, unsigned long c)
{
    // return number of bytes
    /* Runes are signed, so convert to unsigned for range check. */
        
	/*
	 * one character sequence
	 *	00000-0007F => 00-7F
	 */
	if(c <= Rune1) {
		str[0] = (char)c;
		return 1;
	}

	/*
	 * two character sequence
	 *	0080-07FF => T2 Tx
	 */
	if(c <= Rune2) {
		str[0] = (char)(T2 | (c >> 1*Bitx));
		str[1] = (char)(Tx | (c & Maskx));
		return 2;
	}

	/*
	 * If the Rune is out of range, convert it to the error rune.
	 * Do this test here because the error rune encodes to three bytes.
	 * Doing it earlier would duplicate work, since an out of range
	 * Rune wouldn't have fit in one or two bytes.
	 */
	if (c > Runemax)
		c = Runeerror;

	/*
	 * three character sequence
	 *	0800-FFFF => T3 Tx Tx
	 */
	if (c <= Rune3) {
		str[0] = (char)(T3 |  (c >> 2*Bitx));
		str[1] = (char)(Tx | ((c >> 1*Bitx) & Maskx));
		str[2] = (char)(Tx |  (c & Maskx));
		return 3;
	}

	/*
	 * four character sequence (21-bit value)
	 *     10000-1FFFFF => T4 Tx Tx Tx
	 */
	str[0] = T4 | (char)(c >> 3*Bitx);
	str[1] = Tx | ((c >> 2*Bitx) & Maskx);
	str[2] = Tx | ((c >> 1*Bitx) & Maskx);
	str[3] = Tx | (c & Maskx);
	return 4;
}

static void printDungeon()
{
    int i;
    for (i = 0; i < roomCount; ++i)
    {
        // draw the room numbers

        Room* r = rooms + i;
        Uint cx = ((r->r.x1 + r->r.x2)>>1)-1;
        Uint cy = (r->r.y1 + r->r.y2)>>1;
        char buf[3];
        sprintf(buf, "%2d", r->no);
        SET_TILE(cx, cy, buf[0]);
        SET_TILE(cx+1, cy, buf[1]);
    }
        
    char buf[32];
    for (int y = 0; y < DUN_HEIGHT; ++y)
    {
        for (int x = 0; x < DUN_WIDTH; ++x)
        {
            int t = convertToPrint(GET_TILE(x,y));
            if (unicode)
            {
                int n = UtoUtf8(buf, t);
                buf[n] = 0;
                printf("%s", buf);
            }
            else
            {
                printf("%c", t);
            }
        }
        printf("\n");
    }

    if (generationFailed) printf("GENERATION FAILED!\n");
    printf("Number of rooms: %d\n", roomCount-corridorCount);
    printf("Number of corridors: %d\n", corridorCount);
    printf("Number of doors: %d\n", exitCount);
    printf("Total features: %d\n", roomCount);
    printf("total Hlines: %d\n", hlineCount);
    printf("total Vlines: %d\n", vlineCount);

    if (verbose > 1)
    {
        // print rooms
        int i;
        for (i = 0; i < roomCount; ++i)
        {
            Room* r = rooms + i;
            printf("(%d) -> ", r->no);

            int j;
            for (j = 0; j < MAX_ROOM_EXITS; ++j)
            {
                int k = getConnectedRoom(i + 1, j);
                if (k < 0) break;
                if (k)
                {
                    printf("%d ", rooms[k-1].no);
                }
                else
                {
                    printf("nowhere ");
                }
            }
            printf("\n");
        }
    }
    
}

int main(int argc, char** argv)
{
    seed(time(0));
    
    int n = 1;
    int i;
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-')
        {
            if (!strcmp(argv[i], "-v") && i < argc-1)
            {
                verbose = atoi(argv[++i]);
            }
            if (!strcmp(argv[i], "-u"))
            {
                unicode = 1;
            }
        }
        else
        {
            n = atoi(argv[i]);
        }
    }

    for (i = 0; i < n && !halt && !generationFailed; ++i)
    {
        generateDungeon();

        if (generationFailed)
        {
            printf("Generation Failed: ");
            switch (generationFailed)
            {
            case fail_max_room_exits:
                printf("room exits exceeds %d\n", MAX_ROOM_EXITS);
                break;
            case fail_max_exits:
                printf("total exits exceeds %d\n", MAX_EXITS);
                break;
            default:
                printf("whatever\n");
            }
        }
    }
    
	return 0;
}

#endif
