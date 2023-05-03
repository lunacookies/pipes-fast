#include "all.h"

enum { FPS = 60 };

static void
bench(void)
{
	u64 total_ns = 0;
	u64 frame_count = 0;

	for (usize i = 0; i < 10; i++) {
		Rng rng = Rng_CreateWithSystemEntropy();
		App app = App_Create(1024 * 1024, 24, 80, &rng);

		u64 start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);
		for (usize j = 0; j < 100; j++)
			App_Update(&app);
		u64 end_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		total_ns += end_ns - start_ns;
		frame_count += 100;
	}

	printf("%llu micros / frame\n", total_ns / frame_count / 1000);
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

	u16 rows = 0;
	u16 cols = 0;
	GetWindowSize(&rows, &cols);

	Rng rng = Rng_CreateWithSystemEntropy();
	App app = App_Create(10, rows, cols, &rng);
	OutputBuffer buf = OutputBuffer_Create(rows * cols);

	u64 second_ns = 1000000000;
	u64 target_frame_duration_ns = second_ns / FPS;

	for (;;) {
		u64 frame_start_ns = clock_gettime_nsec_np(CLOCK_MONOTONIC_RAW);

		char c;
		if (read(STDIN_FILENO, &c, 1) == 1)
			if (c == 'q')
				break;

		App_Update(&app);

		OutputBuffer_Clear(&buf);
		OutputBuffer_Push(&buf, "\x1b[H");

		for (usize i = 0; i < app.pipe_count; i++) {
			OutputBuffer_Push(&buf, "\x1b[%u;%uH", app.ys[i] + 1,
			                  app.xs[i] + 1);

			OutputBuffer_PushBytes(&buf, app.display[i],
			                       sizeof app.display[0]);
		}

		write(STDOUT_FILENO, buf.p, buf.length);

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
