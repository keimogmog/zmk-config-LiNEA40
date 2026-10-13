# Trackball-only scroll sensitivity trial

This is an experimental **single-setting change** on top of the existing [LiNEA40 encoder v3.4](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38353241803). The user reported that trackball scroll is too fast while the macOS Mouse scroll-speed slider is already at minimum. The encoder v3.4 tuning is untouched.

## Actual source path

The connected PMW3610 driver at `inorichi/zmk-pmw3610-driver/src/pmw3610.c` has separate handling for `input_mode != SCROLL` vs `input_mode == SCROLL`. In SCROLL mode it accumulates motion in `scroll_delta_y` and sends an `INPUT_REL_WHEEL` packet once `abs(scroll_delta_y) > CONFIG_PMW3610_SCROLL_TICK`, resetting the delta afterward. In pointer mode it sends X/Y movement directly instead.

## Change

- In `config/boards/shields/LiNEA40/LiNEA40_right.conf` only, change `CONFIG_PMW3610_SCROLL_TICK=3` to `=7`.
- Approximate intent: threshold from >3 to >7 sensor counts, ~half as many wheel packets for equivalent trackball travel *under small consistent deltas*. Actual perceived macOS scroll distance may vary due to scroll acceleration and sampling/overshoot.
- No changes to `CONFIG_PMW3610_CPI=600`, pointing movement, ZMK v0.3.0, Bluetooth/HID setup, trackball orientation, keymaps, MARK, encoder v3.4, inertia thresholds or the shared macOS slider.
- **Tradeoff:** larger scroll threshold can feel less responsive to tiny ball movements, or produce a few more counts of dead zone before the first wheel report. We will evaluate on hardware.

## Validation and rollback

1. Confirm GitHub Actions left, right and merged `firmware` artifact are all **SUCCESS**.
2. From this experiment's `firmware` artifact flash LEFT file to left and RIGHT file to right. Do not flash `settings_reset`.
3. Keep macOS Mouse scroll-speed slider at the **same setting** as before. Use the trackball in SCROLL layer to compare slow/fine and fast movement. Verify normal pointer movement is unchanged.
4. Verify the encoder still behaves like v3.4, and other layers/keys still work.
5. If the trackball is too slow or feels stepped, next compare threshold **5 or 6** (or a wheel input processor with remainder) on a separate experiment; if still too fast, **9 or 10**.
6. Roll back by flashing the successful [encoder v3.4 build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38353241803). Existing branches and `main` are not touched.

The build validates firmware compatibility, not real-world scroll feel.
