#pragma once

#define LEGACY_OPENGL
#ifdef LEGACY_OPENGL
#import <OpenGL/gl.h> // Legacy GL headers
#else
#import <OpenGL/gl3.h> // Core profile GL headers
#endif

#import <Cocoa/Cocoa.h>

#include "utils.h"

static struct Timer ftimer;
static float screen_width  = 2;
static float screen_height = 2;
static float block_size    = 0.1;
static uint64_t frames;
static float cols[][3] = {
    {1.0f, 0.0f, 0.0f},
    {0.0f, 0.0f, 1.0f},
    {0.0f, 1.0f, 1.0f},
    {1.0f, 1.0f, 0.0f},
};
static int col_idx = 0;
static uint64_t blocks_per_screen;

static void drawRotatingTriangle [[maybe_unused]] (void) {
    static float angle = 0.0f;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(angle, 0.0f, 0.0f, 1.0f);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex2f(-0.5f, 0.5f);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex2f(-0.5f, -0.5f);
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex2f(0.5f, -0.5f);
    // glColor3f(0.7f, 0.7f, 0.7f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.5f, 0.5f);
    glEnd();

    // Rotate the triangle
    angle += 1.0f;
}

static uint64_t row(uint64_t frame) {
    return frame / (uint64_t)(screen_width / block_size) % (uint64_t)(screen_height / block_size);
}

static uint64_t col(uint64_t frame) { return frame % (uint64_t)(screen_width / block_size); }

static void square(float size, uint64_t r, uint64_t c) {
    if ((c + r) % 2)
        glColor3fv(&cols[col_idx][0]);
    else
        glColor3fv(&cols[col_idx ^ 1][0]);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(c * block_size, r * block_size, 0);

    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(size, 0);
    glVertex2f(size, size);
    glVertex2f(0, size);
    glEnd();
}

static void drawBlocks(void) {
    static uint64_t last_switch;
    if (!last_switch) {
        last_switch = clock_gettime_nsec_np(CLOCK_REALTIME);
    }
    square(block_size, row(frames), col(frames));

    frames++;

    if (frames % blocks_per_screen == 0) {
        col_idx = (col_idx + 2) % 4;

        uint64_t now = clock_gettime_nsec_np(CLOCK_REALTIME);
        float dur    = (now - last_switch) / 1e6;
        printf("Switching color -- %llu blocks took %.3fms  (avg %.3fms)\n", blocks_per_screen, dur,
               dur / blocks_per_screen);
        last_switch = now;
    }
}

static void block_reshape(int width, int height) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    GLdouble aspect = (GLdouble)width / (GLdouble)height;
    // glOrtho(-aspect, aspect, -1.0, 1.0, -1.0, 1.0);

    NSLog(@"Aspect: %f", aspect);

    glOrtho(0, screen_width, 0, screen_height, -1, 1);

    blocks_per_screen = (uint64_t)(screen_width * screen_height / block_size / block_size);
}

static void block_render(void (^flush)(void)) {
    start_scope(__FUNCTION__);
    blocks_per_screen = (uint64_t)(screen_width * screen_height / block_size / block_size);
    const char *fps   = get_fps(1e9);

    if (fps) {
        NSLog(@"%s", fps);
        EMIT_EVENT("%s", fps);
    }

    drawBlocks();

    // TODO: Add your OpenGL draw calls here ...

    // Present the frame
    uint64_t start = clock_gettime_nsec_np(CLOCK_REALTIME);
    start_scope("FlushBuffer");
    tstart(&ftimer);
    flush();
    const char *msg = tend(&ftimer);
    if (msg)
        NSLog(@"%15s -- %s", "Flush Time", msg);
    EMIT_EVENT("FlushBuffer: %.2f ms", (clock_gettime_nsec_np(CLOCK_REALTIME) - start) / 1e6);
    end_scope("FlushBuffer");
    end_scope(__FUNCTION__);
}