# SPDX-License-Identifier: MIT
# Behavior is only used on the central half of the split keyboard.
if(CONFIG_ZMK_LINEA40_ENCODER_INERTIA)
  if((NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    target_sources(app PRIVATE src/behaviors/behavior_inertial_scroll.c)
  endif()
endif()
