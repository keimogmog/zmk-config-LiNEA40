# LiNEA40 v3.4 — macOS scroll-speed headroom + stricter fast-only coast (experimental)

User report after flashing [v3.3](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38352497400): ordinary scrolling is quite good, but **sometimes a not-very-fast turn still makes a surprisingly long coast**. macOS **System Settings → Mouse → Scroll speed** slider was already at its **slowest position**. They want usable adjustment range.

## Goal

Fix two separate effects without a macOS utility:

1. Make the LiNEA40 encoder's baseline wheel output slower **without slowing the trackball** so macOS's Mouse scroll speed slider has some upward adjustment range.
2. Make firmware-generated inertia rarer and shorter; ordinary and moderately quick turns should behave like v3.2 **inertia OFF**. Extra wheel packets must only occur **after** a sustained fast spin and a 55ms gap.

This is a config-only change on a **new branch** based on [v3.3 fast-only-coast PR #9](https://github.com/keimogmog/zmk-config-LiNEA40/pull/9). `main`, v3, v3.1, v3.2, v3.2-no-inertia, and v3.3 remain unchanged.

## Changes compared with v3.3

| Param | v3.3 | v3.4 | Reason |
|---|---:|---:|---|
| `encoder-trigger-divisor` | 3 | **4** | Physical wheel reports: **16 → 12 HID fine units per 24-trigger revolution**, 25% less baseline travel if the host uses 16x |
| `rapid-threshold-ms` | 30 | **18** | Individual interval must be shorter to count as fast |
| `coast-min-fast-intervals` | 4 | **7** | Require 8 consecutive fast encoder events instead of 5 |
| `coast-wait-ms` | 55 | **55** | Stay silent until 55ms after last physical event |
| `tail-frames` | 9 | **6** | Shorter deceleration |
| `grouped-impulse-q8` | 140 | **90** | Lighter initial coast |
| `rapid-impulse-q8` | 16 | **8** | Smaller dependence on the most recent fast interval |
| `velocity-cap-q8` | 450 | **220** | Hard-limit synthetic tail |
| `decay-percent` | 78 | **70** | Faster attenuation |

Physical encoder resolution `steps = 24`, `triggers-per-rotation=24`, `hires-detent-units=2`, high-res BLE HID, key mappings, MARK and trackball are unchanged. Only the encoder-specific wheel divider and post-stop coast parameters change.

**Important tradeoff:** At negotiated 16x HID resolution, v3.3 baseline `2/3` unit per sensor event used `0,1,1` over each 3 events. v3.4 `2/4` uses **`0,1,0,1`**, so there are more zero-wheel events. Low-speed stepping might become more perceptible even though previous v3.2 was smooth to the user. We cannot emit a half-HID-unit for every physical event. A 25% reduction in wheel units does **not guarantee** 25% slower pixel motion under macOS mouse acceleration.

**Second caveat:** macOS Mouse scroll-speed adjustment is not device-specific and may affect other mice / the trackball's scroll action. We changed only the encoder-specific wheel path; the global HID multiplier and trackball scroll code are untouched. Use the macOS slider as a fine-tuning control *after* testing the firmware at the same initial setting.

## Hardware A/B checklist

1. Ensure GitHub Actions **left, right, merged firmware** jobs all succeeded.
2. Download this branch's `firmware` ZIP, flash matching **left** and **right** UF2. **Do not flash `settings_reset`**. No new Bluetooth HID descriptor is introduced, so re-pairing is not expected for this change.
3. Keep macOS Mouse scroll speed at the same minimum position initially; compare **v3.3** and **v3.4** in the same long Safari/Finder window, slow turns, step-by-step detents, and reverse.
4. If v3.4 is sufficiently slow, try raising the Mac slider one notch at a time until the desired speed is reached; note how much scroll-speed adjustment range is now available.
5. Test moderate-speed turns: the new fast-only coast should rarely trigger. Spin **very fast** through 8+ consecutive encoder events, release, and observe whether a small post-stop coast occurs.
6. Confirm no surprise motion after pausing and reversing. Also verify trackball, MARK symbols, numeric layer, volume/brightness layer mappings and BLE connections.
7. **Rollback to v3.3:** flash the [successful v3.3 build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38352497400). **Rollback to inertia-OFF**: [v3.2-no-inertia](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38351612424).
8. If the faster/medium rotation still triggers the glide too often even after this change, add event-timestamp instrumentation and compare consecutive intervals. Do not assume host-only acceleration or bounce without measurement.

This is still a draft experiment. No merge until the user verifies the new feel.
