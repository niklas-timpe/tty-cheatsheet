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

WINDOW *create_newwin(int height, int width, int starty, int startx);
WINDOW *create_newwin_boxless(int height, int width, int starty, int startx);
void destroy_win(WINDOW *local_win);
static void draw_floppy(void);
static int file_exists(char *fname);
static void create_db(char *fname);
static char *get_full_db_path(const char *fname);
static void init(void);
static void ncurses_setup(void);
void print_search(WINDOW *search_bar);

int main(int argc, char *argv[]) {
  if (argc > 1) {
    fprintf(stderr, "Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

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

  /* Print Search: */
  print_search(search_bar);

  int ch;
  char input[128];
  size_t len = 0;
  do {
    ch = wgetch(search_bar);

    // 8 is the ascii backspace character and 127 is the delete character
    if (ch == 8 || ch == 127) {
      len = len > 0 ? len - 1 : 0;
      input[len] = '\0';

    } else {
      input[len++] = ch;
      input[len] = '\0';
    }

    werase(search_bar);
    print_search(search_bar);
    mvwprintw(search_bar, 1, 9, input);
    wrefresh(search_bar);
  } while (1);

  getch();
  endwin();

  return 0;
}

void print_search(WINDOW *search_bar) {
  wattron(search_bar, A_ITALIC | A_BLINK | A_DIM);
  mvwprintw(search_bar, 1, 1, "Search: ");
  wrefresh(search_bar);
  wattroff(search_bar, A_ITALIC | A_BLINK | A_DIM);
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

static void draw_floppy(void) {
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

static int file_exists(char *fname) {
  if (access(fname, F_OK) == 0) {
    return 1;
  } else {
    return 0;
  }
}

static void create_db(char *fname) {
  FILE *fp = fopen(fname, "w");
  if (fp == NULL) {
    fprintf(stderr, "Error: Could not create database file: %s\n", DATABASE);
    exit(EXIT_FAILURE);
  }
  fclose(fp);
}

static char *get_full_db_path(const char *fname) {
  char *file;
  file = malloc(strlen(getenv("HOME")) + strlen(fname) + 1);
  strcpy(file, getenv("HOME"));
  strcat(file, fname);
  return file;
}

static void init(void) {
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

static void ncurses_setup(void) {
  initscr();
  cbreak();
  keypad(stdscr, TRUE);
  noecho();
}
