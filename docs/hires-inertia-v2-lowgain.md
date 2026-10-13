# LiNEA40 fine-scroll low-gain experiment (v2-lowgain)

**Purpose:** Direct response to user feedback that [high-resolution + inertia v2](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38328938061) moves far too much after the slightest touch. This is a diagnostic firmware experiment, **not a verified fix**. Preserve original v2, v1.1 and main.

## Root-cause hypothesis

The v2 firmware emits **16 HID wheel units per physical detent** (6 immediately + 5 + 5), while macOS might not interpret those units as 1/16th wheel detents on this BLE device. A quick series of turns adds inertia at 16x units. If the host ignores the wheel-resolution multiplier, this feels like an enormous movement.

The actual host HID resolution negotiation is *not confirmed*. A successful ZMK build does **not** prove macOS is interpreting the feature.

## v2-lowgain adjustments

| Parameter | v2 | lowgain |
|---|---:|---:|
| Fine HID wheel units / physical detent | 16 | **4** |
| Immediate wheel units | 6 | **2** |
| Further units per 16ms frame | 5 | **1** |
| Remainder frames | 2 | **2** |
| Quick-rotation threshold | 90ms | **75ms** |
| Momentum grouping interval | 155ms | **90ms** |
| Fast inertia impulse q8 | 120 | **35** |
| Slower-group impulse q8 | 65 | **15** |
| Velocity cap q8 | 400 | **120** |
| Inertia decay | 76%/frame | unchanged |
| Maximum tail frames | 11 | unchanged (176ms) |

The C code also now scales inertial impulse with the actual per-detent movement instead of always multiplying by the full HID 16x factor. This is an intentional **reduced-sensitivity**, not a calibrated Mac precision-scrolling implementation.

Still ZMK v0.3.0, with `CONFIG_ZMK_POINTING_SMOOTH_SCROLLING=y` only on the right/central half. MARK v2, number layer, and trackball processing code remain untouched, but the global HID resolution descriptor may affect trackball scrolling.

## How to test

1. Download only after CI confirms left, right and artifact merge SUCCESS. Use the **matching left/right UF2 from this branch's run**.
2. Slow single detent; a couple of isolated detents; then turn slowly over several detents. Check whether each notch moves predictably without jumping.
3. Spin quickly: note whether inertia still appears and if it becomes too weak.
4. Immediately reverse direction while momentum remains.
5. Test Mac apps (Safari/Finder) and trackball SCROLL layer separately (global HID resolution may affect it).
6. Compare with original [hires v2](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38328938061) and [non-hires v1.1](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38326525728).
7. For HID descriptor mismatches, macOS may cache the Bluetooth HID descriptor until you forget/re-pair the keyboard. **Do not clear/reset the entire keyboard**; ensure a backup keyboard is available first.

If this still jumps wildly, stop adjusting inertia and instrument/verify BLE HID resolution-multiplier handling before claiming smooth-scroll support. No main-branch changes.
