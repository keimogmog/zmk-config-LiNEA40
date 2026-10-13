# LiNEA40 high-resolution wheel v3.1 — 25% slower

## User feedback

After hardware testing [v3](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38348608575), user reported that the smoothness was **very good**, but the total scrolling distance/speed was slightly too high.

This is a **single-parameter A/B test** of perceived scroll travel; it is not yet an approved merge. The original v3 remains unchanged.

## Difference from v3

In `config/boards/shields/LiNEA40/LiNEA40.dtsi`:

- `hires-detent-units = <4>` → `<3>` (**25% less wheel travel per full encoder revolution**, for a negotiated 16x HID-resolution host).
- `steps=<24>`, `triggers-per-rotation=<24>` **unchanged**; physical event cadence remains fine-grained.
- At 16x wheel resolution, the v3 software previously distributed 4 units over three encoder events as **1,1,2**. v3.1 distributes 3 as **1,1,1** — ideally more even at slow scroll.
- Inertia impulse scales to 3 units rather than 4 automatically through the existing `momentum_gain` formula. Frame interval, decay, thresholds, velocity cap config unchanged.
- MARK v2 symbols, numeric layer, trackball hardware/processing and ZMK v0.3.0 unchanged.
- **Known caveat carried forward from v3:** the global encoder trigger rate 24/rotation also affects non-scroll sensor bindings (volume, brightness, arrow and other layers). These may activate more frequently; must isolate rate compensation before merging to main.
- HID resolution negotiation and host rendering depend on macOS and Bluetooth. The claimed 25% is a firmware wheel-unit reduction, not a guaranteed 25% on-screen speed change.

## Validation

1. Confirm **both halves** and artifact merge succeed in this branch's GitHub Actions.
2. Download the new `firmware` ZIP and flash the matching `left` and `right` UF2; never flash `settings_reset` for ordinary tuning.
3. In Safari/Finder, compare single-notch movement, very slow scrolling, fast rotation and inertia decay.
4. Also test reversed scrolling, MARK v2 and trackball scroll + volume/brightness changes.
5. Roll back by flashing the [known-good v3 build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38348608575), or [v2-lowgain](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38332461574) for a deeper rollback.

Don't merge to main without hardware confirmation.
