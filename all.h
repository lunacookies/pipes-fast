#include <arm_neon.h>
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
void GetWindowSize(u16 *rows, u16 *cols);

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

enum {
	Direction_Up,
	Direction_Right,
	Direction_Down,
	Direction_Left,
};
typedef u8 Direction;

typedef struct {
	u32 pipe_count;
	u8 rows;
	u8 cols;
	Rng *rng;
	u8 *xs;
	u8 *ys;
	Direction *directions;
	Direction *old_directions;
	char (*display)[3];
} App;

App App_Create(u32 pipe_count, u8 rows, u8 cols, Rng *rng);
void App_Update(App *app);
