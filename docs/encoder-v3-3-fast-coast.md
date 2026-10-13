# LiNEA40 v3.3 — fast-spin coast only (experiment)

## Motivation and preserved behavior

The user tested [v3.2 inertia OFF](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38351612424) and felt normal scrolling was good without firmware inertia, **but wanted a little coasting when turning the encoder fast**. Earlier versions accelerated during the physical rotation and felt unnatural.

This experiment is based on the confirmed v3.2 no-inertia branch. The physical encoder HID output is **identical** to that version while actively rotating. No extra scrolling, acceleration or velocity compensation occurs before an idle gap.

Existing v3, v3.1, v3.2, v3.2 inertia-OFF, MARK v2, all key mappings, both encoder sensor bindings and trackball code are left untouched. This is a **separate branch / draft PR**, not merged.

## How fast-only coasting works

- Keep **24 encoder behavior events per revolution**, ZMK v0.3.0 high-resolution HID, `hires-detent-units=2`, `encoder-trigger-divisor=3` (on 16× HID the existing `0,1,1` scroll pattern).
- Track only intervals `<=30ms` between consecutive **same-direction** encoder events.
- Require **4 fast intervals (5 consecutive fast events)** to arm a coast. Slow movement, brief rotations and reversals never arm it.
- During rapid rotation, **no extra synthetic HID wheel values** are sent. New events reset the idle timer instead.
- When there is **55ms of silence** following a qualifying fast run, start a *small* velocity-based decay tail. The initial tail rate is based on the most recent rapid interval. Limited to 450/256 fine wheel units per 16ms frame at full 16× resolution.
- Send up to **9 frames at 16ms intervals** (roughly 144ms total), with 78% velocity retention each frame. Some frames can emit no integer wheel units when the accumulated fraction is below one.
- **Any new event immediately cancels** a queued or active synthetic tail and retains the exact v3.2 physical scroll event behavior. A direction reversal also resets fractional physical wheel remainder.
- If high-res HID multiplier is reported as 1×, coast remains disabled to avoid emitting full-sized wheel steps unexpectedly. On other negotiated factors, the synthetic rate scales to that factor.

## Tunable settings

`config/boards/shields/LiNEA40/LiNEA40.dtsi`:

| Property | v3.3 value | Effect |
| --- | --- | --- |
| `rapid-threshold-ms` | 30 | Maximum gap counted as a fast interval |
| `coast-min-fast-intervals` | 4 | Number of rapid same-direction intervals to arm |
| `coast-wait-ms` | 55 | Quiet time before coast starts |
| `tail-frames` | 9 | Maximum deceleration output frames |
| `frame-ms` | 16 | Time between coast frames |
| `rapid-impulse-q8` | 16 | Initial velocity slope (based on gap) |
| `grouped-impulse-q8` | 140 | Initial velocity base |
| `velocity-cap-q8` | 450 | Maximum initial velocity before HID-scale adjustment |
| `decay-percent` | 78 | Velocity retention per frame |

The `coast-min-fast-intervals` and `coast-wait-ms` settings are new to this experimental branch's Devicetree binding. Other names are reused by the new implementation, which applies them only to the post-stop synthetic tail (no accelerative events during motion).

## Validation plan and caveats

1. Confirm GitHub Actions reports **SUCCESS** for left, right, and merged `firmware` artifact. Flash matching left + right UF2; **do not use settings_reset**.
2. Test gentle one-notch moves, sustained slow spin, immediate reversal: these should feel exactly like [v3.2 inertia OFF](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38351612424).
3. Try a moderate spin that does not reach 4 consecutive fast intervals: no synthetic glide expected.
4. Spin fast enough to activate detection and abruptly stop; a small tail should start **after** 55ms without wheel input, rather than adding speed while still turning.
5. Test short pause / resume and reversing direction during the glide. Compare in Safari and Finder without changing Mac scroll speed.
6. Regression-test trackball SCROLL, volume/brightness encoder layers and Bluetooth profile switching. The v3 split behavior caveat remains: `triggers-per-rotation=24` globally increases sensor frequency on non-scroll layers.
7. Host macOS may independently apply wheel acceleration or render animation; that is separate from ZMK's added post-stop tail.
8. For rollback, flash both halves from the [v3.2 inertia OFF successful build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38351612424) or from [v3.2 with original inertia](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38349933498).

**Known tradeoff**: An intentional 55ms detection pause is necessary to distinguish "still turning" from "stopped" without sensing physical finger contact. This may feel like a tiny pause before the glide. Mac-side wheel acceleration could still affect feel. The actual thresholds and step count are hypotheses pending hardware testing.

Do not merge into main before the user's A/B feedback.
