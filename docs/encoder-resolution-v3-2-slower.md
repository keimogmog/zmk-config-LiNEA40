# Encoder v3.2: slower travel with an explicit smoothness tradeoff

Experimental comparison based on v3.1 commit
`ba4a2588f98d9eef5db36f915fd5c24e3cea7cb4` (PR #6).
Branch: `experiment/encoder-resolution-v3-2-slower`.
Do not merge without hardware feedback. Existing v3, v3.1 and main are preserved.

## Only firmware change

`hires-detent-units` changes from 3 to 2 in `LiNEA40.dtsi`.
The encoder still uses 24 triggers/rotation and divisor 3. MARK, keymaps,
trackball, other sensor bindings, HID descriptors, dependencies, scheduler,
and inertia configuration are unchanged.

At negotiated 16x resolution, excluding inertia:

| Version | Units per three events | Sequence from zero remainder | Units per 24 events |
| --- | --- | --- | --- |
| v3 | 4 | 1,1,2 | 32 |
| v3.1 | 3 | 1,1,1 | 24 |
| v3.2 | 2 | 0,1,1 | 16 |

This reduces base wheel travel by one third relative to v3.1, and by half
relative to v3. It is not a guarantee of an equal on-screen speed reduction.
The existing momentum formula also scales the 16x inertia gain from 3 to 2;
integer rounding means total travel including inertia need not be exactly 2/3.

## Why smoothness cannot be promised

At 16x, v3.1 already sends the smallest nonzero integer HID wheel value at
every event. Keeping 24 events, the same HID resolution and an immediate
positive output for every same-direction event requires at least 24 units
per rotation. Sending only 16 necessarily leaves some events without output.
Timers or fractional accumulators cannot create a sub-unit HID wheel report.

The existing remainder accumulator gives 0,1,1 repeatedly on a slow continuous
turn. It preserves the remainder across same-direction pauses, but clears it
on reversal. Thus the first event after startup or each reversal emits zero;
alternating single events may produce no movement at all. Slow scrolling can
feel stepped again. This is an intentional A/B experiment, not a claim of
equally smooth, universally slower scrolling.

Reducing inertia alone could preserve v3.1's slow-event cadence, but would
not slow isolated events (more than 45ms apart), which have no inertia.
Changing the shared HID resolution or host settings would expand this test's
scope and may affect trackball scrolling, so neither is changed here.

With lower negotiated high-resolution factors, integer rounding is coarser:
at 8x the base sequence is 0,0,1 in both v3.1 and v3.2; at 2x/4x both
produce zero base units. The non-high-resolution fallback is unchanged.
No improvement across every host or resolution is claimed.
The existing 24-trigger sensitivity of volume/brightness and other encoder
layers is inherited from v3.1, not changed by this comparison.

## Build and hardware validation

Confirm both left/right build jobs and Merge Output Artifacts succeed at the
v3.2 commit before flashing. Build success does not validate physical feel.
Download that run's `firmware` artifact, unzip it, and flash only the matching
LiNEA40_left UF2 to the left half and LiNEA40_right UF2 to the right half using
the same bootloader procedure that worked for v3.1. Keep a local copy of the
v3.1 ZIP before starting. **Do not flash the bundled settings_reset UF2**;
this parameter-only experiment does not require clearing settings or pairing.

Compare in the same app and connection mode:

1. Very slow continuous rotation and isolated events: look for skipped output.
2. Immediate reversals and alternating single events: check the dead first event.
3. Medium/fast spins and stopping: compare total travel and inertia.
4. Check MARK, trackball pointer/scroll and other encoder layers as regressions.

If the stepping or reversal response is worse, reflash BOTH matching halves
from the known-good v3.1 artifact. No branch merge is needed to try or revert.

- Preferred rollback: [v3.1 successful build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38349101220)
- Earlier rollback: [v3 successful build](https://github.com/keimogmog/zmk-config-LiNEA40/actions/runs/38348608575)

Both rollback artifacts were present and unexpired when this comparison was
prepared on 2026-10-13. GitHub artifacts expire; retain downloaded copies.
