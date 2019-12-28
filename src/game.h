

#define NUM_FEATUES 50
#define MAX_ROOMS (NUM_FEATUES+1)

#ifdef STANDALONE
#define MAX_ROOM_EXITS 7
#else
// this is not enough, some rooms require 6 or even more
#define MAX_ROOM_EXITS 5
#endif

#define MAX_TREASURES 30

#ifdef STANDALONE

static int verbose = 1;

// debug printf
#define DPF(_c, _x)  if (_c) printf(_x)
#define DPF1(_c, _x, _a)  if (_c) printf(_x, _a)
#define DPF2(_c, _x, _a, _b)  if (_c) printf(_x, _a, _b)
#define DPF3(_c, _x, _a, _b, _d)  if (_c) printf(_x, _a, _b, _d)

// TRS print
#define TPF(_x)
#define TPF1(_x, _a)

#else

#define assert(_x)
#define DPF(_c, _x)
#define DPF1(_c, _x, _a)
#define DPF2(_c, _x, _a, _b)
#define DPF3(_c, _x, _a, _b, _d)

#define TPF(_x)   printf_simple(_x)
#define TPF1(_x, _a)   printf_simple(_x, _a)

#endif

// a position is a 16 bit value on the MAX_SCALEBITS scale
typedef int Pos;

// location of something (full res)
typedef struct
{
    Pos x;
    Pos y;
} Coord;


typedef int Val;

typedef struct
{
    const char*     name;
    Val             value;
    char            count;  // 0=>1, mark < 0 when used
    
} TreasureGen;

typedef struct
{
    // prefix
    uchar   id;
    uchar   room;
    Coord   pos;

    // treasure
    const TreasureGen*    gen;
    
} Treasure;


typedef struct
{
    // prefix
    uchar   id;
    uchar   room;
    Coord   pos;

    // creature
    uchar   dir;
    uchar   wounds;
    uchar   fatigue;
    
} Creature;

typedef struct
{
    // prefix
    uchar   id;
    uchar   room;
    Coord   pos;

    // Creature
    uchar   dir;
    uchar   wounds;
    uchar   fatigue;

    // player
    uchar   weight;
    uchar   arrows;
    uchar   magic_arrows;
    uchar   current_enemy;
    uchar   slain;

    
} Player;

enum RoomFlags
{
    room_normal = 0,
    room_corridor = 1,
};

typedef struct
{
    Rect    r;
    uchar   no; // room number
    uchar   flags; // RoomFlags
    uchar   exits[MAX_ROOM_EXITS];  // index into exits array, 1-based
} Room;

enum Direction
{
    Void = 0,
    North = 1,
    East = 2,
    South = 4, 
    West = 8,
};


typedef struct 
{
    uchar x;
    uchar y;
    char d; // direction
    uchar flags;
    uchar room; // index into rooms when exit added
    uchar otherroom;
} Exit;


extern Player player;
extern uchar roomCount, corridorCount;
extern Room rooms[];
extern uchar treasureCount;
extern Treasure treasures[];
extern uchar cexitCount;
extern Exit exits[];

#define CPLAYER ((Creature*)&player)

#define BASE_ID_TREASURE 101

