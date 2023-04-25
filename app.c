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

static void
OutputBuffer_PushBytes(OutputBuffer *b, const char *bytes, usize count)
{
	assert(b->length + count < b->capacity);
	memcpy(b->p + b->length, bytes, count);
	b->length += count;
}

void
Run(void)
{
	EnableRawMode();
	HideCursor();
	write(STDOUT_FILENO, "\x1b[2J", 4);

	char pipes[16][3] = {
	                [Direction_Up << 2 | Direction_Up] = "┃",
	                [Direction_Up << 2 | Direction_Left] = "┓",
	                [Direction_Up << 2 | Direction_Right] = "┏",

	                [Direction_Down << 2 | Direction_Down] = "┃",
	                [Direction_Down << 2 | Direction_Left] = "┛",
	                [Direction_Down << 2 | Direction_Right] = "┗",

	                [Direction_Left << 2 | Direction_Up] = "┗",
	                [Direction_Left << 2 | Direction_Down] = "┏",
	                [Direction_Left << 2 | Direction_Left] = "━",

	                [Direction_Right << 2 | Direction_Up] = "┛",
	                [Direction_Right << 2 | Direction_Down] = "┓",
	                [Direction_Right << 2 | Direction_Right] = "━",
	};

	u32 rows = 0;
	u32 cols = 0;
	GetWindowSize(&rows, &cols);

	Rng rng = Rng_CreateWithSystemEntropy();
	OutputBuffer buf = OutputBuffer_Create(rows * cols);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	Edge edges[5] = {0};
	for (usize i = 0; i < 5; i++)
		edges[i] = Rng_Next(&rng) & 3;

	u32 xs[5] = {0};
	u32 ys[5] = {0};

	Direction directions[5] = {0};
	Direction old_directions[5] = {0};

	for (usize i = 0; i < 5; i++) {
		switch (edges[i]) {
		case Edge_Top:
			xs[i] = Rng_Next(&rng) % cols;
			ys[i] = 0;
			directions[i] = Direction_Down;
			break;
		case Edge_Bottom:
			xs[i] = Rng_Next(&rng) % cols;
			ys[i] = rows - 1;
			directions[i] = Direction_Up;
			break;
		case Edge_Left:
			xs[i] = 0;
			ys[i] = Rng_Next(&rng) % rows;
			directions[i] = Direction_Right;
			break;
		case Edge_Right:
			xs[i] = cols - 1;
			ys[i] = Rng_Next(&rng) % rows;
			directions[i] = Direction_Left;
			break;
		}
	}

	u64 frame_no = 0;
	for (;;) {
		OutputBuffer_Clear(&buf);

		u64 frame_start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
			if (c == 'q')
				break;

		OutputBuffer_Push(&buf, "\x1b[H");
		OutputBuffer_Push(&buf, "frame %llu\r\n", frame_no);
		OutputBuffer_Push(&buf, "read '%c' %d", c, c);

		for (usize i = 0; i < 5; i++) {
			OutputBuffer_Push(&buf, "\x1b[%u;%uH", ys[i] + 1,
			                  xs[i] + 1);

			usize index = old_directions[i] << 2 | directions[i];
			OutputBuffer_PushBytes(&buf, pipes[index], 3);

			switch (directions[i]) {
			case Direction_Up:
				ys[i]--;
				break;
			case Direction_Down:
				ys[i]++;
				break;
			case Direction_Left:
				xs[i]--;
				break;
			case Direction_Right:
				xs[i]++;
				break;
			}
		}

		write(STDOUT_FILENO, buf.p, buf.length);

		for (usize i = 0; i < 5; i++) {
			old_directions[i] = directions[i];
			if ((Rng_Next(&rng) & 1) == 0) {
				if ((Rng_Next(&rng) & 1) == 0) {
					switch (directions[i]) {
					case Direction_Up:
						directions[i] = Direction_Left;
						break;
					case Direction_Down:
						directions[i] = Direction_Right;
						break;
					case Direction_Left:
						directions[i] = Direction_Down;
						break;
					case Direction_Right:
						directions[i] = Direction_Up;
						break;
					}
				} else {
					switch (directions[i]) {
					case Direction_Up:
						directions[i] = Direction_Right;
						break;
					case Direction_Down:
						directions[i] = Direction_Left;
						break;
					case Direction_Left:
						directions[i] = Direction_Up;
						break;
					case Direction_Right:
						directions[i] = Direction_Down;
						break;
					}
				}
			}
		}

		u64 frame_end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
		u64 frame_duration_ns = frame_end_ns - frame_start_ns;
		if (frame_duration_ns < target_frame_duration_ns) {
			u64 ns_to_sleep = target_frame_duration_ns -
			                  frame_duration_ns;
			struct timespec ts = {0, ns_to_sleep};
			if (nanosleep(&ts, NULL) != 0)
				Die();
		}

		frame_no++;
	}

	printf("\x1b[2J");
	printf("\x1b[H");
	DisableRawMode();
	ShowCursor();
}
