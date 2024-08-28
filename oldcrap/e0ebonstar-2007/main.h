float main_mouse_x(void);
float main_mouse_y(void);
Uint8 *main_keys(void);

int main_control(int, int);


#define GAMESTATE_NONE 0
#define GAMESTATE_EXIT 1
#define GAMESTATE_MENU 2
#define GAMESTATE_GAME 3
#define GAMESTATE_TEST 4
#define GAMESTATE_EDITOR 5
#define GAMESTART_SPLASH 6

#define CONTROL_LEFT 0
#define CONTROL_RIGHT 1
#define CONTROL_THRUST 2
#define CONTROL_FIRE 3

