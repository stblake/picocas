(* --------------------------------------------------------------------
 * deriv.m -- (historical)
 *
 * The partial / total / higher-order derivative rules that used to
 * live here have been replaced by a native C implementation in
 * src/deriv.c. D, Dt and Derivative are now registered directly from
 * core_init() and no longer require bootstrapping through the pattern
 * matcher. This file is retained as a no-op for two reasons:
 *
 *   1. init.m still issues `Get["src/internal/deriv.m"]`; keeping the
 *      file lets anyone upgrading in place continue to boot.
 *   2. Users can drop additional rule-based augmentations here
 *      (e.g. custom Dt identities) without rebuilding.
 * -------------------------------------------------------------------- *)
