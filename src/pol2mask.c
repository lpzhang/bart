/* Copyright 2020. Martin Uecker.
 * All rights reserved. Use of this source code is governed by
 * a BSD-style license which can be found in the LICENSE file.
 */

#include <stdbool.h>
#include <complex.h>
#include <math.h>

#include "num/init.h"
#include "num/multind.h"
#include "num/loop.h"

#include "geom/polygon.h"

#include "misc/mmio.h"
#include "misc/misc.h"
#include "misc/nested.h"
#include "misc/debug.h"
#include "misc/opts.h"

#ifndef DIMS
#define DIMS 16
#endif

#ifndef CFL_SIZE
#define CFL_SIZE sizeof(complex float)
#endif

static const char usage_str[] = "<poly> <output>";
static const char help_str[] = "Compute masks from polygons.";



int main_pol2mask(int argc, char* argv[argc])
{
	int X = 100;
	int Y = 100;

	const struct opt_s opts[] = {

		OPT_INT('X', &X, "size", "size dimension 0"),
		OPT_INT('Y', &Y, "size", "size dimension 1"),
	};

	cmdline(&argc, argv, 2, 2, usage_str, help_str, ARRAY_SIZE(opts), opts);

	num_init();

	long pdims[DIMS];
	long odims[DIMS];

	complex float* pol = load_cfl(argv[1], DIMS, pdims);

	assert(2 == pdims[0]);

	int N = pdims[1];
	int P = pdims[2];

	long pstrs[DIMS];
	md_calc_strides(DIMS, pstrs, pdims, CFL_SIZE);

	md_copy_dims(DIMS, odims, pdims);
	odims[0] = X;
	odims[1] = Y;
	odims[2] = 1;

	complex float* out = create_cfl(argv[2], DIMS, odims);

	NESTED(complex float, sample, (const long pos[]))
	{
		int sum = 0;

		long pos2[DIMS];
		md_select_dims(DIMS, ~7UL, pos2, pos);

		for (int i = 0; i < P; i++) {

			double pg[N][2];

			for (int j = 0; j < N; j++) {

				pos2[1] = j;

				pg[j][0] = crealf(MD_ACCESS(DIMS, pstrs, (pos2[0] = 0, pos2), pol));
				pg[j][1] = crealf(MD_ACCESS(DIMS, pstrs, (pos2[0] = 1, pos2), pol));
			}

			sum += polygon_winding_number(N, pg, (double[2]){ pos[0], pos[1] });
		}

		return sum;
	};

	md_parallel_zsample(DIMS, odims, out, sample);

	unmap_cfl(DIMS, pdims, pol);
	unmap_cfl(DIMS, odims, out);

	return 0;
}



