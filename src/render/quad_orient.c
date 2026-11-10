/* Quad orientation test - hand-converted from QuadWinding (called by the clipper QuadShapeClassify).
 *
 * `p` points at the four screen vertices x0,y0,x1,y1,x2,y2,x3,y3.  For each of the four corners the cross product of its two edges is computed:
 *   bit n of the result (low byte, EAX) = cross product of corner n is > 0  (counter-clockwise corner)
 *   bit n of an internal mask           = cross product is == 0 (degenerate corner)
 * All four zero -> returns 0xFF.  Otherwise the "zero" corners inherit the orientation of their neighbours (three propagation passes: rotate the bit pattern, mask with the zeros).
 * The return value is only used as a byte (the upper 24 bits of EAX were whatever the caller left there).
 */
#include "../rr.h"

RR_REPLACE_FAST(QuadWinding, rr_QuadWinding, 4);
unsigned int __fastcall rr_QuadWinding(int *p)
{
    unsigned char pos = 0, zero = 0, b;
    int e1, c;

    e1 = p[1] - p[3];
    c = (p[7] - p[1]) * (p[2] - p[0]) + (p[6] - p[0]) * e1;
    if (c > 0) pos = 1; else if (c == 0) zero |= 1;

    {   int e2 = p[3] - p[5];
        c = (p[4] - p[2]) * e1 + (p[0] - p[2]) * e2;
        if (c > 0) pos |= 2; else if (c == 0) zero |= 2;

        e1 = p[5] - p[7];
        c = (p[2] - p[4]) * e1 + e2 * (p[6] - p[4]);
        if (c > 0) pos |= 4; else if (c == 0) zero |= 4;
    }
    c = (p[1] - p[7]) * (p[6] - p[4]) + (p[0] - p[6]) * e1;
    if (c > 0) pos |= 8; else if (c == 0) zero |= 8;

    if (zero == 0xF)
        return 0xFF;
    b = pos;
    b = (unsigned char)(b | (((b >> 3) | (b * 2)) & zero));
    b = (unsigned char)(b | (((b >> 3) | (b * 2)) & zero));
    b = (unsigned char)(b | (((b >> 3) | (b * 2)) & zero));
    return b;
}
