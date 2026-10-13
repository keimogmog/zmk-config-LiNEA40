# LiNEA40 v3.2 A/B: firmware inertia OFF

**User request:** v3.2 scrolls smoothly on macOS but accelerates in an unnatural way during turning. Is the extra firmware inertia unnecessary?

This branch is based on **v3.2** (`experiment/encoder-resolution-v3-2-slower`, PR #7). It isolates the firmware momentum system while preserving the exact native sensor-scroll path for an A/B test. Neither `main` nor any working v3/v3.1/v3.2 branch was modified.

## Only runtime change — no added encoder momentum

In `config/boards/shields/LiNEA40/LiNEA40.dtsi`, inertial scroll configuration changes:

- `tail-frames = <11>` → `<0>`
- `rapid-impulse-q8 = <12>` → `<0>`
- `grouped-impulse-q8 = <5>` → `<0>`
- `velocity-cap-q8 = <60>` → `<0>`

The C behavior is deliberately **unchanged**: it continues to handle physical encoder events, integer remainder, HID wheel output, and direction transitions identically to v3.2. Its momentum parameters produce no velocity and zero frames of delayed momentum, so **no synthetic inertial tail** can be scheduled. At normal 16× negotiated high-resolution HID on v3.2, the physical wheel event is at most one HID unit; the interpolation remainder `smooth_remaining` stays zero too, so no deferred scroll reporting is scheduled.

Kept unchanged: `triggers-per-rotation = 24`, `steps = 24`, `hires-detent-units = 2`, `encoder-trigger-divisor = 3`, slow-scroll 0/1/1 pattern, all ZMK/macOS HID and BLE setup, MARK v2, number layer, trackball, other sensor bindings. The existing v3 test caveat remains: the global 24 trigger setting also affects other encoder layers. No extra OS software required.

**Important:** This disables **firmware-generated inertia only**. macOS may independently accelerate, animate or apply inertial effects to mouse wheel events. Removing firmware momentum is not a promise of perfectly linear pixel displacement.

## Physical test

1. Confirm left, right and Merge Output Artifacts all succeeded in the GitHub Actions run for this branch. Download `firmware` and flash the matching left/right UF2 files; **do not flash `settings_reset`**.
2. With the Mac scroll-speed setting unchanged during comparison, compare v3.2 and this OFF build in the same app and same window:
   - Very slow one-notch turning / reversal (same feel expected)
   - Gentle continuous turning (should feel directly proportional to rotation)
   - Rapid spin (not expected to produce **firmware** momentum)
   - **Release** the knob suddenly. Does the scroll end immediately or continue under macOS processing?
   - Normal volume, brightness, layer functions, and trackball SCROLL.
3. If v3.2 feels better, restore [v3.2 successful build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38349933498) with the matching UF2 pair.
4. To revert further, [v3.1 successful build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38349101220) remains available.

**Do not merge** until the user confirms actual feel. This is a controlled A/B diagnostic, not a final optimization.
