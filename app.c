#include "all.h"

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
	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die();
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
	u32 rows = 0;
	u32 cols = 0;
	get_window_size(&rows, &cols);
	printf("rows: %d\r\ncols: %d\r\n", rows, cols);
	printf("\x1b[10;10Hhi");
	printf("\x1b[20;10Hhi");
	disable_raw_mode();
}
