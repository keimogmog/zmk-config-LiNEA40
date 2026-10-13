// SPDX-License-Identifier: MIT
/*
 * LiNEA40 encoder-only inertia experiment.
 *
 * Each encoder detent produces an immediate, precise wheel tick.
 * Repeated same-direction detents within a short window add a small
 * exponentially decaying scroll tail. A reverse detent clears the old tail.
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
};

static void inertia_tick(struct k_work *work) {
    struct k_work_delayable *delayable = k_work_delayable_from_work(work);
    struct inertia_data *data = CONTAINER_OF(delayable, struct inertia_data, tick_work);
    const struct inertia_cfg *cfg = data->dev->config;

    int32_t wheel = 0;
    bool continue_tail = false;

    k_spinlock_key_t key = k_spin_lock(&data->lock);

    if (data->frames_remaining > 0 && data->velocity_q8 != 0) {
        data->fractional_q8 += data->velocity_q8;
        wheel = data->fractional_q8 / 256;
        data->fractional_q8 -= wheel * 256;

        data->velocity_q8 = (data->velocity_q8 * cfg->decay_percent) / 100;
        data->frames_remaining--;

        if (data->frames_remaining == 0 ||
            (data->velocity_q8 > -5 && data->velocity_q8 < 5)) {
            data->velocity_q8 = 0;
            data->fractional_q8 = 0;
            data->frames_remaining = 0;
        } else {
            continue_tail = true;
        }
    }

    k_spin_unlock(&data->lock, key);

    if (wheel != 0) {
        input_report_rel(data->dev, INPUT_REL_WHEEL, (int16_t)wheel, true, K_NO_WAIT);
    }
    if (continue_tail) {
        k_work_reschedule(&data->tick_work, K_MSEC(cfg->frame_ms));
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

    bool has_tail;
    int16_t immediate_ticks;
    k_spinlock_key_t key = k_spin_lock(&data->lock);
    int64_t interval = (data->last_detent_ms < 0) ? INT64_MAX
                      : now - data->last_detent_ms;

    // A slow detent must be prominent enough for macOS to move immediately.
    // Keep the existing fast-spin response and tail unchanged.
    bool rapid = interval >= 0 && interval <= cfg->rapid_threshold_ms &&
                 data->direction == direction;
    immediate_ticks = direction * (rapid ? cfg->rapid_detent_ticks
                                         : cfg->slow_detent_ticks);

    if (interval < 0 || interval > cfg->grouping_threshold_ms ||
        data->direction != direction) {
        // One slow detent should be exact; reverse clears old momentum.
        data->velocity_q8 = 0;
        data->fractional_q8 = 0;
        data->frames_remaining = 0;
    } else {
        int32_t impulse = interval <= cfg->rapid_threshold_ms
                          ? cfg->rapid_impulse_q8
                          : cfg->grouped_impulse_q8;
        data->velocity_q8 = CLAMP(data->velocity_q8 + direction * impulse,
                                  -cfg->velocity_cap_q8, cfg->velocity_cap_q8);
        data->frames_remaining = cfg->tail_frames;
    }

    data->last_detent_ms = now;
    data->direction = direction;
    has_tail = data->frames_remaining != 0;
    k_spin_unlock(&data->lock, key);

    // Larger immediate step for slow/isolated detents; rapid spinning retains v1 rate.
    input_report_rel(dev, INPUT_REL_WHEEL, immediate_ticks, true, K_NO_WAIT);

    if (has_tail) {
        k_work_reschedule(&data->tick_work, K_MSEC(cfg->frame_ms));
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
    };                                                                                 \
    BEHAVIOR_DT_INST_DEFINE(n, inertia_init, NULL, &inertia_data_##n,                  \
                           &inertia_cfg_##n, POST_KERNEL,                             \
                           CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &inertia_api);

DT_INST_FOREACH_STATUS_OKAY(INERTIA_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY */
