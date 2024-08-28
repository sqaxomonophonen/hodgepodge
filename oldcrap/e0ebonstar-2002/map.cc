// notes:

// tiles are 32.0 x 32.0 units

// polygons vs triangles
// polygons are triangulated upon loading a level (to avoid drawing concave polys)
// still the polygons are kept since some polygons have borders that need to be drawn
//  seperately. (polygon specific normals are needed for that)

// drawing order (top = first)
//  backdrop
//  polygon back
//  tile back
//  (FOV shadow)
//  objects (outside FOV shadow)
//  liquids
//  polygon front
//  tile front

// 2d integer vector
struct vec2i_s
{
    int x,y;
};

// 2d floating point vectore
struct vec2f_s
{
    float x,y;
};

struct box4i_s
{
    struct vec2i_s ul,lr; // upperleft & lowerright
};

struct tile_s
{
    int id; // id number of the tile
};

struct polygon_s
{
    int id; // polygon id (appearance.. how it looks)
    int z;  // z used for internal draw ordering (small z's first)
    int vertex_n;
    struct vec2i_s *vertex;
    struct vec2f_s *normal; // are post-calculated (when loading a map)
};

// triangles - they always belong to a polygon.. (and share vertices and id with it)
struct tri_s
{
    int id;
    int z;  // z used for internal draw ordering (small z's first)
    struct vec2i_s vertex[3];
};

struct liquid_s
{
    int type; // type of liquid
    struct box4i_s box; // meassured in units (16:16)
};

struct zone_s
{
    int type; // type of zone
    int *value; // zone specific values
    struct box4i_s box; // meassured in units (16:16)
};

struct object_s
{
    int id;   // id number of the object
    int *value; // object specific value
    struct vec2i_s coord; // object coordinate
};

// contains static (non-changing) map data
struct map_static_s
{
    // DESCRIPTIONAL
    
    // text
    char *name;
    char *author;
    char *description;
	
	
    // OBLIGATORY
    
    // map type (type of game.. ctf, deathmatch, etc?)
    int type;
    
    // backdrop (background.. just for viewing pleasure)
    int backdrop_id;              // an id (number) determining what hardcoded backdrop to use
    struct vec2f_s backdrop_scale;       // how much to show of the texture on the screen (1.0 = 100%)
    struct vec2f_s backdrop_matrix[3];   // translation matrix (for scaling and translating position)
	
    // map boundary box (meassured in tiles)
    struct box4i_s boundary_box;
    
    
    // STRUCTURE
    
    // tiles = mostly indoors structure (number of tiles is the area of the boundary box)
    // back tiles are background and front tiles are foreground (structure)
    struct tile_s *back_tile, *front_tile;
    
    // polygons = mostly outdoors structure
    int back_polygon_n, front_polygon_n; // number of polygons)
    struct polygon_s *back_polygon, *front_polygon;
	
	
    // ZONES
    
    // liquid = visible box with water.. slime.. lava.. etc..
    int liquid_n; // number of liquid zones
    struct liquid_s *liquid;
    
    // zone = invisible box with some purpose
    int zone_n; // number of zones
    struct zone_s *zone;
    
    
    // OBJECTS
    
    // objects = spawn points.. flags.. vehicles.. etc..
    int object_n;
    struct object_s *object;
	
	
    // POST-CALCULATED (calculated when loading a level)
    
    // structure triangles
    int tri_n;
    struct tri_s *tri;
    
    // structure bsp tree (collision detection)
	
};
