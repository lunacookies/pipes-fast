#include "all.h"

enum { FPS = 60 };

int
main(void)
{
	EnableRawMode();
	HideCursor();
	write(STDOUT_FILENO, "\x1b[2J", 4);

	u32 rows = 0;
	u32 cols = 0;
	GetWindowSize(&rows, &cols);

	Rng rng = Rng_CreateWithSystemEntropy();

	App app = App_Create(rows, cols, &rng);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	for (;;) {
		u64 frame_start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
			if (c == 'q')
				break;

		App_Update(&app);
		write(STDOUT_FILENO, app.buf.p, app.buf.length);

		u64 frame_end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
		u64 frame_duration_ns = frame_end_ns - frame_start_ns;
		if (frame_duration_ns < target_frame_duration_ns) {
			u64 ns_to_sleep =
			        target_frame_duration_ns - frame_duration_ns;
			struct timespec ts = {0, ns_to_sleep};
			if (nanosleep(&ts, NULL) != 0)
				Die();
		}
	}

	printf("\x1b[2J");
	printf("\x1b[H");
	DisableRawMode();
	ShowCursor();
}
