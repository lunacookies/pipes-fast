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
		case Direction_Up:
			app.xs[i] = Rng_Next(app.rng) % app.cols;
			app.ys[i] = app.rows - 1;
			app.directions[i] = Direction_Up;
			break;
		case Direction_Right:
			app.xs[i] = 0;
			app.ys[i] = Rng_Next(app.rng) % app.rows;
			app.directions[i] = Direction_Right;
			break;
		case Direction_Down:
			app.xs[i] = Rng_Next(app.rng) % app.cols;
			app.ys[i] = 0;
			app.directions[i] = Direction_Down;
			break;
		case Direction_Left:
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
	Direction *directions = app->directions;
	Direction *old_directions = app->old_directions;
	char(*display)[3] = app->display;

	Rng rng = *app->rng;

	usize i = 0;

	for (; i < (pipe_count % 4); i++) {
		Direction direction = directions[i];

		s32 dx = x_deltas[direction];
		s32 dy = y_deltas[direction];
		u32 x = xs[i] + dx;
		u32 y = ys[i] + dy;
		xs[i] = x;
		ys[i] = y;

		u32 random = Rng_Next(&rng);

		// either 0 or 1
		s32 should_apply = random & 1;

		// either -1 or 1
		s32 rotation = (random & 2) - 1;

		Direction old_direction = direction;
		old_directions[i] = old_direction;
		direction = (direction + rotation * should_apply) & 3;
		directions[i] = direction;

		if (x < 0 | x >= cols | y < 0 | y >= rows) {
			u32 coord = random >> 8;
			u32 new_x[4] = {coord % cols, 0, coord % cols,
			                cols - 1};
			u32 new_y[4] = {rows - 1, coord % rows, 0,
			                coord % rows};

			direction = (random >> 2) & 3;
			old_direction = direction;
			xs[i] = new_x[direction];
			ys[i] = new_y[direction];
			directions[i] = direction;
			old_directions[i] = direction;
		}

		usize index = old_direction << 2 | direction;
		memcpy(display[i], pipes[index], sizeof pipes[0]);
	}

	for (; i < pipe_count; i += 4) {
		Direction direction[4];
		memcpy(direction, directions + i, sizeof(Direction) * 4);

		u32 x[4];
		memcpy(x, xs + i, sizeof(u32) * 4);

		u32 y[4];
		memcpy(y, ys + i, sizeof(u32) * 4);

		x[0] += x_deltas[direction[0]];
		x[1] += x_deltas[direction[1]];
		x[2] += x_deltas[direction[2]];
		x[3] += x_deltas[direction[3]];

		y[0] += y_deltas[direction[0]];
		y[1] += y_deltas[direction[1]];
		y[2] += y_deltas[direction[2]];
		y[3] += y_deltas[direction[3]];

		memcpy(xs + i, x, sizeof(u32) * 4);
		memcpy(ys + i, y, sizeof(u32) * 4);

		u64 random_big0 = Rng_Next(&rng);
		u64 random_big1 = Rng_Next(&rng);
		u32 random[4];
		memcpy(random, &random_big0, sizeof(u64));
		memcpy(random + 2, &random_big1, sizeof(u64));

		// either 0 or 1
		s32 should_apply[4];
		should_apply[0] = random[0] & 1;
		should_apply[1] = random[1] & 1;
		should_apply[2] = random[2] & 1;
		should_apply[3] = random[3] & 1;

		// either -1 or 1
		s32 rotation[4];
		rotation[0] = (random[0] & 2) - 1;
		rotation[1] = (random[1] & 2) - 1;
		rotation[2] = (random[2] & 2) - 1;
		rotation[3] = (random[3] & 2) - 1;

		Direction old_direction[4];
		memcpy(old_direction, direction, sizeof(Direction) * 4);
		memcpy(old_directions + i, old_direction,
		       sizeof(Direction) * 4);

		direction[0] =
		        (direction[0] + rotation[0] * should_apply[0]) & 3;
		direction[1] =
		        (direction[1] + rotation[1] * should_apply[1]) & 3;
		direction[2] =
		        (direction[2] + rotation[2] * should_apply[2]) & 3;
		direction[3] =
		        (direction[3] + rotation[3] * should_apply[3]) & 3;

		memcpy(directions + i, direction, sizeof(Direction) * 4);

		for (usize j = 0; j < 4; j++) {
			if (x[j] < 0 | x[j] >= cols | y[j] < 0 | y[j] >= rows) {
				u32 coord = random[j] >> 8;
				u32 new_x[4] = {coord % cols, 0, coord % cols,
				                cols - 1};
				u32 new_y[4] = {rows - 1, coord % rows, 0,
				                coord % rows};

				direction[j] = (random[j] >> 2) & 3;
				old_direction[j] = direction[j];
				xs[i + j] = new_x[direction[j]];
				ys[i + j] = new_y[direction[j]];
				directions[i + j] = direction[j];
				old_directions[i + j] = direction[j];
			}
		}

		memcpy(display[i + 0],
		       pipes[old_direction[0] << 2 | direction[0]],
		       sizeof pipes[0]);
		memcpy(display[i + 1],
		       pipes[old_direction[1] << 2 | direction[1]],
		       sizeof pipes[0]);
		memcpy(display[i + 2],
		       pipes[old_direction[2] << 2 | direction[2]],
		       sizeof pipes[0]);
		memcpy(display[i + 3],
		       pipes[old_direction[3] << 2 | direction[3]],
		       sizeof pipes[0]);
	}

	*app->rng = rng;
}
