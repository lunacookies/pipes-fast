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

	for (; i < (pipe_count % 64); i++) {
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

	for (; i < pipe_count; i += 64) {
		uint8x16_t direction0 = vld1q_u8(directions + i);
		uint8x16_t direction1 = vld1q_u8(directions + i + 16);
		uint8x16_t direction2 = vld1q_u8(directions + i + 32);
		uint8x16_t direction3 = vld1q_u8(directions + i + 48);

		uint8x16_t x0 = vld1q_u8(xs + i);
		uint8x16_t x1 = vld1q_u8(xs + i + 16);
		uint8x16_t x2 = vld1q_u8(xs + i + 32);
		uint8x16_t x3 = vld1q_u8(xs + i + 48);

		uint8x16_t y0 = vld1q_u8(ys + i);
		uint8x16_t y1 = vld1q_u8(ys + i + 16);
		uint8x16_t y2 = vld1q_u8(ys + i + 32);
		uint8x16_t y3 = vld1q_u8(ys + i + 48);

		x0 += vqtbl1q_u8(x_deltas_v, direction0);
		x1 += vqtbl1q_u8(x_deltas_v, direction1);
		x2 += vqtbl1q_u8(x_deltas_v, direction2);
		x3 += vqtbl1q_u8(x_deltas_v, direction3);

		y0 += vqtbl1q_u8(y_deltas_v, direction0);
		y1 += vqtbl1q_u8(y_deltas_v, direction1);
		y2 += vqtbl1q_u8(y_deltas_v, direction2);
		y3 += vqtbl1q_u8(y_deltas_v, direction3);

		vst1q_u8(xs + i, x0);
		vst1q_u8(xs + i + 16, x1);
		vst1q_u8(xs + i + 32, x2);
		vst1q_u8(xs + i + 48, x3);

		vst1q_u8(ys + i, y0);
		vst1q_u8(ys + i + 16, y1);
		vst1q_u8(ys + i + 32, y2);
		vst1q_u8(ys + i + 48, y3);

		uint64x2_t random_big;
		random_big[0] = Rng_Next(&rng);
		random_big[1] = Rng_Next(&rng);
		uint8x16_t random0 = random_big;
		random_big[0] = Rng_Next(&rng);
		random_big[1] = Rng_Next(&rng);
		uint8x16_t random1 = random_big;
		random_big[0] = Rng_Next(&rng);
		random_big[1] = Rng_Next(&rng);
		uint8x16_t random2 = random_big;
		random_big[0] = Rng_Next(&rng);
		random_big[1] = Rng_Next(&rng);
		uint8x16_t random3 = random_big;

		// either 0 or 1
		uint8x16_t should_apply0 = random0 & 1;
		uint8x16_t should_apply1 = random1 & 1;
		uint8x16_t should_apply2 = random2 & 1;
		uint8x16_t should_apply3 = random3 & 1;

		// either -1 or 1
		uint8x16_t rotation0 = (random0 & 2) - 1;
		uint8x16_t rotation1 = (random1 & 2) - 1;
		uint8x16_t rotation2 = (random2 & 2) - 1;
		uint8x16_t rotation3 = (random3 & 2) - 1;

		uint8x16_t old_direction0 = direction0;
		uint8x16_t old_direction1 = direction1;
		uint8x16_t old_direction2 = direction2;
		uint8x16_t old_direction3 = direction3;

		vst1q_u8(old_directions + i, old_direction0);
		vst1q_u8(old_directions + i + 16, old_direction1);
		vst1q_u8(old_directions + i + 32, old_direction2);
		vst1q_u8(old_directions + i + 48, old_direction3);

		direction0 = (direction0 + rotation0 * should_apply0) & 3;
		direction1 = (direction1 + rotation1 * should_apply1) & 3;
		direction2 = (direction2 + rotation2 * should_apply2) & 3;
		direction3 = (direction3 + rotation3 * should_apply3) & 3;

		vst1q_u8(directions + i, direction0);
		vst1q_u8(directions + i + 16, direction1);
		vst1q_u8(directions + i + 32, direction2);
		vst1q_u8(directions + i + 48, direction3);

		uint8x16_t at_edge0 = x0 < 0 | x0 >= cols | y0 < 0 | y0 >= rows;
		uint8x16_t at_edge1 = x1 < 0 | x1 >= cols | y1 < 0 | y1 >= rows;
		uint8x16_t at_edge2 = x2 < 0 | x2 >= cols | y2 < 0 | y2 >= rows;
		uint8x16_t at_edge3 = x3 < 0 | x3 >= cols | y3 < 0 | y3 >= rows;

		for (usize j = 0; j < 16; j++) {
			if (at_edge0[j]) {
				u8 coord = random0[j];
				u8 new_x[4] = {coord % cols, 0, coord % cols,
				               cols - 1};
				u8 new_y[4] = {rows - 1, coord % rows, 0,
				               coord % rows};

				direction0[j] = (random0[j] >> 2) & 3;
				old_direction0[j] = direction0[j];
				xs[i + j] = new_x[direction0[j]];
				ys[i + j] = new_y[direction0[j]];
				directions[i + j] = direction0[j];
				old_directions[i + j] = direction0[j];
			}
		}

		for (usize j = 0; j < 16; j++) {
			if (at_edge1[j]) {
				u8 coord = random1[j];
				u8 new_x[4] = {coord % cols, 0, coord % cols,
				               cols - 1};
				u8 new_y[4] = {rows - 1, coord % rows, 0,
				               coord % rows};

				direction1[j] = (random1[j] >> 2) & 3;
				old_direction1[j] = direction1[j];
				xs[i + j + 16] = new_x[direction1[j]];
				ys[i + j + 16] = new_y[direction1[j]];
				directions[i + j + 16] = direction1[j];
				old_directions[i + j + 16] = direction1[j];
			}
		}

		for (usize j = 0; j < 16; j++) {
			if (at_edge2[j]) {
				u8 coord = random2[j];
				u8 new_x[4] = {coord % cols, 0, coord % cols,
				               cols - 1};
				u8 new_y[4] = {rows - 1, coord % rows, 0,
				               coord % rows};

				direction2[j] = (random2[j] >> 2) & 3;
				old_direction2[j] = direction2[j];
				xs[i + j + 32] = new_x[direction2[j]];
				ys[i + j + 32] = new_y[direction2[j]];
				directions[i + j + 32] = direction2[j];
				old_directions[i + j + 32] = direction2[j];
			}
		}

		for (usize j = 0; j < 16; j++) {
			if (at_edge3[j]) {
				u8 coord = random3[j];
				u8 new_x[4] = {coord % cols, 0, coord % cols,
				               cols - 1};
				u8 new_y[4] = {rows - 1, coord % rows, 0,
				               coord % rows};

				direction3[j] = (random3[j] >> 2) & 3;
				old_direction3[j] = direction3[j];
				xs[i + j + 48] = new_x[direction3[j]];
				ys[i + j + 48] = new_y[direction3[j]];
				directions[i + j + 48] = direction3[j];
				old_directions[i + j + 48] = direction3[j];
			}
		}

		uint8x16_t indexes0 = old_direction0 << 2 | direction0;
		uint8x16_t indexes1 = old_direction1 << 2 | direction1;
		uint8x16_t indexes2 = old_direction2 << 2 | direction2;
		uint8x16_t indexes3 = old_direction3 << 2 | direction3;

		for (usize j = 0; j < 16; j++) {
			memcpy(display[i + j], pipes[indexes0[j]],
			       sizeof pipes[0]);
		}
		for (usize j = 0; j < 16; j++) {
			memcpy(display[i + j + 16], pipes[indexes1[j]],
			       sizeof pipes[0]);
		}
		for (usize j = 0; j < 16; j++) {
			memcpy(display[i + j + 32], pipes[indexes2[j]],
			       sizeof pipes[0]);
		}
		for (usize j = 0; j < 16; j++) {
			memcpy(display[i + j + 48], pipes[indexes3[j]],
			       sizeof pipes[0]);
		}
	}

	*app->rng = rng;
}
