#include "all.h"

Rng
Rng_CreateWithSystemEntropy(void)
{
	u64 seed = 0;
	if (getentropy(&seed, sizeof seed) != 0)
		Die();
	return (Rng){.s = seed};
}

u64
Rng_Next(Rng *rng)
{
	u64 x = rng->s;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	rng->s = x;
	return x;
}
