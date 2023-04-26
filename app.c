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

static const s32 x_deltas[4] = {0, 1, 0, -1};
static const s32 y_deltas[4] = {-1, 0, 1, 0};

App
App_Create(u32 pipe_count, u32 rows, u32 cols, Rng *rng)
{
	App app = {.pipe_count = pipe_count,
	           .rows = rows,
	           .cols = cols,
	           .rng = rng,
	           .xs = calloc(pipe_count, sizeof(u32)),
	           .ys = calloc(pipe_count, sizeof(u32)),
	           .directions = calloc(pipe_count, sizeof(Direction)),
	           .old_directions = calloc(pipe_count, sizeof(Direction)),
	           .display = calloc(pipe_count, sizeof(char[3]))};

	for (usize i = 0; i < app.pipe_count; i++) {
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

	memcpy(app.old_directions, app.directions,
	       app.pipe_count * sizeof(Direction));

	return app;
}

void
App_Update(App *app)
{
	u32 pipe_count = app->pipe_count;
	u32 rows = app->rows;
	u32 cols = app->cols;
	u32 *xs = app->xs;
	u32 *ys = app->ys;

	for (usize i = 0; i < pipe_count; i++) {
		s32 dx = x_deltas[app->directions[i]];
		s32 dy = y_deltas[app->directions[i]];
		u32 x = xs[i] + dx;
		u32 y = ys[i] + dy;
		xs[i] = x;
		ys[i] = y;

		app->old_directions[i] = app->directions[i];

		u32 random = Rng_Next(app->rng);

		// either 0 or 1
		s32 should_apply = (random & 1) == 0;

		// either -1 or 1
		s32 rotation = (random & 2) - 1;

		app->directions[i] =
		        (app->directions[i] + rotation * should_apply) & 3;

		if (x >= 0 & x < cols & y >= 0 & y < rows)
			continue;

		u32 coord = random >> 8;
		switch ((random >> 2) & 3) {
		case Edge_Top:
			xs[i] = coord % cols;
			ys[i] = 0;
			app->directions[i] = Direction_Down;
			app->old_directions[i] = Direction_Down;
			break;
		case Edge_Bottom:
			xs[i] = coord % cols;
			ys[i] = rows - 1;
			app->directions[i] = Direction_Up;
			app->old_directions[i] = Direction_Up;
			break;
		case Edge_Left:
			xs[i] = 0;
			ys[i] = coord % rows;
			app->directions[i] = Direction_Right;
			app->old_directions[i] = Direction_Right;
			break;
		case Edge_Right:
			xs[i] = cols - 1;
			ys[i] = coord % rows;
			app->directions[i] = Direction_Left;
			app->old_directions[i] = Direction_Left;
			break;
		}
	}

	for (usize i = 0; i < pipe_count; i++) {
		usize index = app->old_directions[i] << 2 | app->directions[i];
		memcpy(app->display[i], pipes[index], sizeof pipes[0]);
	}
}
