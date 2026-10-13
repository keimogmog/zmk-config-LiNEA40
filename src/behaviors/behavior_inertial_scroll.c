// SPDX-License-Identifier: MIT
/*
 * LiNEA40 encoder-only inertia experiment.
 *
 * v3.3 fast-only coast experiment based on the v3.2 inertia-OFF version.
 * Physical encoder events emit precisely the same high-resolution wheel
 * packets as v3.2, with NO firmware boost while the encoder is turning.
 * Four consecutive short same-direction intervals arm a light glide that
 * starts only after the encoder has stopped for coast-wait-ms.
 * Any new encoder event immediately cancels the glide.
 *
 * No global event handlers or trackball changes.
 */

#define DT_DRV_COMPAT zmk_behavior_inertial_scroll

#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/input/input.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>
#include <zmk/behavior.h>
#include <dt-bindings/zmk/pointing.h>

#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
#include <zmk/pointing/resolution_multipliers.h>
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct inertia_cfg {
    uint16_t frame_ms;
    uint8_t tail_frames;
    uint16_t rapid_threshold_ms;
    uint16_t grouping_threshold_ms;
    int32_t rapid_impulse_q8;
    int32_t grouped_impulse_q8;
    int32_t velocity_cap_q8;
    uint8_t decay_percent;
    uint8_t slow_detent_ticks;
    uint8_t rapid_detent_ticks;
    uint8_t hires_detent_units;
    uint8_t hires_immediate_units;
    uint8_t hires_chunk_units;
    uint8_t encoder_trigger_divisor;
    uint16_t coast_wait_ms;
    uint8_t coast_min_fast_intervals;
};

struct inertia_data {
    const struct device *dev;
    struct k_work_delayable tick_work;
    struct k_spinlock lock;
    int32_t velocity_q8;
    int32_t fractional_q8;
    int64_t last_detent_ms;
    int8_t direction;
    uint8_t frames_remaining;
    // A fraction of one physical detent, delivered over later HID reports.
    int32_t smooth_remaining;
    // Distribute one old scroll event over N smaller encoder events.
    uint8_t detent_unit_remainder;
    // Fast-spin tracking; never feeds back into live encoder wheel distance.
    uint8_t fast_intervals;
    uint16_t last_fast_interval_ms;
    uint8_t endpoint_hires_factor;
    bool coasting;
};

static void inertia_tick(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct inertia_data *data = CONTAINER_OF(delayable, struct inertia_data, tick_work);
    const struct inertia_cfg *cfg = data->dev->config;

    int32_t wheel = 0;
    int32_t next_delay_ms = 0;
    const int64_t now = k_uptime_get();

    k_spinlock_key_t key = k_spin_lock(&data->lock);

    // Fast consecutive detents arm the coast. Do not emit extra wheel
    // units until there has been a real gap with no encoder movement.
    if (!data->coasting &&
        data->fast_intervals >= cfg->coast_min_fast_intervals &&
        data->endpoint_hires_factor > 1) {
        int64_t idle_ms = now - data->last_detent_ms;
        if (idle_ms < cfg->coast_wait_ms) {
            next_delay_ms = cfg->coast_wait_ms - idle_ms;
        } else {
            int32_t speed_q8 =
                cfg->grouped_impulse_q8 +
                (cfg->rapid_threshold_ms - data->last_fast_interval_ms) *
                    cfg->rapid_impulse_q8;

            // On macOS/16x HID these are *fine* wheel units, not
            // traditional whole wheel clicks. Avoid a big fallback tail.
            speed_q8 = CLAMP(speed_q8, 0, cfg->velocity_cap_q8);
            speed_q8 = (speed_q8 * data->endpoint_hires_factor + 8) / 16;

            data->velocity_q8 = data->direction * speed_q8;
            data->fractional_q8 = 0;
            data->frames_remaining = cfg->tail_frames;
            data->fast_intervals = 0;
            data->coasting = data->frames_remaining > 0 &&
                             data->velocity_q8 != 0;
        }
    }

    if (data->coasting) {
        // This small synthetic tail runs only after the fast spin ended.
        data->fractional_q8 += data->velocity_q8;
        wheel = data->fractional_q8 / 256;
        data->fractional_q8 -= wheel * 256;
        data->velocity_q8 = (data->velocity_q8 * cfg->decay_percent) / 100;

        if (--data->frames_remaining == 0 || data->velocity_q8 == 0) {
            data->velocity_q8 = 0;
            data->fractional_q8 = 0;
            data->frames_remaining = 0;
            data->coasting = false;
        } else {
            next_delay_ms = cfg->frame_ms;
        }
    }

    k_spin_unlock(&data->lock, key);

    if (wheel != 0) {
        input_report_rel(data->dev, INPUT_REL_WHEEL, (int16_t)wheel, true, K_NO_WAIT);
    }
    if (next_delay_ms > 0) {
        k_work_reschedule(&data->tick_work, K_MSEC(next_delay_ms));
    }
}

static int inertia_init(const struct device *dev) {
    struct inertia_data *data = dev->data;
    data->dev = dev;
    data->last_detent_ms = -1;
    k_work_init_delayable(&data->tick_work, inertia_tick);
    return 0;
}

static int inertia_pressed(struct zmk_behavior_binding *binding,
                           struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct inertia_data *data = dev->data;
    const struct inertia_cfg *cfg = dev->config;

    // Existing SCRL_UP/SCRL_DOWN ZMK parameters encode signed Y direction.
    int16_t wheel_direction = MOVE_Y_DECODE(binding->param1);
    if (wheel_direction == 0) {
        return 0;
    }
    const int8_t direction = wheel_direction > 0 ? 1 : -1;
    const int64_t now = k_uptime_get();

    // On supported hosts, HID Resolution Multiplier allows fractional
    // wheel steps: e.g. wheel=15 means 16 HID units per normal detent.
    // Without smooth scrolling, keep the proven v1.1 scroll behavior.
    uint8_t hi_res_factor = 1;
#if IS_ENABLED(CONFIG_ZMK_POINTING_SMOOTH_SCROLLING)
    uint8_t wheel_res =
        zmk_pointing_resolution_multipliers_get_current_profile().wheel;
    if (wheel_res > 0 && wheel_res < 16) {
        hi_res_factor = 16 / (16 - wheel_res);
    }
#endif

    bool arm_coast;
    int16_t immediate_ticks;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    int64_t interval = (data->last_detent_ms < 0) ? INT64_MAX
                      : now - data->last_detent_ms;

    bool rapid = interval >= 0 && interval <= cfg->rapid_threshold_ms &&
                 data->direction == direction;

    // Any encoder motion stops the old synthetic tail, including a reverse.
    // Never change physical scroll distance based on recent speed.
    data->velocity_q8 = 0;
    data->fractional_q8 = 0;
    data->frames_remaining = 0;
    data->coasting = false;
    data->smooth_remaining = 0;

    if (data->direction != direction) {
        data->detent_unit_remainder = 0;
    }
    if (rapid) {
        if (data->fast_intervals < UINT8_MAX) {
            data->fast_intervals++;
        }
        data->last_fast_interval_ms = interval;
    } else {
        data->fast_intervals = 0;
        data->last_fast_interval_ms = 0;
    }
    data->endpoint_hires_factor = hi_res_factor;

    // Each sensor event is now 1/3 of its previous angular travel.
    // Preserve the v3.2 physical wheel distance exactly (at 16x: 0,1,1).
    // The fast coast is separate and never accelerates in-flight detents.
    int32_t units_per_old_event = (hi_res_factor > 1)
        ? (cfg->hires_detent_units * hi_res_factor) / 16
        : (rapid ? cfg->rapid_detent_ticks : cfg->slow_detent_ticks);
    int32_t credits = units_per_old_event + data->detent_unit_remainder;
    int32_t total = credits / cfg->encoder_trigger_divisor;
    data->detent_unit_remainder = credits % cfg->encoder_trigger_divisor;

    if (hi_res_factor > 1) {
        // v3.2 physical HID output: same fine step and remainder as before.
        int32_t first = MIN(total, (cfg->hires_immediate_units * hi_res_factor) / 16);
        immediate_ticks = direction * first;
        data->smooth_remaining += direction * (total - first);
    } else {
        // Non-hires host: a coarser fallback. Avoid tripling scroll distance.
        immediate_ticks = direction * total;
    }

    data->last_detent_ms = now;
    data->direction = direction;
    arm_coast = data->fast_intervals >= cfg->coast_min_fast_intervals &&
                hi_res_factor > 1 && cfg->tail_frames > 0;
    k_spin_unlock(&data->lock, key);

    // Retain physical v3.2 wheel output, completely free of acceleration.
    if (immediate_ticks != 0) {
        input_report_rel(dev, INPUT_REL_WHEEL, immediate_ticks, true, K_NO_WAIT);
    }

    // A fast streak restarts the idle timer; ordinary turns cancel it.
    if (arm_coast) {
        k_work_reschedule(&data->tick_work, K_MSEC(cfg->coast_wait_ms));
    } else {
        k_work_cancel_delayable(&data->tick_work);
    }
    return 0;
}

static int inertia_released(struct zmk_behavior_binding *binding,
                            struct zmk_behavior_binding_event event) {
    // Only press is an impulse; release must not stop an ongoing tail.
    return 0;
}

static const struct behavior_driver_api inertia_api = {
    .binding_pressed = inertia_pressed,
    .binding_released = inertia_released,
};

#define INERTIA_INST(n)                                                                \
    static struct inertia_data inertia_data_##n;                                       \
    static const struct inertia_cfg inertia_cfg_##n = {                               \
        .frame_ms = DT_INST_PROP(n, frame_ms),                                         \
        .tail_frames = DT_INST_PROP(n, tail_frames),                                   \
        .rapid_threshold_ms = DT_INST_PROP(n, rapid_threshold_ms),                     \
        .grouping_threshold_ms = DT_INST_PROP(n, grouping_threshold_ms),               \
        .rapid_impulse_q8 = DT_INST_PROP(n, rapid_impulse_q8),                         \
        .grouped_impulse_q8 = DT_INST_PROP(n, grouped_impulse_q8),                     \
        .velocity_cap_q8 = DT_INST_PROP(n, velocity_cap_q8),                           \
        .decay_percent = DT_INST_PROP(n, decay_percent),                               \
        .slow_detent_ticks = DT_INST_PROP(n, slow_detent_ticks),                       \
        .rapid_detent_ticks = DT_INST_PROP(n, rapid_detent_ticks),                     \
        .hires_detent_units = DT_INST_PROP(n, hires_detent_units),                     \
        .hires_immediate_units = DT_INST_PROP(n, hires_immediate_units),               \
        .hires_chunk_units = DT_INST_PROP(n, hires_chunk_units),                       \
        .encoder_trigger_divisor = DT_INST_PROP(n, encoder_trigger_divisor),           \
        .coast_wait_ms = DT_INST_PROP(n, coast_wait_ms),                               \
        .coast_min_fast_intervals = DT_INST_PROP(n, coast_min_fast_intervals),         \
    };                                                                                 \
    BEHAVIOR_DT_INST_DEFINE(n, inertia_init, NULL, &inertia_data_##n,                  \
                           &inertia_cfg_##n, POST_KERNEL,                             \
                           CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &inertia_api);

DT_INST_FOREACH_STATUS_OKAY(INERTIA_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
