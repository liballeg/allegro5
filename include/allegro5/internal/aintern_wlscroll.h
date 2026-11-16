#ifndef __al_included_allegro5_aintern_wlscroll_h
#define __al_included_allegro5_aintern_wlscroll_h

#include <stdbool.h>
#include <stdint.h>

/* Reconcile the representations of one axis in a wl_pointer.frame. The
 * discrete/value120 events describe the same motion as the axis event. */
typedef struct ALLEGRO_WL_SCROLL_AXIS {
    double continuous;
    int64_t discrete;
    int64_t value120;
    bool have_discrete;
    bool have_value120;
    double continuous_remainder;
    int64_t value120_remainder;
} ALLEGRO_WL_SCROLL_AXIS;

static inline int _al_wl_scroll_flush(ALLEGRO_WL_SCROLL_AXIS *axis,
    int precision)
{
    int delta;
    if (axis->have_value120) {
        int64_t value = axis->value120 * precision + axis->value120_remainder;
        delta = (int)(value / 120);
        axis->value120_remainder = value - (int64_t)delta * 120;
    }
    else if (axis->have_discrete) {
        delta = (int)(axis->discrete * precision);
    }
    else {
        double value = axis->continuous * precision / 10.0 +
            axis->continuous_remainder;
        delta = (int)value;
        axis->continuous_remainder = value - delta;
    }
    axis->continuous = 0;
    axis->discrete = 0;
    axis->value120 = 0;
    axis->have_discrete = false;
    axis->have_value120 = false;
    return delta;
}

#endif
