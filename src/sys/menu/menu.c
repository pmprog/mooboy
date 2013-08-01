#include "menu.h"
#include "rom.h"
#include "options.h"
#include "sys/sys.h"
#include "sys/sdl/input.h"
#include "sys/sdl/video.h"
#include "sys/sdl/state.h"
#include <assert.h>
#include "util.h"
#include <SDL/SDL.h>

#define NUM_ENTRIES 3

#define LABEL_RESUME     0
#define LABEL_LOAD_ROM   1
#define LABEL_RESET      2
#define LABEL_OPTIONS    3
#define LABEL_LOAD_STATE 4
#define LABEL_SAVE_STATE 5
#define LABEL_QUIT       6

static menu_list_t *list = NULL;
static int load_slot = 0;
static int save_slot = 0;

static void set_slot(int label, int i) {
    if(i < 0 || i > 9) {
        return;
    }

    menu_listentry_val_int(list, label, i);

    if(label == LABEL_LOAD_STATE) {
        load_slot = i;
    }
    else {
        save_slot = i;
    }
}

static void load_state() {
    char file[256];
    sprintf(file, "%s.sav%i", sys.rompath, load_slot);
    if(state_load(file) == 0) {
        sys.in_menu = 0;
    }
}

static void save_state() {
    char file[256];
    sprintf(file, "%s.sav%i", sys.rompath, save_slot);
    state_save(file);
}

static void setup() {
    menu_listentry_visible(list, LABEL_RESUME, sys.rom_loaded);
    menu_listentry_visible(list, LABEL_RESET, sys.rom_loaded);
    menu_listentry_visible(list, LABEL_LOAD_STATE, sys.rom_loaded);
    menu_listentry_visible(list, LABEL_SAVE_STATE, sys.rom_loaded);
    menu_list_select_first(list);
}

static void draw() {
    SDL_Rect prevr = {250, 10, 200, 180};

    SDL_FillRect(SDL_GetVideoSurface(), NULL, 0);
    menu_draw_list(list);
    if(sys.rom_loaded) {
        video_render(SDL_GetVideoSurface(), prevr);
    }
    SDL_Flip(SDL_GetVideoSurface());
}

static void menu_input_event(int type, int key) {
    menu_list_input(list, type, key);

    if(type == SDL_KEYDOWN) {
        int label = menu_list_selected_id(list);

        switch(key) {
            case KEY_ACCEPT:
                switch(label) {
                    case LABEL_RESUME: sys.in_menu = 0; break;
                    case LABEL_LOAD_ROM: menu_rom(); break;
                    case LABEL_LOAD_STATE: load_state(); break;
                    case LABEL_SAVE_STATE: save_state(); break;
                    case LABEL_OPTIONS: menu_options(); break;
                    case LABEL_QUIT: sys.running = 0; break;
                }
            break;
        }
        switch(label) {
            case LABEL_LOAD_STATE:
                set_slot(label, key == SDLK_LEFT ? load_slot-1 : key == SDLK_RIGHT ? load_slot+1 : load_slot);
            break;
            case LABEL_SAVE_STATE:
                set_slot(label, key == SDLK_LEFT ? save_slot-1 : key == SDLK_RIGHT ? save_slot+1 : save_slot);
            break;
        }

        if(key == KEY_BACK) {
            sys.in_menu = 0;
        }
    }
}


void menu_init() {
    menu_util_init();
    menu_rom_init();
    menu_options_init();

    list = menu_new_list("Main Menu");

    menu_new_listentry(list, "Resume", LABEL_RESUME);
    menu_new_listentry(list, "Load ROM", LABEL_LOAD_ROM);
    menu_new_listentry(list, "Reset", LABEL_RESET);
    menu_new_listentry(list, "Load state", LABEL_LOAD_STATE);
    menu_new_listentry(list, "Save state", LABEL_SAVE_STATE);
    menu_new_listentry(list, "Options", LABEL_OPTIONS);
    menu_new_listentry(list, "Quit", LABEL_QUIT);

    set_slot(LABEL_LOAD_STATE, 0);
    set_slot(LABEL_SAVE_STATE, 0);
}


void menu_close() {
    menu_free_list(list);
    menu_rom_close();
    menu_options_close();
}

void menu() {
    sys_pause();
    setup();

    while(sys.running && sys.in_menu) {
        draw();
        sys_handle_events(menu_input_event);
        menu_list_update(list);
    }
    sys_run();
}


