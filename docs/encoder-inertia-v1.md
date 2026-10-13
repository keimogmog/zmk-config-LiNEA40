# LiNEA40 encoder inertia — experimental tuning v1.1

**Status:** Experimental (must pass CI and hardware testing before merging). Based on `experiment/us-symbol-layer-v2`. No change to `main` or original v2 firmware.

## Scope

- **Encoder only**: modify the existing `mouse_scrl` sensor-rotate behavior to send detent events to a custom ZMK behavior, instead of generating 15 ms `&msc` holds.
- **Trackball unchanged**: the `trackball_listener`, `&msc`, and separate trackball `SCROLL` processing remain unchanged. The `FUNCTION` layer's 3x wheel processor setting remains as before, but will not affect the custom encoder events unless explicitly routed through it.
- **MARK v2 stays**: symbol-only US layout retained; CURSOR number layout retained.
- **Non-scroll encoder layers stay**: brightness, volume, and arrows continue to use their existing `sensor-bindings`.

## Proposed behavior (starting feel)

1. An isolated or slow encoder detent immediately emits **two** wheel ticks; no synthetic tail. Rapid same-direction (<90 ms) detents emit **one** wheel tick, preserving v1 fast-spin response.
2. Repeated same-direction detents within 155 ms build small inertia. Faster (<90 ms) detents add a larger impulse.
3. The tail ticks approximately every 16 ms and decays to zero within **up to 176 ms after the latest quick detent** (11 x 16 ms).
4. A turn in the opposite direction cancels remaining inertia and immediately emits one tick in the new direction.
5. Velocity is clamped and fractional ticks are accumulated so the tail cannot emit unbounded scroll.

All parameters are set in shared `config/boards/shields/LiNEA40/LiNEA40.dtsi` at the `inertial_scroll` node; the custom behavior is `src/behaviors/behavior_inertial_scroll.c` and the config is a Zephyr module (`zephyr/module.yml`).

| Parameter | v1 |
|--|--:|
| frame-ms | 16 |
| tail-frames | 11 |
| rapid-threshold-ms | 90 |
| grouping-threshold-ms | 155 |
| rapid-impulse-q8 | 120 |
| grouped-impulse-q8 | 65 |
| velocity-cap-q8 | 400 |
| decay-percent | 76 |
| immediate wheel units, isolated/slow detent | 2 |
| immediate wheel units, rapid same-direction detent | 1 |

**Design limitation**: This sends ordinary HID wheel events. It is **not** macOS native trackpad touch momentum, and apps can handle HID wheel movement differently. A tiny tail may be swallowed by integer tick quantization.

## Test safely

1. Confirm the latest CI build finishes with **left and right success**. `main` and `experiment/us-symbol-layer-v2` provide known-good rollback images.
2. Download the `firmware` artifact from the latest successful GitHub Actions run in **this branch**.
3. Flash the left and right `.uf2` files to their respective halves; do **not** flash `settings_reset` unless troubleshooting pairing.
4. In a text editor or browser, check: slow single detent, medium continuous rotation, fast flick, stop, immediate opposite-direction rotation.
5. Verify MARK v2 symbols, number layer, trackball and scroll mode, volume/brightness/arrow encoder layers, and Bluetooth.
6. If the wheel is worse, re-flash the known-good `experiment/us-symbol-layer-v2` artifact (GitHub Actions run 38321638555).

### Field feedback and rationale

User reported v1 fast inertial feel was good but **slow, one-notch scrolling felt unresponsive**. v1 used a 1-unit HID wheel step for every detent, replacing the earlier 15 ms `&msc` behavior. v1.1 boosts only the slow/isolated detent to 2 units, leaving fast-scroll direct ticks and inertial tail unchanged. **This is a hypothesis to check on the actual Mac**; wheel movement is app/OS dependent. The previously confirmed v1 artifact is GitHub Actions run `38325701553` for A/B comparison.

No automatic deploy to the physical device is performed. Do not merge to `main` until hardware observations justify it.
