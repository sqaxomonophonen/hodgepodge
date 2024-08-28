#define GAME_BOARD_WIDTH 81
#define GAME_BOARD_HEIGHT 55

#define GAME_BOARD_LEFT         20.0
#define GAME_BOARD_RIGHT        780.0
#define GAME_BOARD_TOP          20.0
#define GAME_BOARD_BOTTOM       530.0

// gameplay constants

#define GAME_SHIPS_MAX  8

#define GAME_PLAYER_TURN_SPEED 500
#define GAME_PLAYER_ACCEL      256
#define GAME_PLAYER_DEACCEL      256
#define GAME_PLAYER_TOP_SPEED  (50*256)
#define GAME_PLAYER_SHOT_SPEED (100*256)
#define GAME_PLAYER_HIT_SPEED (80*256);
#define GAME_PLAYER_SHOT_TTL    600
#define GAME_PLAYER_DEATH_TIME         200

#define GAME_HOLE_SUCK_POWER1   300
#define GAME_HOLE_SUCK_POWER2   7000.0

#define GAME_PARTICLE_TTL       60

#define GAME_TEXTURE_COUNT      7


#define GAME_SCORE_HIT_NORMAL   20
#define GAME_SCORE_KILL_NORMAL  100


// data for a single grid node
struct game_grid_s
{
    float dx,dy; // x/y displacement
    float r,g,b; // color
};

// blob = hole or .. blob
struct blob_s
{
    float x,y;    // position
    float z;      // 1.0 = hole / 0.0 = flat / -1.0 = blob
    float r;      // radius
};

// permanent data for sprites
struct game_sprite_s
{
    GLuint texture;
    float tx1,ty1,tx2,ty2;      // texture coordinates
    float ox,oy;                // origin 
    float scale;
};

// bounding circles 
// (first circle is meant for quick checking - 
// i.e. if two 'first' cicles don't intersect it's safe to say that two objects are not intersecting
// but if they do, the hit detect algorithm will continue checking)
struct game_hit_data_s
{
    int x,y,r;
    
    float scale;

    struct game_hit_data_s *next;
};

// sparkle!! fizzle!!
struct game_particle_s
{
    float x,y;
    
    int dir;
    
    int speed;
    int colour;
    int ttl;
    
    struct game_particle_s *next;
};

// dir - 0.0 to 360.0 degrees => dir = 0 to 65536
struct game_player_s
{
    // 0 = inactive / !0 = active
    int status;    
    
    int lives, score;
    
    float ship_x,ship_y;
    int ship_dir;
    int ship_speed;
    int thrust, firehold;
    
    float how_dead, shot_how_dead; // 0.0=not, 1.0=alot
    
    // when being hit you temporarly take a new direction
    int hit_speed, hit_dir;
    int hit_owner; // who is to be blaimed? (and credited)
    
    // shot .. ttl=time to live - bounce=have the shot bounced yet? 
    float shot_x, shot_y;
    int shot_dir, shot_ttl, shot_bounce;
    
    int control_owner; // 0=player, 1=player ai, 2=enemy ai
}; 

// game variables
struct game_vars_s
{
    GLuint	texture[GAME_TEXTURE_COUNT];
    
    struct game_grid_s grid[GAME_BOARD_WIDTH*GAME_BOARD_HEIGHT];
    struct game_player_s player[GAME_SHIPS_MAX];
    
    int blobs;
    struct blob_s *blob;
        
    // temp
    int i;
        
    // particles 
    struct game_particle_s *particle;
    
    // logic timer specific
    int gradient;
    int counter;
    int lasttime;
    int wait;
    
    float title_intensity;
};


// container for sprite and hit data
struct game_sprites_s
{
    struct game_sprite_s station;
    struct game_sprite_s station_light;
    struct game_sprite_s station_glow;
    struct game_sprite_s station_parabola;
    struct game_sprite_s station_hook;
    struct game_hit_data_s *station_hit;
        
    struct game_sprite_s ship_white;
    struct game_sprite_s ship_colour;
    struct game_sprite_s ship_thrust;
    struct game_sprite_s ship_shot;    
    struct game_hit_data_s *ship_hit;
    struct game_hit_data_s *ship_shot_hit;
        
};

extern struct game_vars_s game_vars;
extern struct game_sprites_s game_sprite;

// prototypes
void game_reset_player(int, int);
void game_logic(void);
void game_drawgrid(void);
void game_drawsprites(void);
void game_drawparticles(void);
void game_drawtitle(void);
void game_drawstatus(float);
void game_advance(float *, float *, int, int);




