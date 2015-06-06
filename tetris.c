/*!
 * @file tetris.c
 * @brief Console tetris
 * @author koturn
 */
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _MSC_VER
#  include <sys/timeb.h>
#else
#  include <sys/time.h>
#  include <unistd.h>
#endif
#include <termutil.h>


#ifndef __UNUSED__
#  if defined(__cplusplus)
#    define __UNUSED__(x)
#  elif defined(__GNUC__)
#    define __UNUSED__(x)  __UNUSED___ ## x __attribute__((unused))
#  elif defined(_MSC_VER)
#    define __UNUSED__(x)  __pragma(warning(suppress: 4100)) x
#  elif defined(__LCLINT__)
#    define __UNUSED__(x)  /*@unused@*/ x
#  else
#    define __UNUSED__(x)  x
#  endif
#endif

#ifdef _MSC_VER
#  define msec_sleep(x)  Sleep(x)
#else
#  define msec_sleep(x)  usleep(x * 1000)
#endif

#define LENGTHOF(array)  (sizeof(array) / sizeof((array)[0]))


#define N_NEXT_BLOCK  3

#define STAGE_WIDTH   12  /*!< Width of stage */
#define STAGE_HEIGHT  21  /*!< Height of stage */

#define BLOCK_WIDTH   4  /*!< Width of blocks */
#define BLOCK_HEIGHT  4  /*!< Height of blocks */
#define N_BLOCK       7  /*!< The number of block types */

#define SPACE  0  /*!< Indicate there is nothing */
#define WALL   9  /*!< Indicate there is wall */

#define FIELD_X   0  /*!< X-coordinate of player's field */
#define FIELD_Y   2  /*!< Y-coordinate of player's field */
#define SCORE_X  36  /*!< X-coordinate of player's score */
#define SCORE_Y  10  /*!< Y-coordinate of player's score */

#define NEXT_BLOCK_X  23
static const int NEXT_BLOCK_YS[] = {3, 10, 17};

#define SCORE_LEN  10  /*!< Buffer size of score label */
#define SCORE1  100
#define SCORE2  300
#define SCORE3  500
#define SCORE4  1000

#define CURSOR_X  0  /*!< X-coordinate of temporary cursor position */
#define CURSOR_Y  0  /*!< Y-coordinate of temporary cursor position */

#define TIME_X    36  /*!< X-coordinate of time label */
#define TIME_Y     5  /*!< Y-coordinate of time label */
#define TIME_LEN   5  /*!< Buffer size of time label */

#ifndef TRUE
#  define TRUE  1
#endif
#ifndef FALSE
#  define FALSE  0
#endif


typedef struct {
  int x;
  int y;
} Position;

typedef struct {
  int block;
  int next_block;
  int score;
  int time;
} UpdateFlag;

/*! Key-codes */
enum keycode {
  CTRL_B = 0x02,  /*!< Ctrl-B */
  CTRL_F = 0x06,  /*!< Ctrl-F */
  CTRL_N = 0x0e   /*!< Ctrl-N */
};


/*! The direction of rotation of the block */
typedef enum {
  RIGHT,  /*!< Indicates right-handed rotation */
  LEFT    /*!< Indicates right-handed rotation */
} Direction;


unsigned int next_idx = 0;
static unsigned char next_blocks[N_NEXT_BLOCK];
static unsigned char stage[STAGE_HEIGHT][STAGE_WIDTH];
static unsigned char block[BLOCK_HEIGHT][BLOCK_WIDTH];
static unsigned char field[STAGE_HEIGHT][STAGE_WIDTH];

/*! Seven blocks */
static const unsigned char block_list[N_BLOCK][BLOCK_HEIGHT][BLOCK_WIDTH] = {
  {{0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 0, 0}},
  {{0, 0, 0, 0},
    {0, 2, 2, 0},
    {0, 2, 0, 0},
    {0, 2, 0, 0}},
  {{0, 0, 3, 0},
    {0, 3, 3, 0},
    {0, 3, 0, 0},
    {0, 0, 0, 0}},
  {{0, 4, 0, 0},
    {0, 4, 4, 0},
    {0, 0, 4, 0},
    {0, 0, 0, 0}},
  {{0, 0, 0, 0},
    {0, 5, 0, 0},
    {5, 5, 5, 0},
    {0, 0, 0, 0}},
  {{0, 0, 0, 0},
    {0, 6, 6, 0},
    {0, 6, 6, 0},
    {0, 0, 0, 0}},
  {{0, 0, 0, 0},
    {0, 7, 7, 0},
    {0, 0, 7, 0},
    {0, 0, 7, 0}}};

static UpdateFlag update_flag = {FALSE, FALSE, FALSE, FALSE};
static Position block_pos = {0, 4};    /*!< Block position */
static int    score = 0;               /*!< Player's score */
static time_t gametime = (time_t) -1;  /*!< Player's score */
static int    is_gameover = FALSE;     /*!< Flag of gameover */

static void
initialize(void);

static void
control_block(void);

static void
drop_block(void);

static void
create_block(void);

static void
change_background_color(int color_nr);

static int
check_overlap(int x, int y);

static void
move_block(int new_x, int new_y);

static int
turn_block(Direction direction);

static void
lock_block(void);

static void
check_lines(void);

static void
update_screen(void);

static void
print_labels(void);

static void
print_wall(void);

static void
print_field(unsigned char field[STAGE_HEIGHT][STAGE_WIDTH], int x);

static void
print_next_blocks(void);

static void
print_score(int _score);

static void
print_time(time_t time);

static time_t
get_utc(void);

static void
sigint_handler(int sig);




/*!
 * @brief An entry point of this program
 * @return exit-status
 */
int
main(void)
{
  int cnt = 1;
  time_t base_time = time(NULL);

  initialize();
  while (!is_gameover) {
    update_screen();
    control_block();
    if ((cnt = (cnt + 1) % 32) == 0) {
      drop_block();
    }
    if (gametime != time(NULL) - base_time) {
      gametime = time(NULL) - base_time;
      update_flag.time = TRUE;
    }
    tu_move(CURSOR_Y, CURSOR_X);
    msec_sleep(20);
  }
  printf("GAME OVER!!!!\n");

  tu_refresh();
  msec_sleep(2000);
  return EXIT_SUCCESS;
}


/*!
 * @brief Initialize game configurations
 */
static void
initialize(void)
{
  int i, j;

  srand((unsigned int) get_utc());
  tu_init();
  tu_setcur(0);
  tu_clear();
  tu_cbreak();
  tu_noecho();
  tu_nonblocking();
  atexit(tu_cleanup);
  signal(SIGINT, sigint_handler);
  print_labels();
  for (i = 0; i < STAGE_HEIGHT; i++) {
    for (j = 0; j < STAGE_WIDTH; j++) {
      if ((j == 0) || (j == STAGE_WIDTH - 1) || (i == STAGE_HEIGHT - 1)) {
        field[i][j] = stage[i][j] = WALL;
      } else {
        field[i][j] = stage[i][j] = SPACE;
      }
    }
  }
  for (i = 0; i < (int) LENGTHOF(next_blocks); i++) {
    next_blocks[i] = (unsigned char) (rand() % N_BLOCK);
  }
  create_block();
  print_wall();
  print_next_blocks();
  print_field(field, FIELD_X);
}


/*!
 * @brief Control block with key input
 */
static void
control_block(void)
{
  switch (tu_getch()) {
    /* ----- Like vi ----- */
    case 'l':
      if (!check_overlap(block_pos.x + 1, block_pos.y)) {
        move_block(block_pos.x + 1, block_pos.y);
      }
      break;
    case 'h':
      if (!check_overlap(block_pos.x - 1, block_pos.y)) {
        move_block(block_pos.x - 1, block_pos.y);
      }
      break;
    case 'j':
      if (!check_overlap(block_pos.x, block_pos.y + 1)) {
        move_block(block_pos.x, block_pos.y + 1);
      }
      break;
    /* ----- Like Emacs ----- */
    case CTRL_F:
      if (!check_overlap(block_pos.x + 1, block_pos.y)) {
        move_block(block_pos.x + 1, block_pos.y);
      }
      break;
    case CTRL_B:
      if (!check_overlap(block_pos.x - 1, block_pos.y)) {
        move_block(block_pos.x - 1, block_pos.y);
      }
      break;
    case CTRL_N:
      if (!check_overlap(block_pos.x, block_pos.y + 1)) {
        move_block(block_pos.x, block_pos.y + 1);
      }
      break;
    /* ----- Rotate block ----- */
    case 'a':
      turn_block(RIGHT);
      break;
    case 's':
      turn_block(LEFT);
      break;
    case ' ':
      turn_block(RIGHT);
      break;
  }
}


/*!
 * @brief Drop a block
 */
static void
drop_block(void)
{
  if (!check_overlap(block_pos.x, block_pos.y + 1)) {
    move_block(block_pos.x, block_pos.y + 1);
  } else {
    lock_block();
    create_block();
    update_flag.block = TRUE;
    update_flag.next_block = TRUE;
  }
}


/*!
 * @brief Create block
 */
static void
create_block(void)
{
  int i, j;
  block_pos.y = 0;
  block_pos.x = 4;

  memcpy((void *) block, (const void *) block_list[next_blocks[next_idx]], sizeof(block));
  next_blocks[next_idx] = (unsigned char) (rand() % N_BLOCK);
  next_idx = (next_idx + 1) % LENGTHOF(next_blocks);
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      /* Gameover if locked block is already exist on in initial position */
      if (stage[i][j + 4] != 0) {
        is_gameover = TRUE;
        return;
      }
      field[i][j + 4] = block[i][j];
    }
  }
}


/*!
 * @brief Change background color
 * @param [in] color_nr  Color number
 */
static void
change_background_color(int color_nr)
{
  switch (color_nr) {
    case SPACE:
      tu_set_background(TU_DEFAULT_COLOR);
      break;
    case 1:
      tu_set_background(TU_GRAY);
      break;
    case 2:
      tu_set_background(TU_RED);
      break;
    case 3:
      tu_set_background(TU_GREEN);
      break;
    case 4:
      tu_set_background(TU_BLUE);
      break;
    case 5:
      tu_set_background(TU_YELLOW);
      break;
    case 6:
      tu_set_background(TU_MAGENTA);
      break;
    case 7:
      tu_set_background(TU_CYAN);
      break;
  }
}


/*!
 * @brief Check whether the block overlaps
 * @param [in] x  X-coordinate of base position
 * @param [in] y  Y-coordinate of base position
 * @return Return 1, if block overlaps, otherwise return 0.
 */
static int
check_overlap(int x, int y)
{
  int i, j;

  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      if (block[i][j] && stage[y + i][x + j] != 0) {
        return TRUE;
      }
    }
  }
  return FALSE;
}


/*!
 * @brief Move block
 * @param [in] new_x  New X-coordinate of block
 * @param [in] new_y  New Y-coordinate of block
 */
static void
move_block(int new_x, int new_y)
{
  int i, j;

  /* Delete block on old position */
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      field[block_pos.y + i][block_pos.x + j] -= block[i][j];
    }
  }

  /* Update block position */
  block_pos.x = new_x;
  block_pos.y = new_y;

  /* Push block into new position */
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      field[block_pos.y + i][block_pos.x + j] += block[i][j];
    }
  }
  update_flag.block = TRUE;
}


/*!
 * @brief Rotate block
 * @param [in] direction  Value which indicates right or left
 * @return Return 0 if rotate is enable, otherwise return 1.
 */
static int
turn_block(Direction direction)
{
  static unsigned char temp[BLOCK_HEIGHT][BLOCK_WIDTH];
  int i, j;

  memcpy((void *) temp, (const void *) block, sizeof(temp));
  if (direction == RIGHT) {  /* Right-handed rotation */
    for (i = 0; i < BLOCK_HEIGHT; i++) {
      for (j = 0; j < BLOCK_WIDTH; j++) {
        block[i][j] = temp[(BLOCK_WIDTH - 1) - j][i];
      }
    }
  } else {  /* Left-handed rotation */
    for (i = 0; i < BLOCK_HEIGHT; i++) {
      for (j = 0; j < BLOCK_WIDTH; j++) {
        block[i][j] = temp[j][(BLOCK_WIDTH - 1) - i];
      }
    }
  }
  /* If block is overlapped, return to the original. */
  if (check_overlap(block_pos.x, block_pos.y)) {
    memcpy((void *) block, (const void *) temp, sizeof(block));
    return FALSE;
  }
  /* Delete old block and add new block */
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      field[block_pos.y + i][block_pos.x + j] -= temp[i][j];
      field[block_pos.y + i][block_pos.x + j] += block[i][j];
    }
  }
  update_flag.block = TRUE;
  return TRUE;
}


/*!
 * @brief Fix block after landing
 *
 * Also check whether a single horizontal row are aligned.
 */
static void
lock_block(void)
{
  /* Lock block */
  memcpy((void *) stage, (const void *) field, sizeof(stage));
  check_lines();
  /* Update */
  memcpy((void *) field, (const void *) stage, sizeof(field));
}


/*!
 * @brief Check whether a single horizontal row are aligned
 *
 * If horizontal row are aligned, delete the row and drop upper blocks.
 */
static void
check_lines(void)
{
  int comp;
  int lines = 0;

  for (;;) {
    int i, j, k;
    for (i = 0; i < STAGE_HEIGHT - 1; i++) {
      comp = TRUE;
      for (j = 1; j < STAGE_WIDTH - 1; j++) {
        if (stage[i][j] == 0) {
          comp = FALSE;
        }
      }
      if (comp) break;
    }
    if (!comp) break;

    lines++;
    /* Delete rows */
    for (j = 1; j < STAGE_WIDTH - 1; j++) {
      stage[i][j] = 0;
    }
    /* Drop upper blocks */
    for (j = i; j > 0; j--) {
      for (k = 1; k < STAGE_WIDTH - 1; k++) {
        stage[j][k] = stage[j - 1][k];
      }
    }
  }
  /* Add score depending on the number of simultaneously erase line */
  switch (lines) {
    case 1:
      score += SCORE1;
      update_flag.score = TRUE;
      break;
    case 2:
      score += SCORE2;
      update_flag.score = TRUE;
      break;
    case 3:
      score += SCORE3;
      update_flag.score = TRUE;
      break;
    case 4:
      score += SCORE4;
      update_flag.score = TRUE;
      break;
  }
}


/*!
 * @brief Update screen
 */
static void
update_screen(void)
{
  if (update_flag.block) {
    print_field(field, FIELD_X);
    update_flag.block = FALSE;
  }
  if (update_flag.next_block) {
    print_next_blocks();
    update_flag.next_block = FALSE;
  }
  if (update_flag.time) {
    print_time(gametime);
    update_flag.time = FALSE;
  }
  if (update_flag.score) {
    print_score(score);
    update_flag.score = FALSE;
  }
  tu_refresh();
}

/*!
 * @brief Draw labels
 */
static void
print_labels(void)
{
  tu_mvaddstr(TIME_Y - 1, TIME_X - 1, "time:");
  tu_mvaddstr(SCORE_Y - 1, SCORE_X - 1, "score:");
  print_score(0);

  tu_mvaddstr(SCORE_Y + 2, SCORE_X - 1, "h : move left");
  tu_mvaddstr(SCORE_Y + 3, SCORE_X - 1, "l : move right");
  tu_mvaddstr(SCORE_Y + 4, SCORE_X - 1, "j : drop a block");
  tu_mvaddstr(SCORE_Y + 5, SCORE_X - 1, "a : right-handed rotation");
  tu_mvaddstr(SCORE_Y + 6, SCORE_X - 1, "s : left-handed  rotation");
}


/*!
 * @brief Draw wall
 */
static void
print_wall(void)
{
  int i;

  tu_mvaddstr(FIELD_Y - 1, 0, "xxxxx            xxxxxxxxxxxxxxxx");
  for (i = 0; i < STAGE_HEIGHT - 1; i++) {
    tu_mvaddstr(i + FIELD_Y, 0, "x                    x          x");
  }
  for (i = 1; i < (int) LENGTHOF(next_blocks); i++) {
    tu_mvaddstr(NEXT_BLOCK_YS[i] - 2, NEXT_BLOCK_X - 1, "xxxxxxxxxx");
  }
  tu_mvaddstr(FIELD_Y + STAGE_HEIGHT - 1, 0, "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
}


/*!
 * @brief Draw field
 *
 * @param [in] field  Field data
 * @param [in] x      X-coordinate of the field
 */
static void
print_field(unsigned char field[STAGE_HEIGHT][STAGE_WIDTH], int x)
{
  int i, j;

  for (i = 0; i < STAGE_HEIGHT - 1; i++) {
    tu_move(i + FIELD_Y, x + 1);
    for (j = 1; j < STAGE_WIDTH - 1; j++) {
      change_background_color(field[i][j]);
      tu_addstr("  ");
    }
  }
  tu_set_foreground(TU_DEFAULT_COLOR);
  tu_set_background(TU_DEFAULT_COLOR);
}


/*!
 * @brief Print next blocks
 */
static void
print_next_blocks(void)
{
  int i, j, k, idx;

  for (i = 0; i < (int) LENGTHOF(next_blocks); i++) {
    idx = next_blocks[(next_idx + i) % LENGTHOF(next_blocks)];
    for (j = 0; j < BLOCK_HEIGHT; j++) {
      for (k = 0; k < BLOCK_WIDTH; k++) {
        if (block_list[idx][j][k]) {
          change_background_color(block_list[idx][j][k]);
        } else {
          tu_set_background(TU_DEFAULT_COLOR);
        }
        tu_mvaddstr(NEXT_BLOCK_YS[i] + j, NEXT_BLOCK_X + k * 2, "  ");
      }
    }
  }
  tu_set_background(TU_DEFAULT_COLOR);
}


/*!
 * @brief Draw player's score
 * @param [in] x      X-coordinate of score
 * @param [in] y      Y-coordinate of score
 * @param [in] score  Player's score
 */
static void
print_score(int _score)
{
  tu_mvprintw(SCORE_Y, SCORE_X, "%5d", _score);
}


/*!
 * @brief Draw elapsed time
 * @param [in] time  elapsed time
 */
static void
print_time(time_t time)
{
  tu_mvprintw(TIME_Y, TIME_X, "%5lu", time);
}


/*!
 * @brief Return mill second of current time (UTC)
 * @return Mill second of current time
 */
static time_t
get_utc(void)
{
  time_t sec;
  long int msec;
#ifdef _MSC_VER
  struct _timeb timeb;
  _ftime_s(&timeb);
  sec = timeb.time;
  msec = timeb.millitm;
#elif defined(_POSIX_TIMERS)
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  sec = tp.tv_sec;
  msec = tp.tv_nsec / 10000000;
#else
  struct timeval tv;
  gettimeofday(&tv, 0);
  sec = tv.tv_sec;
  msec = tv.tv_usec / 1000;
#endif
  return sec * 1000 + msec;
}


/*!
 * @brief Signal handler for SIGINT
 * @param [in] sig  Signal number (unused variable)
 */
static void
sigint_handler(int __UNUSED__(sig))
{
  tu_cleanup();
  exit(EXIT_SUCCESS);
}
