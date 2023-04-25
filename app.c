#include "all.h"

static const char pipes[16][3] = {
        [Direction_Up << 2 | Direction_Up] = "┃",
        [Direction_Up << 2 | Direction_Right] = "┏",
        [Direction_Up << 2 | Direction_Down] = {0},
        [Direction_Up << 2 | Direction_Left] = "┓",

        [Direction_Right << 2 | Direction_Up] = "┛",
        [Direction_Right << 2 | Direction_Right] = "━",
        [Direction_Right << 2 | Direction_Down] = "┓",
        [Direction_Right << 2 | Direction_Left] = {0},

        [Direction_Down << 2 | Direction_Up] = {0},
        [Direction_Down << 2 | Direction_Right] = "┗",
        [Direction_Down << 2 | Direction_Down] = "┃",
        [Direction_Down << 2 | Direction_Left] = "┛",

        [Direction_Left << 2 | Direction_Up] = "┗",
        [Direction_Left << 2 | Direction_Right] = {0},
        [Direction_Left << 2 | Direction_Down] = "┏",
        [Direction_Left << 2 | Direction_Left] = "━",
};

App
App_Create(u32 rows, u32 cols, Rng *rng)
{
	App app = {.rows = rows,
	           .cols = cols,
	           .rng = rng,
	           .buf = OutputBuffer_Create(rows * cols),
	           .xs = {0},
	           .ys = {0},
	           .directions = {0},
	           .old_directions = {0},
	           .frame_no = 0};

	for (usize i = 0; i < 5; i++) {
		switch (Rng_Next(app.rng) & 3) {
		case Edge_Top:
			app.xs[i] = Rng_Next(app.rng) % app.cols;
			app.ys[i] = 0;
			app.directions[i] = Direction_Down;
			break;
		case Edge_Bottom:
			app.xs[i] = Rng_Next(app.rng) % app.cols;
			app.ys[i] = app.rows - 1;
			app.directions[i] = Direction_Up;
			break;
		case Edge_Left:
			app.xs[i] = 0;
			app.ys[i] = Rng_Next(app.rng) % app.rows;
			app.directions[i] = Direction_Right;
			break;
		case Edge_Right:
			app.xs[i] = app.cols - 1;
			app.ys[i] = Rng_Next(app.rng) % app.rows;
			app.directions[i] = Direction_Left;
			break;
		}
	}

	memcpy(app.old_directions, app.directions, sizeof app.directions);

	return app;
}

void
App_Update(App *app)
{
	app->frame_no++;

	OutputBuffer_Clear(&app->buf);

	OutputBuffer_Push(&app->buf, "\x1b[H");
	OutputBuffer_Push(&app->buf, "frame %llu\r\n", app->frame_no);

	for (usize i = 0; i < 5; i++) {
		OutputBuffer_Push(&app->buf, "\x1b[%u;%uH", app->ys[i] + 1,
		                  app->xs[i] + 1);

		usize index = app->old_directions[i] << 2 | app->directions[i];
		OutputBuffer_PushBytes(&app->buf, pipes[index], 3);
	}

	for (usize i = 0; i < 5; i++) {
		switch (app->directions[i]) {
		case Direction_Up:
			app->ys[i]--;
			break;
		case Direction_Down:
			app->ys[i]++;
			break;
		case Direction_Left:
			app->xs[i]--;
			break;
		case Direction_Right:
			app->xs[i]++;
			break;
		}

		app->old_directions[i] = app->directions[i];

		// either 0 or 1
		s32 should_apply = (Rng_Next(app->rng) & 1) == 0;

		// either -1 or 1
		s32 rotation = ((Rng_Next(app->rng) & 1) << 1) - 1;

		app->directions[i] =
		        (app->directions[i] + rotation * should_apply) & 3;

		if (app->xs[i] >= 0 && app->xs[i] < app->cols &&
		    app->ys[i] >= 0 && app->ys[i] < app->rows)
			continue;

		switch (Rng_Next(app->rng) & 3) {
		case Edge_Top:
			app->xs[i] = Rng_Next(app->rng) % app->cols;
			app->ys[i] = 0;
			app->directions[i] = Direction_Down;
			app->old_directions[i] = Direction_Down;
			break;
		case Edge_Bottom:
			app->xs[i] = Rng_Next(app->rng) % app->cols;
			app->ys[i] = app->rows - 1;
			app->directions[i] = Direction_Up;
			app->old_directions[i] = Direction_Up;
			break;
		case Edge_Left:
			app->xs[i] = 0;
			app->ys[i] = Rng_Next(app->rng) % app->rows;
			app->directions[i] = Direction_Right;
			app->old_directions[i] = Direction_Right;
			break;
		case Edge_Right:
			app->xs[i] = app->cols - 1;
			app->ys[i] = Rng_Next(app->rng) % app->rows;
			app->directions[i] = Direction_Left;
			app->old_directions[i] = Direction_Left;
			break;
		}
	}
}
