#include <assert.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/random.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef size_t usize;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

// terminal.c

void Die(void);
void EnableRawMode(void);
void DisableRawMode(void);
void ShowCursor(void);
void HideCursor(void);
void GetWindowSize(u32 *rows, u32 *cols);

// rng.c

typedef struct {
	u64 s;
} Rng;

Rng Rng_CreateWithSystemEntropy(void);
u64 Rng_Next(Rng *rng);

// output_buffer.c

typedef struct {
	char *p;
	usize length;
	usize capacity;
} OutputBuffer;

OutputBuffer OutputBuffer_Create(usize capacity);
void OutputBuffer_Clear(OutputBuffer *b);
void OutputBuffer_Push(OutputBuffer *b, const char *fmt, ...);
void OutputBuffer_PushBytes(OutputBuffer *b, const char *bytes, usize count);

// app.c

typedef enum {
	Edge_Top,
	Edge_Bottom,
	Edge_Left,
	Edge_Right,
} Edge;

typedef enum {
	Direction_Up,
	Direction_Right,
	Direction_Down,
	Direction_Left,
} Direction;

typedef struct {
	u32 rows;
	u32 cols;
	Rng *rng;
	OutputBuffer buf;
	u32 xs[5];
	u32 ys[5];
	Direction directions[5];
	Direction old_directions[5];
} App;

App App_Create(u32 rows, u32 cols, Rng *rng);
void App_Update(App *app);
