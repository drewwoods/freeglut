/*
 * fg_glut_shim_emscripten.c
 *
 * GLUT entry points Emscripten's JS GLUT (library_glut.js) does not
 * implement, supplied so that ordinary GLUT programs link.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * On Emscripten the JS GLUT owns the window and the event loop:
 * include/GL/emscripten_hide_glut.h renames freeglut's own windowing entry
 * points to fg_glut*, so the JS implementations win at link and freeglut
 * supplies only geometry, fonts and the teapot.  That leaves the handful of
 * GLUT entry points the JS side never implemented undefined.
 *
 * This TU fills those gaps under their PUBLIC names, so it undoes the hide
 * header for exactly the names it touches.  The bodies are written against
 * the JS loop's real mechanics -- they are deliberately NOT forwards to the
 * freeglut implementations, whose state (fgState.ExecState, the window list)
 * drives nothing here.
 */

#undef glutLeaveMainLoop
#undef glutDisplayFunc
#undef glutIdleFunc

extern void glutDisplayFunc( void (*callback)( void ) );
extern void glutIdleFunc( void (*callback)( void ) );

/*
 * There is no loop flag to clear.  JS glutMainLoop() unwinds the C stack and
 * returns to the browser; what keeps running afterwards is a one-shot
 * requestAnimationFrame chain that each glutPostRedisplay() re-arms, plus a
 * setTimeout-driven idle callback.  Leaving the loop therefore means dropping
 * the callbacks that re-arm it: with no display callback installed,
 * glutPostRedisplay() becomes a no-op and the chain ends.
 *
 * Two differences from native freeglut, both unavoidable here:
 *   - main() does not resume.  Its stack was discarded by glutMainLoop().
 *   - a glutTimerFunc() already scheduled still fires (JS GLUT hands it
 *     straight to setTimeout and keeps no handle to cancel).  It can no
 *     longer cause a redisplay, which is what stops the animation.
 */
void glutLeaveMainLoop( void )
{
    glutDisplayFunc( 0 );
    glutIdleFunc( 0 );
}
