# LiNEA40 US symbol MARK layer — v2 proposal

An experimental alternative to v1: **No numbers anywhere in MARK**. The existing CURSOR and SCROLL numeric/numpad bindings remain unchanged.

## Layout

Left five keys, right five keys:

```
!  @  #  $  %     ^  &  *  (  )
[  ]  {  }  =     +  ;  :  '  "
-  _  \\  |  /     ?  `  ~  <  >
```

- Top: familiar US keyboard shifted-number symbols in order.
- Middle: paired bracket types, equals/plus, semicolon/colon, single/double quotes.
- Bottom: minus/underscore, backslash/pipe, slash/question, backtick/tilde, angle brackets.
- The existing 11-key thumb/bottom section of MARK remains unchanged; so do the default layer, trackball, combos, scroll, cursor, function, wireless, and other mappings.
- All layer changes are limited to the 30 main keys inside MARK. No changes to main.

## Key codes

Home row uses ZMK `LBKT`, `RIGHT_BRACKET`, and shifted forms; ZMK's key codes are interpreted by the active macOS input source (assumed US). Bottom row `<` and `>` are Shift+Comma and Shift+Dot, not dedicated hardware keys. Both keys already work from base + Shift, but have been placed together here for memorability.

## Test / rollback

1. Confirm firmware builds for *both halves* using GitHub Actions before flashing.
2. Save main branch build artifacts beforehand.
3. Verify macOS keyboard input source is US.
4. Check the top row (`!@#$%^&*()`), middle row (`[]{}=+;:'"`), bottom row (minus, underscore, backslash, pipe, slash, question, grave, tilde, angle brackets).
5. Check numeric layer and trackball remain unchanged.
6. Roll back to main firmware if any regression.

This is a design experiment, not an assertion that the physical keyboard already runs this keymap.
