#define _POSIX_C_SOURCE 199309L
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<sys/select.h>
#include<signal.h>
#include<stdlib.h>
#include<time.h>

/* terminal input handling: switch to non-canonical, no-echo mode */
static struct termios orig_termios;
static int input_mode_inited = 0;

/* forward declare screen restore so restore_input_mode can call it */
void restore_screen(void);

void restore_input_mode(void)
{
    if (!input_mode_inited) return;
    /* best-effort restore; ignore errors */
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
    input_mode_inited = 0;
    /* also restore screen (show cursor) */
    restore_screen();
}

void init_input_mode(void)
{
    struct termios newt;
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return;
    newt = orig_termios;
    /* disable canonical mode and echo */
    newt.c_lflag &= ~(ICANON | ECHO);
    /* return each byte, no timeout */
    newt.c_cc[VMIN] = 0;
    newt.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &newt) == 0) {
        input_mode_inited = 1;
        /* ensure we restore on normal exit */
        atexit(restore_input_mode);
    }
}

/* Signal handler: restore terminal then exit immediately */
void handle_signal(int sig)
{
    restore_input_mode();
    /* use _exit to avoid unsafe behaviour in signal handler */
    _exit(128 + sig);
}

//data
char map[12][12];

typedef struct snake
{
    int x;
    int y;
    struct snake* next;
}snake;

typedef struct status
{
    snake* head;
    snake* tail;
    int food_x;
    int food_y;
    int flag;
    char direction;
} status;


void food_generate(status* game)
{
    int is_empty=0;
    int food_x;
    int food_y;
    snake* current;
    while(is_empty==0)
    {
        food_x=rand()%10+1;
        food_y=rand()%10+1;
        is_empty=1;
        current = game->head;
        while(current!=NULL)
        {
            /* collision if both coordinates match */
            if(current->x==food_x && current->y==food_y)
            {
                is_empty=0;
                break;
            }
            current=current->next;
        }
    }

    /* assign generated food position to game */
    game->food_x = food_x;
    game->food_y = food_y;

}

void map_print()
{
    /* move cursor to top-left so we overwrite previous frame (in-place refresh) */
    const char *home = "\x1b[H"; /* ANSI: cursor home */
    write(STDOUT_FILENO, home, 3);

    for(int i=0;i<12;i++)
    {
        for(int j=0;j<12;j++)
        {
            printf("%c",map[i][j]);
        }
        printf("\n");

    }
}

/* initialize screen: clear and hide cursor */
void init_screen(void)
{
    /* clear screen */
    const char *clear = "\x1b[2J"; /* ANSI clear screen */
    write(STDOUT_FILENO, clear, 4);
    /* move to home and hide cursor */
    const char *home_hide = "\x1b[H\x1b[?25l";
    write(STDOUT_FILENO, home_hide, 8);
    fflush(stdout);
}

/* restore screen state: show cursor and move to next line */
void restore_screen(void)
{
    const char *show = "\x1b[?25h"; /* show cursor */
    write(STDOUT_FILENO, show, 6);
    /* move cursor down to avoid overwriting prompt */
    const char *down = "\n";
    write(STDOUT_FILENO, down, 1);
    fflush(stdout);
}

void map_init()
{
    // 初始化地图
    for(int i = 0; i < 12; i++)
    {
        for(int j = 0; j < 12; j++)
        {
            // 设置边界为墙，内部为空地
            if(i == 0 || i == 11 || j == 0 || j == 11)
                map[i][j] = '#';
            else
                map[i][j] = ' ';
        }
    }
}

void map_generate(status* game)
{
    map_init();
    
    snake* current=game->head;
    while(current!=NULL)
    {
        map[current->x][current->y]='o';
        current=current->next;
    }
    map[game->head->x][game->head->y]='O';
    map[game->food_x][game->food_y]='f';
}


void update(status* game)
{
    int x=game->head->x;
    int y=game->head->y;   
    
    /* read requested direction (non-blocking); keep previous to prevent immediate reverse */
    char prev_dir = game->direction;

    /* poll stdin for available input */
    int c = -1;
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0; /* no wait */
    if (select(STDIN_FILENO+1, &rfds, NULL, NULL, &tv) > 0) {
        char ch;
        ssize_t r = read(STDIN_FILENO, &ch, 1);
        if (r > 0) {
            /* handle ANSI arrow sequences: ESC [ A/B/C/D */
            if (ch == '\x1b') {
                /* try to read two more bytes if available */
                char seq[2] = {0,0};
                ssize_t r1 = read(STDIN_FILENO, &seq[0], 1);
                ssize_t r2 = read(STDIN_FILENO, &seq[1], 1);
                if (r1 > 0 && r2 > 0 && seq[0] == '[') {
                    if (seq[1] == 'A') c = 'w';
                    else if (seq[1] == 'B') c = 's';
                    else if (seq[1] == 'C') c = 'd';
                    else if (seq[1] == 'D') c = 'a';
                }
            } else {
                c = (int)ch;
            }
        }
    }

    if (c != -1) game->direction = (char)c;

    //move the snake
    switch (game->direction) {
        case 'w':
        case 'W':
            /* move up: decrease x (row) */
            if (!(prev_dir=='s' || prev_dir=='S')) x--;
            break;
        case 's':
        case 'S':
            /* move down: increase x (row) */
            if (!(prev_dir=='w' || prev_dir=='W')) x++;
            break;
        case 'a':
        case 'A':
            /* move left: decrease y (col) */
            if (!(prev_dir=='d' || prev_dir=='D')) y--;
            break;
        case 'd':
        case 'D':
            /* move right: increase y (col) */
            if (!(prev_dir=='a' || prev_dir=='A')) y++;
            break;
        case 'q':
        case 'Q':
            game->flag=0;
            break;
        }

    //detect eat_food
    int eat_food=0;
    if(x==game->food_x&&y==game->food_y)
    {
        eat_food=1;
    }

    //detect alive (valid playable coords are 1..10)
    int alive=1;
    if(x>=11 || x<=0)
    {
        alive=0;
    }
    if(y>=11 || y<=0)
    {
        alive=0;
    }
    
    //update the status
    if(alive==0)
    {
        game->flag=0;
    }
    
    snake* new_head;
    new_head = malloc(sizeof(snake));
    if (!new_head) {
        perror("malloc");
        game->flag = 0;
        return;
    }
    new_head->x=x;
    new_head->y=y;
    new_head->next=game->head;
    game->head=new_head;
    if(eat_food==0)
    {
        //remove last tail node 
        if (game->head->next == NULL) 
        {
            //single element, keep it 
            game->tail = game->head;
        } 
        else 
        {
            snake* cur = game->head;
            while(cur->next && cur->next->next) 
            {
                cur = cur->next;
            }
            //cur->next is tail
            free(cur->next);
            cur->next = NULL;
            game->tail = cur;
        }
    }
    else food_generate(game);

    //update the map
    map_generate(game);
    map_print();
    }

int main()
{
    //initialize
    status game_var;
    status* game = &game_var;

    /* seed RNG */
    srand((unsigned)time(NULL));

    game->direction='w';
    game->flag=1;
    game->food_x = rand()%10+1;
    game->food_y = rand()%10+1;

    /* create initial head */
    game->head = malloc(sizeof(snake));
    if (!game->head) {
        perror("malloc");
        return 1;
    }
    game->head->x = 5;
    game->head->y = 5;
    game->head->next = NULL;
    game->tail = game->head;

    map_generate(game);
    map_print();
    /* set terminal to non-canonical mode so we can read keys without blocking */
    init_input_mode();
    /* initialize screen (clear and hide cursor) for in-place refresh */
    init_screen();

    /* register signal handlers to restore terminal on interruption */
    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    while(game->flag==1)
    {
        update(game);
        /* sleep 200ms */
        struct timespec ts = {0, 200000000};
        nanosleep(&ts, NULL);
    }
    
    return(0);
}