#include "all.h"

struct termios orig_termios;

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

void
Die(void)
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	write(STDOUT_FILENO, "error!\r\n", 7);
	exit(1);
}

void
DisableRawMode(void)
{
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios) == -1)
		Die();
}

void
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

void
HideCursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void
ShowCursor(void)
{
	write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void
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
