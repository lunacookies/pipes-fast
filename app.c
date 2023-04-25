#include "all.h"

enum { FPS = 60 };

struct termios orig_termios;

static void
die(void)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, "error!\r\n", 7);
	exit(1);
}

static void
disable_raw_mode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
		die();
}

static void
enable_raw_mode(void)
{
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1)
		die();
	atexit(disable_raw_mode);

	struct termios raw = orig_termios;
	raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
	raw.c_oflag &= ~(OPOST);
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

	s32 flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die();
}

static void
hide_cursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25l", 6);
}

static void
show_cursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

static void
get_cursor_position(u32 *rows, u32 *cols)
{
	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
		die();

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
		die();
	if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
		die();
}

static void
get_window_size(u32 *rows, u32 *cols)
{
	struct winsize ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999;999H", 10) != 10)
			die();
		get_cursor_position(rows, cols);
		return;
	}

	*cols = ws.ws_col;
	*rows = ws.ws_row;
}

void
run(void)
{
	enable_raw_mode();
	hide_cursor();
	printf("\x1b[2J");

	u32 rows = 0;
	u32 cols = 0;
	get_window_size(&rows, &cols);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	u64 i = 0;
	for (;;) {
		u64 frame_start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		char c;
		read(STDIN_FILENO, &c, 1);
		if (c == 'q')
			break;
		printf("\x1b[H");

		for (u32 y = 0; y < rows; y++) {
			if (y != 0)
				printf("\r\n");
			if (y == 0)
				printf("frame %llu", i);
			if (y == 1)
				printf("read '%c' %d", c, c);
		}

		u64 frame_end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
		u64 frame_duration_ns = frame_end_ns - frame_start_ns;
		if (frame_duration_ns < target_frame_duration_ns) {
			u64 ns_to_sleep = target_frame_duration_ns -
			                  frame_duration_ns;
			struct timespec ts = {0, ns_to_sleep};
			if (nanosleep(&ts, NULL) != 0)
				die();
		}

		i++;
	}

	printf("\x1b[2J");
	printf("\x1b[H");
	disable_raw_mode();
	show_cursor();
}
