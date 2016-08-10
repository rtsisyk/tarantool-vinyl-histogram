#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>

#include <tarantool/module.h>
#include <lua.h>
#include <lauxlib.h>
#include <msgpuck.h>

#include "nb_histogram.h"

int
wl_upserts_uint_f(va_list ap)
{
	uint32_t space_id = va_arg(ap, uint32_t);
	uint32_t index_id = va_arg(ap, uint32_t);
	double duration = va_arg(ap, double);
	struct nb_histogram *hist = va_arg(ap, struct nb_histogram *);
	fiber_sleep(0);

	double stop_time = fiber_time() + duration;
 	while (fiber_time() < stop_time) {
		char tuple[128];
		char *tuple_end = tuple;
		tuple_end = mp_encode_array(tuple_end, 2);
		tuple_end = mp_encode_uint(tuple_end, rand());
		tuple_end = mp_encode_uint(tuple_end, 0);
		assert(tuple_end <= tuple + sizeof(tuple));

		char ops[128];
		char *ops_end = ops;
		ops_end = mp_encode_array(ops_end, 1);
		ops_end = mp_encode_array(ops_end, 3);
		ops_end = mp_encode_str(ops_end, "+", 1);
		ops_end = mp_encode_uint(ops_end, 1); /* field_id */
		ops_end = mp_encode_uint(ops_end, 1); /* += 1 */
		assert(ops_end <= ops + sizeof(ops));

		double start = clock_monotonic();
		if (box_upsert(space_id, index_id, tuple, tuple_end,
			       ops, ops_end, 0, NULL) != 0)
			return -1;
		double stop = clock_monotonic();
		nb_histogram_add(hist, stop - start);
	}

	return 0;
}

static int
luaL_bench_upserts_uint(lua_State *L)
{
	const uint32_t SPACE_ID = 512;
	const uint32_t INDEX_ID = 0;
	const int FIBER_COUNT = 10;
	const double DURATION = 20.0;
	double PERCENTILES[] = { 0.05, 0.50, 0.95, 0.96, 0.97, 0.98, 0.99,
		0.995, 0.999, 0.9995, 0.9999 };
	size_t PERCENTILES_SIZE = sizeof(PERCENTILES) / sizeof(PERCENTILES[0]);

	struct nb_histogram *hist = nb_histogram_new(6);
	if (hist == NULL)
		return luaL_error(L, "Failed to create histogram");

	struct fiber *fibers[FIBER_COUNT];
	for (int f = 0; f < FIBER_COUNT; f++) {
		fibers[f] = fiber_new("cbench", wl_upserts_uint_f);
		if (fibers[f] == NULL) {
			while (--f >= 0) {
				fiber_join(fibers[f]);
			}
			nb_histogram_delete(hist);
			return -1;
		}
		fiber_set_joinable(fibers[f], true);
	}

	for (int f = 0; f < FIBER_COUNT; f++) {
		fiber_start(fibers[f], SPACE_ID, INDEX_ID, DURATION, hist);
	}

	for (int f = 0; f < FIBER_COUNT; f++) {
		fiber_join(fibers[f]);
	}

	fprintf(stdout, "Histogram:\n");
	nb_histogram_dump(hist, stdout, PERCENTILES, PERCENTILES_SIZE);
	fflush(stdout);

	nb_histogram_delete(hist);

	return 0;
}

LUA_API int
luaopen_cbench_bench(lua_State *L)
{
	lua_newtable(L);
	static const struct luaL_reg meta [] = {
		{"upserts_uint", luaL_bench_upserts_uint },
		{NULL, NULL}
	};
	luaL_register(L, NULL, meta);
	return 1;
}
