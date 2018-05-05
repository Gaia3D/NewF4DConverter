/**
 * ColorU4 Header
 */
#ifndef _ColorU4_H_
#define _ColorU4_H_
#pragma once

namespace gaia3d
{
	typedef unsigned long ColorU4;

	enum ColorMode {NoColor, SingleColor, ColorsOnVertices};
}

#define LOWBYTE(w)			((unsigned char)(((unsigned long)(w)) & 0xff))
#define MakeColorU4(r,g,b)	((gaia3d::ColorU4)(((unsigned char)(r)|((unsigned short)((unsigned char)(g))<<8))|(((unsigned long)(unsigned char)(b))<<16)))
#define GetRedValue(rgb)	(LOWBYTE(rgb))
#define GetGreenValue(rgb)	(LOWBYTE(((unsigned short)(rgb)) >> 8))
#define GetBlueValue(rgb)	(LOWBYTE((rgb)>>16))
#define DefaultColor		MakeColorU4(204, 204, 204)

/*
static gaia3d::ColorU4 rgba2hex(unsigned int r, unsigned int g, unsigned int b, unsigned int a)
{
	return ((a & 0xff) << 24) + ((b & 0xff) << 16) + ((g & 0xff) << 8) + (r & 0xff);
}
*/
#endif // _ColorU4_H_