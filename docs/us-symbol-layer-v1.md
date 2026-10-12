# US-oriented symbol layer v1 (experiment)

This branch is an **experimental draft**, not the live firmware.

## Goal
Make LiNEA40 symbol entry easier to remember for an Apple US-keyboard user, without changing base typing, trackball, combos, encoder, or other layers.

## MARK layer (hold Z or Enter on the base layer)

Keys in each row map left hand (Q W E R T / A S D F G / Z X C V B) then right hand (Y U I O P / H J K L - / N M , . Backspace).

```text
!  @  #  $  %      ^  &  *  (  )
1  2  3  4  5      6  7  8  9  0
[  ]  {  }  =      +  ;  :  '  "
```

- TOP: same sequence as Shift+1 through Shift+0 on a US keyboard.
- MIDDLE: digits 1 through 0 in matching columns. This is an optional convenience in addition to the existing CURSOR numpad.
- LOWER: punctuation arranged in pairs, with direct `{}` `:` `"` and `+` outputs.
- The 11 thumb/bottom bindings in MARK are deliberately unchanged from main.

## Remains on base layer
- `-` (Shift = `_`)
- `/` (Shift = `?`)
- `\\` (Shift = `|`)
- grave accent (Shift = `~`)
- comma and dot (Shift = `<` and `>`)
- right bracket (Shift = `}`)

## Validation checklist before flashing
- Confirm GitHub Actions succeeds for both LiNEA40 halves.
- Back up the existing main build; this branch does not change main.
- Verify macOS uses the US keyboard input layout.
- Test `!@#$%^&*()`, `1234567890`, `[]{}=+;:'"`.
- Check that holding Z / Enter still triggers MARK and tapping either still emits Z / Enter.
- Check typing, trackball motion/clicks, scroll, arrow and Bluetooth layers.
- If any unintentional key behavior occurs, flash the previous firmware from main.

## Next iteration candidates
- If numbers on MARK feel redundant, reclaim the middle row for punctuation or macros.
- If same-hand symbol chords feel awkward, consider moving the MARK hold trigger to a thumb key (separate experiment).
- Reconsider position of `;`, `'` and brackets based on actual typing mistakes.
