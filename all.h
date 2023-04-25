#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
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

// app.c

void Run(void);
