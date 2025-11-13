#define _POSIX_C_SOURCE 199309L
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<time.h>

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
    for(int i=0;i<12;i++)
    {
        for(int j=0;j<12;j++)
        {
            printf("%c",map[i][j]);
        }
        printf("\n");

    }
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
    
    /* read requested direction (blocking); keep previous to prevent immediate reverse */
    char prev_dir = game->direction;
    int c = getchar();
    if (c != EOF) game->direction = (char)c;

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
        /* remove last tail node */
        if (game->head->next == NULL) {
            /* single element, keep it */
            game->tail = game->head;
        } else {
            snake* cur = game->head;
            while(cur->next && cur->next->next) {
                cur = cur->next;
            }
            /* cur->next is tail */
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

    while(game->flag==1)
    {
        update(game);
        /* sleep 200ms */
        struct timespec ts = {0, 200000000};
        nanosleep(&ts, NULL);
    }
    
    return(0);
}