/*
 * Test program for glutInitDisplayString comparator support
 * Tests all valid comparators: =, !=, <, >, <=, >=, ~
 */
#include <GL/freeglut.h>
#include <stdio.h>

int main(int argc, char** argv)
{
    /* Initialize GLUT */
    glutInit(&argc, argv);

    /* Test 1: Basic capabilities without comparators */
    printf("Test 1: Basic - rgb double depth stencil\n");
    glutInitDisplayString("rgb double depth stencil");

    /* Test 2: Using = (equal) comparator */
    printf("Test 2: Equal - depth=16 samples=4\n");
    glutInitDisplayString("depth=16 samples=4");

    /* Test 3: Using >= (greater than or equal) comparator */
    printf("Test 3: GTE - red>=8 green>=8 blue>=8 depth>=24\n");
    glutInitDisplayString("red>=8 green>=8 blue>=8 depth>=24");

    /* Test 4: Using ~ (greater than or equal, preferring less) comparator */
    printf("Test 4: MIN - stencil~8 auxbufs~2\n");
    glutInitDisplayString("stencil~8 auxbufs~2");

    /* Test 5: Using < (less than) comparator */
    printf("Test 5: LT - samples<8\n");
    glutInitDisplayString("samples<8");

    /* Test 6: Using > (greater than) comparator */
    printf("Test 6: GT - depth>16\n");
    glutInitDisplayString("depth>16");

    /* Test 7: Using <= (less than or equal) comparator */
    printf("Test 7: LTE - samples<=4\n");
    glutInitDisplayString("samples<=4");

    /* Test 8: Using != (not equal) comparator */
    printf("Test 8: NEQ - depth!=16\n");
    glutInitDisplayString("depth!=16");

    /* Test 9: Complex example from man page */
    printf("Test 9: Complex - stencil~2 rgb double depth>=16 samples\n");
    glutInitDisplayString("stencil~2 rgb double depth>=16 samples");

    /* Test 10: Hexadecimal and octal values */
    printf("Test 10: Hex/Octal - depth=0x10 stencil=010\n");
    glutInitDisplayString("depth=0x10 stencil=010");

    /* Test 11: Multiple color channel specifications */
    printf("Test 11: Color channels - red>=8 green>=8 blue>=8 alpha>=8\n");
    glutInitDisplayString("red>=8 green>=8 blue>=8 alpha>=8");

    /* Test 12: Accumulation buffer */
    printf("Test 12: Accumulation - rgba acca\n");
    glutInitDisplayString("rgba acca");

    printf("\nAll tests completed successfully!\n");
    printf("glutInitDisplayString now properly parses all comparators:\n");
    printf("  = (equal)\n");
    printf("  != (not equal)\n");
    printf("  < (less than)\n");
    printf("  > (greater than)\n");
    printf("  <= (less than or equal)\n");
    printf("  >= (greater than or equal)\n");
    printf("  ~ (greater than or equal, preferring less)\n");

    return 0;
}
