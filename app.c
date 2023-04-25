#include "all.h"

enum { FPS = 60 };

struct termios orig_termios;

static void
Die(void)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, "error!\r\n", 7);
	exit(1);
}

static void
DisableRawMode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
		Die();
}

static void
EnableRawMode(void)
{
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
		Die();
	atexit(DisableRawMode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	s32 flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		Die();
}

static void
HideCursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25l", 6);
}

static void
ShowCursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

static void
GetCursorPosition(u32 *rows, u32 *cols)
{
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		Die();

	char buf[32];
	u32 i = 0;

	while (i < sizeof(buf) - 1) {
		if (read(STDIN_FILENO, &buf[i], 1) != 1)
			break;
		if (buf[i] == 'R')
			break;
		i++;
	}
	buf[i] = '\0';

	if (buf[0] != '\x1b' || buf[1] != '[')
		Die();
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
		Die();
}

static void
GetWindowSize(u32 *rows, u32 *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999;999H", 10) != 10)
			Die();
		GetCursorPosition(rows, cols);
		return;
	}

	*cols = ws.ws_col;
	*rows = ws.ws_row;
}

typedef struct {
	char *p;
	usize length;
	usize capacity;
} OutputBuffer;

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
	printf("\x1b[2J");

	u32 rows = 0;
	u32 cols = 0;
	GetWindowSize(&rows, &cols);

	OutputBuffer buf = OutputBuffer_Create(rows * cols);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	u32 x = 0;
	u32 y = 5;

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
		OutputBuffer_Push(&buf, "-", 1);
		x++;

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
