/*!
 * @file tetris.c
 * @brief Console tetris
 * @author koturn
 */
#include <memory.h>
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


#ifdef _MSC_VER
#  define msec_sleep(x)  Sleep(x)
#else
#  define msec_sleep(x)  usleep(x * 1000)
#endif


#define STAGE_WIDTH   12  /*!< Width of stage */
#define STAGE_HEIGHT  21  /*!< Height of stage */

#define BLOCK_WIDTH   4  /*!< Width of blocks */
#define BLOCK_HEIGHT  4  /*!< Height of blocks */
#define N_BLOCK       7  /*!< The number of block types */

#define SPACE  0  /*!< Indicate there is nothing */
#define WALL   9  /*!< Indicate there is wall */

#define FIELD_X   0  /*!< X-coordinate of player's field */
#define FIELD_Y   2  /*!< Y-coordinate of player's field */
#define SCORE_X  32  /*!< X-coordinate of player's score */
#define SCORE_Y  10  /*!< Y-coordinate of player's score */

#define SCORE_LEN  10  /*!< Buffer size of score label */
#define SCORE1  100
#define SCORE2  300
#define SCORE3  500
#define SCORE4  1000

#define CURSOR_X  (FIELD_X + STAGE_WIDTH)   /*!< X-coordinate of temporary cursor position */
#define CURSOR_Y  (FIELD_Y + STAGE_HEIGHT)  /*!< Y-coordinate of temporary cursor position */

#define TIME_X    32  /*!< X-coordinate of time label */
#define TIME_Y     5  /*!< Y-coordinate of time label */
#define TIME_LEN   5  /*!< Buffer size of time label */

#ifndef TRUE
#  define TRUE  1
#endif
#ifndef FALSE
#  define FALSE  0
#endif


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

static int y = 0;            /*!< Y-coordinate of dropping block */
static int x = 4;            /*!< X-coordinate of dropping block */
static int score = 0;        /*!< Player's score */
static int gameover = FALSE; /*!< Flag of gameover */

static void
initialize(void);

static void
print_labels(void);

static void
control_block(void);

static void
drop_block(void);

static void
print_wall(void);

static void
print_field(unsigned char field[STAGE_HEIGHT][STAGE_WIDTH], int x);

static void
print_score(int x, int y, int score);

static void
print_time(time_t time);

static void
create_block(void);

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

static time_t
get_utc(void);




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
  while (!gameover) {
    control_block();
    if ((cnt = (cnt + 1) % 32) == 0) {
      drop_block();
    }
    print_time(time(NULL) - base_time);
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
  create_block();
  print_wall();
  print_field(field, FIELD_X);
}


/*!
 * @brief Draw labels
 */
static void
print_labels(void)
{
  tu_mvaddstr(TIME_Y - 1, TIME_X - 1, "time:");
  tu_mvaddstr(SCORE_Y - 1, SCORE_X - 1, "score:");
  print_score(SCORE_X, SCORE_Y, 0);

  tu_mvaddstr(SCORE_Y + 2, SCORE_X - 1, "h : move left");
  tu_mvaddstr(SCORE_Y + 3, SCORE_X - 1, "l : move right");
  tu_mvaddstr(SCORE_Y + 4, SCORE_X - 1, "j : drop a block");
  tu_mvaddstr(SCORE_Y + 5, SCORE_X - 1, "a : right-handed rotation");
  tu_mvaddstr(SCORE_Y + 6, SCORE_X - 1, "s : left-handed  rotation");
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
      if (!check_overlap(x + 1, y)) {
        move_block(x + 1, y);
      }
      break;
    case 'h':
      if (!check_overlap(x - 1, y)) {
        move_block(x - 1, y);
      }
      break;
    case 'j':
      if (!check_overlap(x, y + 1)) {
        move_block(x, y + 1);
      }
      break;
    /* ----- Like Emacs ----- */
    case CTRL_F:
      if (!check_overlap(x + 1, y)) {
        move_block(x + 1, y);
      }
      break;
    case CTRL_B:
      if (!check_overlap(x - 1, y)) {
        move_block(x - 1, y);
      }
      break;
    case CTRL_N:
      if (!check_overlap(x, y + 1)) {
        move_block(x, y + 1);
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
  if (!check_overlap(x, y + 1)) {
    move_block(x, y + 1);
  } else {
    lock_block();
    create_block();
    print_field(field, FIELD_X);
  }
}


/*!
 * @brief Draw player's score
 * @param [in] x      X-coordinate of score
 * @param [in] y      Y-coordinate of score
 * @param [in] score  Player's score
 */
static void
print_score(int x, int y, int score)
{
  static char score_str[SCORE_LEN];

  sprintf(score_str, "%5u", score);
  tu_mvaddstr(y, x, score_str);
}


/*!
 * @brief Draw elapsed time
 * @param [in] time  elapsed time
 */
static void
print_time(time_t time)
{
  static char time_str[TIME_LEN];

  sprintf(time_str, "%3ld", time);
  tu_mvaddstr(TIME_Y, TIME_X, time_str);
}


/*!
 * @brief Create block
 */
static void
create_block(void)
{
  int i, j;
  int block_type = rand() % N_BLOCK;
  y = 0;
  x = 4;

  memcpy((void *) block, (const void *) block_list[block_type], sizeof(block));
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      /* Gameover if locked block is already exist on in initial position */
      if (stage[i][j + 4] != 0) {
        gameover = TRUE;
        return;
      }
      field[i][j + 4] = block[i][j];
    }
  }
}


/*!
 * @brief Draw wall
 */
static void
print_wall(void)
{
  int i;

  tu_move(FIELD_Y - 1, 0);
  tu_addstr("xxxxx            xxxxx");
  for (i = 0; i < STAGE_HEIGHT - 1; i++) {
    tu_move(i + FIELD_Y, 0);
    tu_addstr("x                    x");
  }
  tu_move(FIELD_Y + STAGE_HEIGHT - 1, 0);
  tu_addstr("xxxxxxxxxxxxxxxxxxxxxx");
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
      switch (field[i][j]) {
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
      tu_addstr("  ");
    }
  }
  tu_set_foreground(TU_DEFAULT_COLOR);
  tu_set_background(TU_DEFAULT_COLOR);
  print_score(SCORE_X, SCORE_Y, score);

  tu_refresh();
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
      field[y + i][x + j] -= block[i][j];
    }
  }

  /* Update block position */
  x = new_x;
  y = new_y;

  /* Push block into new position */
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      field[y + i][x + j] += block[i][j];
    }
  }
  print_field(field, FIELD_X);
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
  if (check_overlap(x, y)) {
    memcpy((void *) block, (const void *) temp, sizeof(block));
    return FALSE;
  }
  /* Delete old block and add new block */
  for (i = 0; i < BLOCK_HEIGHT; i++) {
    for (j = 0; j < BLOCK_WIDTH; j++) {
      field[y + i][x + j] -= temp[i][j];
      field[y + i][x + j] += block[i][j];
    }
  }
  print_field(field, FIELD_X);
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
      break;
    case 2:
      score += SCORE2;
      break;
    case 3:
      score += SCORE3;
      break;
    case 4:
      score += SCORE4;
      break;
  }
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
