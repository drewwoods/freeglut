#pragma  once
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define RUNTIME_NS(start, end) (end.tv_sec - start.tv_sec) * 1e9 + end.tv_nsec - start.tv_nsec;

struct Timer {
    uint64_t start;
    uint64_t next_update;
    uint64_t count;
    uint64_t interval;
    float sum;
    double sum_of_squares;
    char buf[256];
    float min, max;
};

static const char *tend(struct Timer *self);

uint64_t get_time_ns(clockid_t clock_id) {
    struct timespec ts;
    clock_gettime(clock_id, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static const char *tstart(struct Timer *self) {
    const char *msg = NULL;
    if (self->start) {
        msg = tend(self);
    }
    if (self->interval == 0) {
        self->interval = 10e9;
    }
    self->start = get_time_ns(CLOCK_REALTIME);
    if (!self->next_update) {
        self->next_update = self->start + self->interval;
    }

    return msg;
}

static const char *tend(struct Timer *self) {
  uint64_t now = get_time_ns(CLOCK_REALTIME);
  float dur = (now - self->start) / 1e6;
  self->min = (self->min == 0 || dur < self->min) ? dur : self->min;
  self->max = (dur > self->max) ? dur : self->max;
  self->sum_of_squares += dur * dur;
  self->sum += dur;
  self->count++;

  const char *msg = NULL;

  if (now > self->next_update) {
    float avg = self->sum / self->count;
    float var = self->sum_of_squares / self->count - avg * avg;
    snprintf(self->buf, sizeof(self->buf),
             "Avg: %5.2fms   Min/Max: %5.2f/%5.2fms   STD:%.2fms   CNT: %u",
             avg, self->min, self->max, sqrt(var), (uint32_t)self->count);
    msg = self->buf;
    self->next_update += self->interval;
    self->min = INT_MAX;
    self->max = INT_MIN;
    self->sum_of_squares = 0;
    self->sum = 0;
    self->count = 0;
    }

    self->start = 0;

    return msg;
}

static const char *get_fps(float update_interval_ns) {
    static struct Timer timer;
    timer.interval  = update_interval_ns;
    const char *msg = tstart(&timer);
    static char buf[256];

    if (msg) {
        snprintf(buf, sizeof(buf), "%15s -- %s", "Frame Time", msg);
        return buf;
    }
    return NULL;
}

#ifdef __APPLE__


#import <os/signpost.h>
static os_log_t OSLog;
static os_signpost_id_t spid;

static void init() {
    if (OSLog) {
        return;
    }
    OSLog = os_log_create("blocks-per-frame-trace", OS_LOG_CATEGORY_POINTS_OF_INTEREST);
}

static void start_scope(const char *scope_name) {
    init();

    // Create a signpost ID for this specific operation instance
    spid = os_signpost_id_make_with_pointer(OSLog, scope_name);

    // Begin the interval
    os_signpost_interval_begin(OSLog, spid, "Scope", "Start %s", scope_name);
}

static void end_scope(const char *scope_name) {
    init();

    os_signpost_id_t expected = os_signpost_id_make_with_pointer(OSLog, scope_name);
    // if (!spid) {
    //     fprintf(stderr, "Error: Scope ID not set\n");
    // }
    // if (spid != expected) {
    //     fprintf(stderr, "Error: Scope ID mismatch\n");
    // }

    // end the interval
    os_signpost_interval_end(OSLog, expected, "Scope", "End %s", scope_name);

    spid = 0;
}

#define EMIT_EVENT(...) os_signpost_event_emit(OSLog, spid, "Event:", ##__VA_ARGS__)
#endif