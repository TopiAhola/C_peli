//TODO:


//

#if !defined(MYGAME_H)

struct game_backbuffer{
    void * p_memory;
    int width;
    int height;
    int bytes_per_pixel;
    int pitch;
};


//time (for variable rate rendering), inputs, pointer to bitmap and sound buffer
static void game_update_and_render(game_backbuffer * bitmap, int y_input = 0 , int x_input = 0);

#define MYGAME_H

#endif