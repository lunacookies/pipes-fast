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

static const s8 x_deltas[4] = {0, 1, 0, -1};
static const s8 y_deltas[4] = {-1, 0, 1, 0};

App
App_Create(u32 pipe_count, u8 rows, u8 cols, Rng *rng)
{
	App app = {.pipe_count = pipe_count,
	           .rows = rows,
	           .cols = cols,
	           .rng = rng,
	           .xs = calloc(pipe_count, sizeof(u16)),
	           .ys = calloc(pipe_count, sizeof(u16)),
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
	u8 rows = app->rows;
	u8 cols = app->cols;
	u8 *xs = app->xs;
	u8 *ys = app->ys;
	Direction *directions = app->directions;
	Direction *old_directions = app->old_directions;
	char(*display)[3] = app->display;

	Rng rng = *app->rng;

	usize i = 0;

	for (; i < (pipe_count % 16); i++) {
		Direction direction = directions[i];

		s8 dx = x_deltas[direction];
		s8 dy = y_deltas[direction];
		u8 x = (s8)xs[i] + dx;
		u8 y = (s8)ys[i] + dy;
		xs[i] = x;
		ys[i] = y;

		u32 random = Rng_Next(&rng);

		// either 0 or 1
		s8 should_apply = random & 1;

		// either -1 or 1
		s8 rotation = (random & 2) - 1;

		Direction old_direction = direction;
		old_directions[i] = old_direction;
		direction = (direction + rotation * should_apply) & 3;
		directions[i] = direction;

		if (x < 0 | x >= cols | y < 0 | y >= rows) {
			u8 coord = (u8)(random >> 8);
			u8 new_x[4] = {coord % cols, 0, coord % cols, cols - 1};
			u8 new_y[4] = {rows - 1, coord % rows, 0, coord % rows};

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

	const int8x16_t x_deltas_v = {0, 1, 0, -1};
	const int8x16_t y_deltas_v = {-1, 0, 1, 0};

	for (; i < pipe_count; i += 16) {
		uint8x16_t direction = vld1q_u8(directions + i);

		uint8x16_t x = vld1q_u8(xs + i);

		uint8x16_t y = vld1q_u8(ys + i);

		x += vqtbl1q_u8(x_deltas_v, direction);
		y += vqtbl1q_u8(y_deltas_v, direction);

		vst1q_u8(xs + i, x);
		vst1q_u8(ys + i, y);

		uint64x2_t random_big;
		random_big[0] = Rng_Next(&rng);
		random_big[1] = Rng_Next(&rng);
		uint8x16_t random = random_big;

		// either 0 or 1
		uint8x16_t should_apply = random & 1;

		// either -1 or 1
		uint8x16_t rotation = (random & 2) - 1;

		uint8x16_t old_direction = direction;
		vst1q_u8(old_directions + i, old_direction);

		direction = (direction + rotation * should_apply) & 3;
		vst1q_u8(directions + i, direction);

		uint8x16_t at_edge = x < 0 | x >= cols | y < 0 | y >= rows;
		for (usize j = 0; j < 16; j++) {
			if (at_edge[j]) {
				u8 coord = random[j];
				u8 new_x[4] = {coord % cols, 0, coord % cols,
				               cols - 1};
				u8 new_y[4] = {rows - 1, coord % rows, 0,
				               coord % rows};

				direction[j] = (random[j] >> 2) & 3;
				old_direction[j] = direction[j];
				xs[i + j] = new_x[direction[j]];
				ys[i + j] = new_y[direction[j]];
				directions[i + j] = direction[j];
				old_directions[i + j] = direction[j];
			}
		}

		for (usize j = 0; j < 16; j++) {
			memcpy(display[i + j],
			       pipes[old_direction[j] << 2 | direction[j]],
			       sizeof pipes[0]);
		}
	}

	*app->rng = rng;
}
