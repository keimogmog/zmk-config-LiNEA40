# LiNEA40 trackball SCROLL sensitivity experiment v2

User tested the [trackball scroll v1 (threshold 7)](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38354865519), said it could be even slower, and correctly noted that the encoder scroll and PMW3610 trackball scroll had **always been separately configurable**. They were already on separate code paths: `CONFIG_PMW3610_SCROLL_TICK` versus the encoder behavior. No new separation has been invented.

## One changed setting

Based on branch `experiment/trackball-scroll-slower-v1`, change only `config/boards/shields/LiNEA40/LiNEA40_right.conf`:

```diff
-CONFIG_PMW3610_SCROLL_TICK=7
+CONFIG_PMW3610_SCROLL_TICK=12
```

The [PMW3610 driver](https://github.com/inorichi/zmk-pmw3610-driver/blob/main/src/pmw3610.c) accumulates trackball movement in SCROLL mode and sends one wheel step when `abs(scroll_delta_y) > CONFIG_PMW3610_SCROLL_TICK`, resetting the accumulator after each report. Raising the threshold from >7 to >12 generally reduces the wheel report frequency (~8→13 input counts per report if movements arrive one sensor count at a time), although exact screen movement is affected by sample increments and macOS acceleration.

All existing scroll-layer input processors, cursor CPI, X/Y movement, orientation, high-resolution mouse HID setting, 24-trigger encoder v3.4, MARK v2, number layer and Bluetooth behavior are unmodified. Main and previous experiment branches remain untouched.

**Caveat:** More sensor movement is now needed before the *first* wheel report. If the trackball feels sluggish when nudged slowly, this driver threshold approach has reached a practical limit. The next improvement would be more granular scroll HID reporting or a driver-side fractional accumulator, not just increasing this number.

## Flash and test

1. Check CI `left`, `right`, and merged `firmware` success before downloading.
2. Flash matching left/right UF2 from this branch; do **not** flash `settings_reset`.
3. Compare SCROLL-layer slow/fast ball movement and normal pointer movement with the same macOS scroll-speed slider setting.
4. If this is too slow or has a noticeable dead zone, revert to [v1 threshold 7](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38354865519). To revert further, [encoder v3.4 and trackball threshold 3](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38353241803).
5. No changes to `main` without explicit hardware feedback and approval.
