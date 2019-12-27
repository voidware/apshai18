

// a position is a 16 bit value on the MAX_SCALEBITS scale
typedef int Pos;

// location of something (full res)
typedef struct
{
    Pos x;
    Pos y;
} Coord;

typedef struct
{
    // prefix
    uchar   id;
    uchar   room;
    Coord   pos;

    // treasure
    uchar   value;
    
} Treaure;

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

extern Player player;

#define CPLAYER ((Creature*)&player)

