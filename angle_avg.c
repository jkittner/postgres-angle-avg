#include "postgres.h"

#include <complex.h>
#include <math.h>

#include "catalog/pg_type.h"
#include "utils/array.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(complex_angle_accum);

Datum complex_angle_accum(PG_FUNCTION_ARGS) {
  /*
   * The state is an array of [N, real, imag]} where:
   * N: current number of aggregations
   * real: real part of the complex number
   * imag: imaginary part of the complex number
   */
  ArrayType *state = PG_GETARG_ARRAYTYPE_P(0);
  // new value which will be added (accumulated) to the state
  float8 new_val = PG_GETARG_FLOAT8(1);
  // We do not support values outside of 0 - 360° to avoid surprising behavior
  if (new_val < 0.0 || new_val > 360.0) {
    elog(ERROR, "Invalid value encountered %f", new_val);
  }
  float8 *vals_state;
  float8 real, imag, N, new_val_rad;

  vals_state = (float8 *)ARR_DATA_PTR(state);

  N = vals_state[0];
  real = vals_state[1];
  imag = vals_state[2];

  // conversion of the new value to radians
  new_val_rad = new_val * (M_PI / 180.0);

  /*
   * increase the total number of accumulations and calculate real and
   * imaginary part of the complex number
   */
  N += 1;
  real += cos(new_val_rad);
  imag += sin(new_val_rad);

  Datum datums[3];
  ArrayType *result;

  // construct the new datum for further aggregation
  datums[0] = Float8GetDatumFast(N);
  datums[1] = Float8GetDatumFast(real);
  datums[2] = Float8GetDatumFast(imag);

  result = construct_array(datums, 3, FLOAT8OID, sizeof(float8),
                           FLOAT8PASSBYVAL, 'd');
  PG_RETURN_ARRAYTYPE_P(result);
}

PG_FUNCTION_INFO_V1(complex_angle_avg);

Datum complex_angle_avg(PG_FUNCTION_ARGS) {
  /*
   * The state is an array of [N, real, imag]} where:
   * N: current number of aggregations
   * real: real part of the complex number
   * imag: imaginary part of the complex number
   */
  ArrayType *state = PG_GETARG_ARRAYTYPE_P(0);
  float8 *vals_state;
  float8 N, real, imag, real_mean, imag_mean, phase_angle, mean_deg;

  vals_state = (float8 *)ARR_DATA_PTR(state);
  N = vals_state[0];
  real = vals_state[1];
  imag = vals_state[2];

  // no values, nothing to aggregate
  if (N == 0.0) {
    PG_RETURN_NULL();
  } else {
    // calculate the mean of real and imaginary part of the complex number
    real_mean = real / N;
    imag_mean = imag / N;
  }
  phase_angle = carg(CMPLX(real_mean, imag_mean));
  mean_deg = phase_angle * (180.0 / M_PI);
  while (mean_deg < 0.0) {
    mean_deg += 360.0;
  }
  PG_RETURN_FLOAT8(mean_deg);
}

PG_FUNCTION_INFO_V1(complex_angle_combine);

Datum complex_angle_combine(PG_FUNCTION_ARGS) {
  /*
   * The state is an array of [N, real, imag]} where:
   * N: current number of aggregations
   * real: real part of the complex number
   * imag: imaginary part of the complex number
   *
   * We get two of those array from different parallel processes and need
   * to combine them to one accumulation
   */
  ArrayType *state1 = PG_GETARG_ARRAYTYPE_P(0);
  ArrayType *state2 = PG_GETARG_ARRAYTYPE_P(1);
  float8 *state1_vals;
  float8 *state2_vals;
  float8 N1, real1, imag1, N2, real2, imag2, N, real, imag;

  state1_vals = (float8 *)ARR_DATA_PTR(state1);
  state2_vals = (float8 *)ARR_DATA_PTR(state2);

  N1 = state1_vals[0];
  real1 = state1_vals[1];
  imag1 = state1_vals[2];

  N2 = state2_vals[0];
  real2 = state2_vals[1];
  imag2 = state2_vals[2];

  // if one of the array has no values return the other half
  if (N1 == 0.0) {
    N = N2;
    real = real2;
    imag = imag2;
  } else if (N2 == 0.0) {
    N = N1;
    real = real1;
    imag = imag1;
  }
  // both arrays have values, combine them
  else {
    N = N1 + N2;
    real = real1 + real2;
    imag = imag1 + imag2;
  }

  Datum datums[3];
  ArrayType *result;

  /*
   * constrcut a new datum for passing it to the final averaging function or
   * more combines
   */
  datums[0] = Float8GetDatumFast(N);
  datums[1] = Float8GetDatumFast(real);
  datums[2] = Float8GetDatumFast(imag);

  result = construct_array(datums, 3, FLOAT8OID, sizeof(float8),
                           FLOAT8PASSBYVAL, 'd');
  PG_RETURN_ARRAYTYPE_P(result);
}
