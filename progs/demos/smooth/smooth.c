/*
 * Original copyright notice from smooth.c:
 *
 * License Applicability. Except to the extent portions of this file are
 * made subject to an alternative license as permitted in the SGI Free
 * Software License B, Version 1.1 (the "License"), the contents of this
 * file are subject only to the provisions of the License. You may not use
 * this file except in compliance with the License. You may obtain a copy
 * of the License at Silicon Graphics, Inc., attn: Legal Services, 1600
 * Amphitheatre Parkway, Mountain View, CA 94043-1351, or at:
 * 
 * http://oss.sgi.com/projects/FreeB
 * 
 * Note that, as provided in the License, the Software is distributed on an
 * "AS IS" basis, with ALL EXPRESS AND IMPLIED WARRANTIES AND CONDITIONS
 * DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED WARRANTIES AND
 * CONDITIONS OF MERCHANTABILITY, SATISFACTORY QUALITY, FITNESS FOR A
 * PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
 * 
 * Original Code. The Original Code is: OpenGL Sample Implementation,
 * Version 1.2.1, released January 26, 2000, developed by Silicon Graphics,
 * Inc. The Original Code is Copyright (c) 1991-2000 Silicon Graphics, Inc.
 * Copyright in any portions created by third parties is as indicated
 * elsewhere herein. All Rights Reserved.
 * 
 * Additional Notice Provisions: The application programming interfaces
 * established by SGI in conjunction with the Original Code are The
 * OpenGL(R) Graphics System: A Specification (Version 1.2.1), released
 * April 1, 1999; The OpenGL(R) Graphics System Utility Library (Version
 * 1.3), released November 4, 1998; and OpenGL(R) Graphics with the X
 * Window System(R) (Version 1.3), released October 19, 1998. This software
 * was created using the OpenGL(R) version 1.2.1 Sample Implementation
 * published by SGI, but has not been independently verified as being
 * compliant with the OpenGL(R) version 1.2.1 Specification.
 *
 */

#include "GL/freeglut_std.h"
#include <GL/freeglut.h>
#include <OpenGL/OpenGL.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* report GL errors, if any, to stderr */
void checkError(const char *functionName)
{
   GLenum error;
   while (( error = glGetError() ) != GL_NO_ERROR) {
      fprintf (stderr, "GL error 0x%X detected in %s\n", error, functionName);
   }
}

/* vertex array data for a colored 2D triangle, consisting of RGB color values
   and XY coordinates */
const GLfloat varray[] = {
   1.0f, 0.0f, 0.0f, /* red */
   5.0f, 5.0f,       /* lower left */

   0.0f, 1.0f, 0.0f, /* green */
   25.0f, 5.0f,      /* lower right */

   0.0f, 0.0f, 1.0f, /* blue */
   5.0f, 25.0f       /* upper left */
};

/* ISO C somehow enforces this silly use of 'enum' for compile-time constants */
enum {
  numColorComponents = 3,
  numVertexComponents = 2,
  stride = sizeof(GLfloat) * (numColorComponents + numVertexComponents),
  numElements = sizeof(varray) / stride
};

void initRendering(void)
{
   glClearColor (0.0, 0.0, 0.0, 0.0);
   checkError ("initRendering");
}

void initBuffer(void)
{
   glEnableClientState (GL_VERTEX_ARRAY);
   glEnableClientState (GL_COLOR_ARRAY);
   glColorPointer (3, GL_FLOAT, stride, varray);
   glVertexPointer (2, GL_FLOAT, stride, varray+3);
   checkError ("initBuffer");
}

void init(void) 
{
   initBuffer();
   initRendering ();
}

void dumpInfo(void)
{
   printf ("Vendor: %s\n", glGetString (GL_VENDOR));
   printf ("Renderer: %s\n", glGetString (GL_RENDERER));
   printf ("Version: %s\n", glGetString (GL_VERSION));
   printf ("GLSL: %s\n", glGetString (GL_SHADING_LANGUAGE_VERSION));
   checkError ("dumpInfo");
}
float angle = 0.0;

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

const char *tend(struct Timer *self);

const char *tstart(struct Timer *self) {
  const char *msg = NULL;
  if (self->start) {
    msg = tend(self);
  }
  if (self->interval == 0) {
    self->interval = 10e9;
  }
  self->start = clock_gettime_nsec_np(CLOCK_REALTIME);
  if (!self->next_update) {
    self->next_update = self->start + self->interval;
  }

  return msg;
}

const char *tend(struct Timer *self) {
  uint64_t now = clock_gettime_nsec_np(CLOCK_REALTIME);
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
             "Avg: %5.2fms   Min/Max: %5.2f/%5.2fms   STD:%.2fms   CNT: %llu",
             avg, self->min, self->max, sqrt(var), self->count);
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
  timer.interval = update_interval_ns;
  const char *msg = tstart(&timer);
  static char buf[256];

  if (msg) {
    snprintf(buf, sizeof(buf), "%15s -- %s", "Frame Time", msg);
    return buf;
  }
  return NULL;
}

// #import <Cocoa/Cocoa.h>
void displayCB(void) {
  // Make the context current if it exists
  //   if (fgStructure.CurrentWindow &&
  //   fgStructure.CurrentWindow->Window.Context) {
  //     [(NSOpenGLContext *)
  //             fgStructure.CurrentWindow->Window.Context makeCurrentContext];
  //   }

  //   if ([NSOpenGLContext currentContext] == nil) {
  //     printf("No current OpenGL context in displayCB!\n");
  //   } else {
  //   }
  //   printf("displayCB\n");

  const char *fps = get_fps(1e9);
  if (fps) {
    printf("%s\n", fps);
  }

  glClearColor(0.1, 0.2, 0.3, 1.0);
  glClear(GL_COLOR_BUFFER_BIT);

  glLoadIdentity();
  glRotatef(angle, 0, 0, 1);
  angle += 0.1;

  glBegin(GL_TRIANGLES);
  glColor3f(1.0, 0.0, 0.0);
  glVertex2d(0.0, 0.5);
  glColor3f(0.0, 1.0, 0.0);
  glVertex2d(0.5, -0.5);
  glColor3f(0.0, 0.0, 1.0);
  glVertex2d(-0.5, -0.5);
  glEnd();

  glutSwapBuffers();
  //   glutPostRedisplay();
}

void reshape (int w, int h)
{
  printf("reshape\n");
  glViewport(0, 0, (GLsizei)w, (GLsizei)h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glutPostRedisplay();
  checkError("reshape");
}

void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit (0);
         break;
   }
}

#if 1
int main(int argc, char** argv)
{
   glutInit (&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
   glutInitWindowSize (500, 500); 
   glutInitWindowPosition (100, 100);
   glutCreateWindow (argv[0]);
   dumpInfo ();
   // init ();
   glutDisplayFunc(displayCB);
   glutReshapeFunc (reshape);
   glutIdleFunc(displayCB);
   // glutKeyboardFunc (keyboard);
   glutPostRedisplay(); // Trigger an initial redraw

   // while(1);
   glutMainLoop();
   return 0;
}

#else

int main(int argc, char **argv) {
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
  glutInitWindowSize(500, 500);
  glutInitWindowPosition(100, 100);
  int win = glutCreateWindow(argv[0]);

  printf("Vendor: %s\n", glGetString(GL_VENDOR));
  printf("Renderer: %s\n", glGetString(GL_RENDERER));
  printf("Version: %s\n", glGetString(GL_VERSION));
  printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

  glutMainLoop();

  return 0;
}
#endif