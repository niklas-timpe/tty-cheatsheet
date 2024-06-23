#define _GNU_SOURCE /* Required for getline with -std=c99 */
#include "submodules/fuzzy-match/fuzzy_match.h"
#include <dirent.h>
#include <errno.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char *DATABASE = "/.tty_cheatsheet";

enum mode { TOTAL, TOPIC };

WINDOW *create_newwin(int height, int width, int starty, int startx);
WINDOW *create_newwin_boxless(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
void update_search_windows(WINDOW *search_win, WINDOW *search_bar, enum mode mode, char *user_input, size_t idx_cursor);
size_t max(size_t a, size_t b);
void draw_floppy(void);
int file_exists(char *fname);
void create_db(char *fname);
char *get_full_db_path(const char *fname);
void init(void);
void ncurses_setup(void);
void erase_win_keep_box(WINDOW *local_win);
int print_search(WINDOW *search_bar, enum mode* mode);

const char entire_search[] = "Total Search: ";
const char topic_search[] = "Topic Search: ";


int main(int argc, char *argv[]) {
    if (argc > 1) {
        fprintf(stderr, "Usage: %s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    enum mode mode = TOPIC;

    init();
    ncurses_setup();

    /* The search window */
    int search_height = LINES - 2;
    int search_width = COLS / 3;
    int search_starty =
        (LINES - search_height) / 2; /* Calculating for a center placement */
    int search_startx = 1;
    mvprintw(0, 2, "Press Ctl-C to exit");
    refresh();
    WINDOW *search_win =
        create_newwin(search_height, search_width, search_starty, search_startx);

    /* The search bar */
    int search_bar_height = 3;
    int search_bar_width = search_width - 4;
    int search_bar_starty = search_starty + search_height - search_bar_height - 1;
    int search_bar_startx = search_startx + 2;
    WINDOW *search_bar =
        create_newwin_boxless(search_bar_height, search_bar_width,
                              search_bar_starty, search_bar_startx);
    refresh();

    /* EDITOR BOX */
    int editor_height = search_height;
    int editor_width = COLS - search_width - 3;
    int editor_starty = search_starty;
    int editor_startx = search_startx + search_width + 1;
    WINDOW *editor_win =
        create_newwin(editor_height, editor_width, editor_starty, editor_startx);
    refresh();

    /* Print Topic Search: */
    print_search(search_bar, &mode);

    int ch;
    char input[128];
    size_t idx = 0;
    size_t len = 0;

    do {
        ch = wgetch(search_bar);
        switch (ch) {
            // Ctl-A
            case 1:
                idx = 0;
                wrefresh(search_bar);
                break;
            // Ctl-E
            case 5:
                idx = strlen(input);
                wrefresh(search_bar);
                break;

            case 9:
                switch (mode) {
                    case TOTAL:
                        mode = TOPIC;
                        break;
                    case TOPIC:
                        mode = TOTAL;
                        break;
                }
                break;

            // backspace
            case 8:
                if (idx > 0) {
                    idx--;
                } else {
                    idx = 0;
                    continue;
                }
                if (idx != len) {
                    for (size_t i = idx; i < len; i++) {
                        input[i] = input[i+1];
                    }
                    len--;
                }
                break;
            
            // Ctl-K
            case 11:
                len = idx;
                break;

            // standard chars
            // The condition should prevent weird keys to do something. 
            // But is doesn't. Also the no echo doesn't work..
            case 32 ... 126: // The ... is a GNU extension and not supported by the standard
                if (idx != len) {
                    // shift right chars from idx one to the right
                    for (size_t i = len; i > idx; i--) {
                        input[i] = input[i-1];
                    }
                }
                input[idx++] = ch;
                len++;
                {
                    int bar_text_len = print_search(search_bar, &mode);
                    wmove(search_bar, 1, bar_text_len+1+strlen(input));

                }
                break;

            // delete
            case 127:
                if (idx > 0) {
                    idx--;
                } else {
                    idx = 0;
                    continue;
                }
                if (idx != len) {
                    for (size_t i = idx; i < len; i++) {
                        input[i] = input[i+1];
                    }
                    len--;
                }
                break;

            default:
                continue;
        }

        input[len] = '\0';

        update_search_windows(search_win, search_bar, mode, input, idx);


        // Refresh search related windows
        wrefresh(search_win);
        wrefresh(search_bar);
    } while (1);

    getch();
    endwin();

    return 0;
}

void update_search_windows(
    WINDOW *search_win, 
    WINDOW *search_bar, 
    enum mode mode,
    char *user_input,
    size_t idx_cursor
) {
    // Search window: Needs to happen first, otherwise the search bar will disappear
    erase_win_keep_box(search_win);
    int search_height, search_width;
    getmaxyx(search_win, search_height, search_width);
    int search_starty, search_startx;
    getbegyx(search_win, search_starty, search_startx);
    int search_bar_starty, search_bar_startx;
    getbegyx(search_bar, search_bar_starty, search_bar_startx);

    mvwprintw(search_win, search_height+search_starty-6, search_bar_startx, "ficken");

    // Search bar
    werase(search_bar);

    size_t input_len = print_search(search_bar, &mode);
    mvwprintw(search_bar, 1, input_len+1, user_input);
    wmove(search_bar, 1, input_len + 1 + idx_cursor);
}

int print_search(WINDOW *search_bar, enum mode* mode) {
    switch (*mode) {
        case TOTAL:
            wattron(search_bar, A_ITALIC | A_BLINK | A_DIM);
            mvwprintw(search_bar, 1, 1, entire_search);
            wrefresh(search_bar);
            wattroff(search_bar, A_ITALIC | A_BLINK | A_DIM);
            return strlen(entire_search);
        case TOPIC:
            wattron(search_bar, A_ITALIC | A_BLINK | A_DIM);
            mvwprintw(search_bar, 1, 1, topic_search);
            wrefresh(search_bar);
            wattroff(search_bar, A_ITALIC | A_BLINK | A_DIM);
            return strlen(topic_search);
    }
}

WINDOW *create_newwin(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    box(local_win, 0, 0); /* 0, 0 gives default characters
                         * for the vertical and horizontal
                         * lines			*/
    wrefresh(local_win);  /* Show that box 		*/

    return local_win;
}

void erase_win_keep_box(WINDOW *local_win) {
    werase(local_win);
    box(local_win, 0, 0);
    wrefresh(local_win);
}

WINDOW *create_newwin_boxless(int height, int width, int starty, int startx) {
    WINDOW *local_win;

    local_win = newwin(height, width, starty, startx);
    wrefresh(local_win); /* Show that box 		*/

    return local_win;
}

void destroy_win(WINDOW *local_win) {
    /* box(local_win, ' ', ' '); : This won't produce the desired
   * result of erasing the window. It will leave it's four corners
   * and so an ugly remnant of window.
   */
    wborder(local_win, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    /* The parameters taken are
   * 1. win: the window on which to operate
   * 2. ls: character to be used for the left side of the window
   * 3. rs: character to be used for the right side of the window
   * 4. ts: character to be used for the top side of the window
   * 5. bs: character to be used for the bottom side of the window
   * 6. tl: character to be used for the top left corner of the window
   * 7. tr: character to be used for the top right corner of the window
   * 8. bl: character to be used for the bottom left corner of the window
   * 9. br: character to be used for the bottom right corner of the window
   */
    wrefresh(local_win);
    delwin(local_win);
}

void draw_floppy(void) {
    printf("\n\n");
    printf("\t\t___________________________.\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|[]|---------------------|[]||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|                     |;;||\n");
    printf("\t\t|;;|_____________________|;;||\n");
    printf("\t\t|;;;;;;;;;;;;;;;;;;;;;;;;;;;||\n");
    printf("\t\t|;;;;;;_______________ ;;;;;||\n");
    printf("\t\t|;;;;;|  ___          |;;;;;||\n");
    printf("\t\t|;;;;;| |;;;|         |;;;;;||\n");
    printf("\t\t|;;;;;| |;;;|         |;;;;;||\n");
    printf("\t\t|;;;;;| |;;;|         |;;;;;||\n");
    printf("\t\t|;;;;;| |;;;|         |;;;;;||\n");
    printf("\t\t|;;;;;| |___|         |;;;;;||\n");
    printf("\t\t\\_____|_______________|_____||\n");
    printf("\t\t ~~~~~^^^^^^^^^^^^^^^^^~~~~~~ \n");
    printf("\n\n");
}

int file_exists(char *fname) {
    if (access(fname, F_OK) == 0) {
        return 1;
    } else {
        return 0;
    }
}

void create_db(char *fname) {
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not create database file: %s\n", DATABASE);
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

char *get_full_db_path(const char *fname) {
    char *file;
    file = malloc(strlen(getenv("HOME")) + strlen(fname) + 1);
    strcpy(file, getenv("HOME"));
    strcat(file, fname);
    return file;
}

void init(void) {
    /* Check if database file exists */
    /* If not, ask to create it      */
    char *full_path = get_full_db_path(DATABASE);
    printf("Database directory: %s\n", full_path);

    DIR *dir = opendir(full_path);
    if (dir) {
        /* Directory exists. */
        closedir(dir);
    } else {
        /* Directory does not exist. */
        int result = mkdir(full_path, 0755);
        if (result != -1) {
            printf("Creating new database ...\n");
            sleep(1);
            draw_floppy();
            printf("DONE! LET'S GET STARTED\n");
            sleep(1);
        }
    }
}

void ncurses_setup(void) {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, FALSE);
}

size_t max(size_t a, size_t b) {
    return a > b ? a : b;
}
