#include "all.h"

enum { FPS = 60 };

static void
bench(void)
{
	u64 total_ns = 0;
	u64 frame_count = 0;

	for (usize i = 0; i < 1000; i++) {
		Rng rng = Rng_CreateWithSystemEntropy();
		App app = App_Create(24, 80, &rng);
		for (usize j = 0; j < 10000; j++) {
			u64 start_ns =
			        clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
			App_Update(&app);
			u64 end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
			total_ns += end_ns - start_ns;
			frame_count++;
		}
	}

	printf("%llu nanos / frame\n", total_ns / frame_count);
}

int
main(s32 argument_count, const char **arguments)
{
	if (argument_count == 2 && strcmp(arguments[1], "--bench") == 0) {
		bench();
		return 0;
	}

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
