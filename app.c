#include "all.h"

enum { FPS = 60 };

typedef struct {
	char *p;
	usize length;
	usize capacity;
} OutputBuffer;

typedef enum {
	Edge_Top,
	Edge_Bottom,
	Edge_Left,
	Edge_Right,
} Edge;

typedef enum {
	Direction_Up,
	Direction_Down,
	Direction_Left,
	Direction_Right,
} Direction;

static OutputBuffer
OutputBuffer_Create(usize capacity)
{
	return (OutputBuffer){.p = calloc(capacity, 1),
	                      .length = 0,
	                      .capacity = capacity};
}

static void
OutputBuffer_Clear(OutputBuffer *b)
{
	b->length = 0;
}

static void
OutputBuffer_Push(OutputBuffer *b, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	usize remaining = b->capacity - b->length;
	usize bytes_written = vsnprintf(b->p + b->length, remaining, fmt, ap);
	b->length += bytes_written;
	va_end(ap);
}

void
Run(void)
{
	EnableRawMode();
	HideCursor();
	write(STDOUT_FILENO, "\x1b[2J", 4);

	u32 rows = 0;
	u32 cols = 0;
	GetWindowSize(&rows, &cols);

	Rng rng = Rng_CreateWithSystemEntropy();
	OutputBuffer buf = OutputBuffer_Create(rows * cols);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	Edge edge = Rng_Next(&rng) & 3;

	u32 x = 0;
	u32 y = 0;

	Direction direction = 0;

	switch (edge) {
	case Edge_Top:
		x = Rng_Next(&rng) % cols;
		y = 0;
		direction = Direction_Down;
		break;
	case Edge_Bottom:
		x = Rng_Next(&rng) % cols;
		y = rows - 1;
		direction = Direction_Up;
		break;
	case Edge_Left:
		x = 0;
		y = Rng_Next(&rng) % rows;
		direction = Direction_Right;
		break;
	case Edge_Right:
		x = cols - 1;
		y = Rng_Next(&rng) % rows;
		direction = Direction_Left;
		break;
	}

	u64 i = 0;
	for (;;) {
		OutputBuffer_Clear(&buf);

		u64 frame_start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
			if (c == 'q')
				break;

		OutputBuffer_Push(&buf, "\x1b[H");
		OutputBuffer_Push(&buf, "frame %llu\r\n", i);
		OutputBuffer_Push(&buf, "read '%c' %d", c, c);

		OutputBuffer_Push(&buf, "\x1b[%u;%uH", y + 1, x + 1);

		switch (direction) {
		case Direction_Up:
			OutputBuffer_Push(&buf, "|");
			y--;
			break;
		case Direction_Down:
			OutputBuffer_Push(&buf, "|");
			y++;
			break;
		case Direction_Left:
			OutputBuffer_Push(&buf, "-");
			x--;
			break;
		case Direction_Right:
			OutputBuffer_Push(&buf, "-");
			x++;
			break;
		}

		write(STDOUT_FILENO, buf.p, buf.length);

		u64 frame_end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
		u64 frame_duration_ns = frame_end_ns - frame_start_ns;
		if (frame_duration_ns < target_frame_duration_ns) {
			u64 ns_to_sleep = target_frame_duration_ns -
			                  frame_duration_ns;
			struct timespec ts = {0, ns_to_sleep};
			if (nanosleep(&ts, NULL) != 0)
				Die();
		}

		i++;
	}

	printf("\x1b[2J");
	printf("\x1b[H");
	DisableRawMode();
	ShowCursor();
}
