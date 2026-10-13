# LiNEA40 high-resolution scroll + inertia — experimental v2

**Not released. Test after CI; do not merge into main.** Built on `experiment/encoder-inertia-v1` (the user's experimental v1.1). The familiar US MARK v2 and number layer are unchanged.

## Why

User said encoder inertia v1/v1.1 is somewhat nice, but slow scrolling feels unresponsive / not quite right. Sending `1` or `2` whole HID wheel ticks is quantized and feels stepped. The target is responsive low-speed scrolling *and* natural fast spin and slowdown, with no macOS helper application.

## Dependencies and known BLE problem

- ZMK `v0.2.1` has a **known smooth-scroll over BLE bug** (upstream issue [#2957](https://github.com/zmkfirmware/zmk/issues/2957), fixed by upstream PR #2998 in ZMK `v0.3.0`).
- This experiment pins ZMK `v0.3.0` in `config/west.yml`. This changes more than scrolling; verify other keyboard functions on the actual hardware.
- The right/central shield alone enables `CONFIG_ZMK_POINTING_SMOOTH_SCROLLING=y` (HID Resolution Multiplier feature report). A host that negotiates 16x resolution can interpret 16 small wheel units as one normal detent. **Actual macOS host negotiation has NOT been verified**.
- Bluetooth hosts cache HID descriptors. You may need to forget the LiNEA40 device on macOS and clear **only that Bluetooth profile** on the keyboard before pairing again (do not perform a full settings reset casually). This changes the descriptor globally for the BLE mouse, not only the encoder.

## Proposed encoder motion

When the endpoint's reported scroll multiplier is >1 (16x on a fully high-resolution HID link):

- Each detent outputs 16 small wheel units instead of a single large tick.
- First 6 units are emitted immediately; remaining 10 units are sent as +5 and +5 after 16ms and 32ms. Direction signs are preserved.
- On rapid repeated same-direction detents, inertia velocity and caps scale with the resolution multiplier (so momentum distance stays about comparable to the prior v1.1 design).
- Spin-down decay stays `76% per 16ms`, up to `11 frames = 176ms`.
- Reversing direction cancels pending fine-scroll units and tail impulse; a new immediate pulse goes in the reversed direction.
- Existing non-scroll encoder layer bindings unchanged.

When the selected endpoint reports wheel multiplier 1x, preserve the v1.1 approach (slow isolated tick=2, quick tick=1, existing momentum units). At ZMK v0.3.0 the multiplier defaults to 16x, so a host which ignores the HID feature may still behave poorly; on-host testing is essential.

## Important limitations

- This is ordinary mouse HID wheel scrolling with finer time/size steps, **not real Apple trackpad gesture events**. Some apps may still render steps; final feel depends on macOS HID processing and BLE connection/report interval.
- This global HID descriptor modification **can affect trackball SCROLL-layer scroll distance** even though the trackball code is not changed. Check trackball scroll before accepting any result; it may need a compensating input processor.
- The EC11 encoder is detented. We can interpolate movement between detents, but cannot read continuous wheel position or generate new physical angular resolution.
- The work is an untested experimental build, not a recommended upgrade.

## A/B steps

1. Confirm GitHub Actions **left, right, settings_reset and merge = success**, then download the firmware artifact for this v2 branch.
2. Save/retain the known-good v1.1 artifacts: [Actions run 38326525728](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38326525728). v1 and MARK v2 are also available.
3. Write the left and right matching UF2 files. `settings_reset` is unnecessary.
4. Test slow single detents (immediate response and physical travel), mid-speed scrolling, rapid spin, stopping, and reversal in Finder, Safari and another application.
5. Test trackball pointer and SCROLL layer, volume/brightness encoder bindings, MARK v2, Bluetooth pairing and switching.
6. If smooth HID is not recognized by macOS, *refresh the BLE HID descriptor*: remove the keyboard in macOS Bluetooth, clear only that active LiNEA40 BLE profile using its existing WIRELESS layer, then pair again. Warn that doing this disconnects the keyboard; have another keyboard ready. If you do not want to risk losing pairing, test USB first.
7. If smoothness or Bluetooth becomes worse, flash the v1.1 successful UF2 pair and, if macOS still keeps the old HID descriptor, re-pair again.

No claim of macOS support or tested feel until the user's actual keyboard has been checked.
