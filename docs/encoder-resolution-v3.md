# Encoder physical-resolution experiment v3 — 24 triggers/rotation

**Hardware feedback:** v2 high-res and inertia scrolled too far; v2-lowgain improved travel but slow scrolling was visibly jumpy/stuttered. This branch is isolated from `main`, MARK v2, v1.1 and v2-lowgain.

## What changed (and why)

The EC11 sensor is configured with `steps = <24>` (ZMK's "encoder pulses per full rotation"), but the shared keymap sensor had `triggers-per-rotation = <8>` (only eight ZMK sensor-behavior activations per revolution, or one for every three *configured pulses*). Upstream ZMK documents that these are independent; the behavior trigger count should often match physical detents for detented encoders. **Actual encoder detent count has not been independently measured**, so 24 is a hypothesis to validate on hardware, not a proven mechanical specification.

The previous lowgain firmware then divided each coarse 4-unit wheel event into `2 + 1 + 1` HID packets over 32 ms. That cannot respond to the intermediary encoder movement before the *next* coarse ZMK trigger, and may add jumpiness.

This test:

- Sets `triggers-per-rotation = <24>` (3× as many sensor behaviors per one revolution).
- Preserves the approximate **total wheel movement per full revolution** by distributing the previous 4 high-res units across three new sensor events using integer remainder tracking: `1, 1, 2` fine HID wheel units (in either direction). There is no 32-ms spreading for each slow detent.
- Keeps the ZMK v0.3.0 high-resolution HID feature and MAC/BLE configuration of v2-lowgain.
- Adjusts the inertia grouping threshold from 90 ms to 45 ms, rapid threshold 75 ms to 30 ms, and the impulses/cap downward so gentle turns do not generate large coast.
- Uses `k_work_schedule` rather than `k_work_reschedule` on fresh sensor impulses, avoiding deferring a scheduled scroll tick indefinitely while spinning.
- Cancels existing momentum on reversal; resets partial fractional units when direction changes.

**IMPORTANT side effect:** `triggers-per-rotation` applies to the encoder in **all layers**, not just the scrolling layer. Brightness, volume, arrow and other sensor-rotate actions may now occur up to 3× as often per revolution. This is *intentionally a short-lived diagnostic test*. If smoother physical tracking is confirmed, we should add 3:1 rate limiting to those non-scroll layer bindings before considering a merge to `main`. No change to MARK keys, number layer, trackball sensor or trackball firmware.

## Expected feel

- Small slow movements are sensed sooner and should not wait for three encoder pulses between scroll events.
- A quick spin may show reduced inertia compared to v2; that's acceptable for this test, because the goal is to isolate slow-scroll smoothness.
- Total travel per turn should be similar to v2-lowgain **if** the physical encoder really reports 24 pulses per full revolution and macOS negotiates high-resolution HID correctly.

## Test plan

1. Confirm `left`, `right`, and `Merge Output Artifacts` jobs all succeeded before downloading this branch's `firmware` ZIP.
2. Flash matching left/right UF2. **Do not flash `settings_reset`**.
3. In macOS Safari, scroll very slowly with each notch. Compare whether you see ~3 times more update points and more even movement. Try a single detent and immediate reverse.
4. Spin quickly and observe inertia; expect lower tail strength.
5. **Check volume, brightness and layer-specific sensor bindings**: these are expected to be faster in this experiment. Do not leave an unwanted volume behavior in regular use.
6. Check trackball pointer and SCROLL layer as a regression; high-resolution BLE feature is shared.
7. Roll back by reflashing the successful [v2-lowgain firmware](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38332461574). Pairing/descriptor changes are not introduced by this branch beyond the previous v2 feature.

If moving a *single* physical notch still produces **no scrolling at all**, the issue could instead be encoder contacts/transition geometry or OS quantization. Instrument the pulses before selecting a definitive detent count.
