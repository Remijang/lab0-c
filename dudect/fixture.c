/** dude, is my code constant time?
 *
 * This file measures the execution time of a given function many times with
 * different inputs and performs a Welch's t-test to determine if the function
 * runs in constant time or not. This is essentially leakage detection, and
 * not a timing attack.
 *
 * Notes:
 *
 *  - the execution time distribution tends to be skewed towards large
 *    timings, leading to a fat right tail. Most executions take little time,
 *    some of them take a lot. We try to speed up the test process by
 *    throwing away those measurements with large cycle count. (For example,
 *    those measurements could correspond to the execution being interrupted
 *    by the OS.) Setting a threshold value for this is not obvious; we just
 *    keep the x% percent fastest timings, and repeat for several values of x.
 *
 *  - the previous observation is highly heuristic. We also keep the uncropped
 *    measurement time and do a t-test on that.
 *
 *  - we also test for unequal variances (second order test), but this is
 *    probably redundant since we're doing as well a t-test on cropped
 *    measurements (non-linear transform)
 *
 *  - as long as any of the different test fails, the code will be deemed
 *    variable time.
 */

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../console.h"
#include "../random.h"

#include "constant.h"
#include "fixture.h"
#include "ttest.h"

#define ENOUGH_MEASURE 10000
#define TEST_TRIES 10

#define N_PERCENTILES (100)

#define TESTS (1 + N_PERCENTILES + 1)

static t_context_t *t[TESTS];

/* threshold values for Welch's t-test */
enum {
    t_threshold_bananas = 500, /* Test failed with overwhelming probability */
    t_threshold_moderate = 10, /* Test failed */
};

static void __attribute__((noreturn)) die(void)
{
    exit(111);
}

static void differentiate(int64_t *exec_times,
                          const int64_t *before_ticks,
                          const int64_t *after_ticks)
{
    for (size_t i = 0; i < N_MEASURES; i++)
        exec_times[i] = after_ticks[i] - before_ticks[i];
}

static void update_statistics(const int64_t *exec_times,
                              uint8_t *classes,
                              const int64_t *percentiles)
{
    for (size_t i = DROP_SIZE; i < N_MEASURES - DROP_SIZE; i++) {
        int64_t difference = exec_times[i];
        /* CPU cycle counter overflowed or dropped measurement */
        if (difference <= 0)
            continue;

        /* do a t-test on the execution time */
        t_push(t[0], difference, classes[i]);

        for (size_t crop_index = 0; crop_index < N_PERCENTILES; crop_index++) {
            if (difference < percentiles[crop_index]) {
                t_push(t[crop_index + 1], difference, classes[i]);
            }
        }

        if (t[0]->n[0] > 10000) {
            double centered = (double) difference - t[0]->mean[classes[i]];
            t_push(t[1 + N_PERCENTILES], centered * centered, classes[i]);
        }
    }
}

static int cmp(const int64_t *a, const int64_t *b)
{
    if (*a == *b)
        return 0;
    return (*a > *b) ? 1 : -1;
}

static int64_t percentile(int64_t *a_sorted, double which, size_t size)
{
    size_t array_position = (size_t) ((double) size * (double) which);
    assert(array_position < size);
    return a_sorted[array_position];
}

static void prepare_percentiles(int64_t *exec_times, int64_t *percentiles)
{
    qsort(exec_times, N_MEASURES, sizeof(int64_t),
          (int (*)(const void *, const void *)) cmp);
    for (size_t i = 0; i < N_PERCENTILES; i++) {
        percentiles[i] = percentile(
            exec_times, 1 - (pow(0.5, 10 * (double) (i + 1) / N_PERCENTILES)),
            N_MEASURES);
    }
}

static t_context_t *max_test()
{
    size_t ret = 0;
    double max = 0;
    for (size_t i = 1; i < TESTS; i++) {
        if (t[i]->n[0] > ENOUGH_MEASURE) {
            double x = fabs(t_compute(t[i]));
            if (max < x) {
                max = x;
                ret = i;
            }
        }
    }
    return t[ret];
}

static bool report(void)
{
    t_context_t *t_max = max_test();
    double max_t = fabs(t_compute(t_max));
    double number_traces_max_t = t_max->n[0] + t_max->n[1];
    double max_tau = max_t / sqrt(number_traces_max_t);

    printf("\033[A\033[2K");
    printf("meas: %7.2lf M, ", (number_traces_max_t / 1e6));
    if (number_traces_max_t < ENOUGH_MEASURE) {
        printf("not enough measurements (%.0f still to go).\n",
               ENOUGH_MEASURE - number_traces_max_t);
        return false;
    }

    /* max_t: the t statistic value
     * max_tau: a t value normalized by sqrt(number of measurements).
     *          this way we can compare max_tau taken with different
     *          number of measurements. This is sort of "distance
     *          between distributions", independent of number of
     *          measurements.
     * (5/tau)^2: how many measurements we would need to barely
     *            detect the leak, if present. "barely detect the
     *            leak" = have a t value greater than 5.
     */
    printf("max t: %+7.2f, max tau: %.2e, (5/tau)^2: %.2e.\n", max_t, max_tau,
           (double) (5 * 5) / (double) (max_tau * max_tau));

    /* Definitely not constant time */
    if (max_t > t_threshold_bananas)
        return false;

    /* Probably not constant time. */
    if (max_t > t_threshold_moderate)
        return false;

    /* For the moment, maybe constant time. */
    return true;
}

static bool doit(int64_t *before_ticks,
                 int64_t *after_ticks,
                 int64_t *exec_times,
                 uint8_t *classes,
                 uint8_t *input_data,
                 int64_t *percentiles,
                 int mode)
{
    prepare_inputs(input_data, classes);

    measure(before_ticks, after_ticks, input_data, mode);
    differentiate(exec_times, before_ticks, after_ticks);
    bool ret = true;

    bool first_time = percentiles[N_PERCENTILES - 1] == 0;
    if (first_time) {
        prepare_percentiles(exec_times, percentiles);
    } else {
        update_statistics(exec_times, classes, percentiles);
        ret = report();
    }

    return ret;
}

static void init_once(void)
{
    init_dut();
    for (int i = 0; i < TESTS; ++i)
        t_init(t[i]);
}

static bool test_const(char *text, int mode)
{
    int64_t *before_ticks = calloc(N_MEASURES + 1, sizeof(int64_t));
    int64_t *after_ticks = calloc(N_MEASURES + 1, sizeof(int64_t));
    int64_t *exec_times = calloc(N_MEASURES, sizeof(int64_t));
    uint8_t *classes = calloc(N_MEASURES, sizeof(uint8_t));
    uint8_t *input_data = calloc(N_MEASURES * CHUNK_SIZE, sizeof(uint8_t));
    if (!before_ticks || !after_ticks || !exec_times || !classes ||
        !input_data) {
        die();
    }
    for (int i = 0; i < TESTS; ++i) {
        t[i] = malloc(sizeof(t_context_t));
        if (!t[i])
            die();
    }

    bool result = false;

    for (int cnt = 0; cnt < TEST_TRIES; ++cnt) {
        printf("Testing %s...(%d/%d)\n\n", text, cnt, TEST_TRIES);
        /* initialization and prepare_percentiles */
        init_once();
        int64_t *percentiles = calloc(N_PERCENTILES, sizeof(int64_t));
        if (!percentiles)
            die();
        result = doit(before_ticks, after_ticks, exec_times, classes,
                      input_data, percentiles, mode);

        for (int i = 0; i < ENOUGH_MEASURE / (N_MEASURES - DROP_SIZE * 2) + 1;
             ++i)
            result = doit(before_ticks, after_ticks, exec_times, classes,
                          input_data, percentiles, mode);
        printf("\033[A\033[2K\033[A\033[2K");

        free(percentiles);
        if (result)
            break;
    }

    for (int i = 0; i < TESTS; ++i)
        free(t[i]);
    free(before_ticks);
    free(after_ticks);
    free(exec_times);
    free(classes);
    free(input_data);

    return result;
}

#define DUT_FUNC_IMPL(op)                \
    bool is_##op##_const(void)           \
    {                                    \
        return test_const(#op, DUT(op)); \
    }

#define _(x) DUT_FUNC_IMPL(x)
DUT_FUNCS
#undef _
