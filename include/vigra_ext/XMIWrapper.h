/*
 * Copyright (C) 2004-2005 Andrew Mihal
 *
 * This file is part of Enblend.
 *
 * Enblend is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Enblend is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Enblend; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __XMIWRAPPER_H__
#define __XMIWRAPPER_H__

extern "C" {
#include <xmi.h>
}

/* This is the public include file for the miPaintedSet module, contained
   in mi_spans.c. */

/* A Spans structure is a sorted list of spans, i.e. a list of point ranges
   [xmin,xmax], sorted in increasing y order.  There may be more than one
   span at a given y. */

typedef struct 
{
  int		count;		/* number of spans		    */
  miPoint	*points;	/* pointer to list of start points  */
  unsigned int	*widths;	/* pointer to list of widths	    */
} Spans;

/* A SpanGroup is an unsorted list of Spans's, associated with a particular
   pixel value.  

   A SpanGroup is allowed to include more than a single Spans because most
   libxmi drawing functions write out multiple Spans's. */

typedef struct 
{
    miPixel     pixel;		/* pixel value				*/
    Spans       *group;		/* Spans slots				*/
    int		size;		/* number of Spans slots allocated	*/
    int		count;		/* number of Spans slots filled		*/
    int		ymin, ymax;	/* min, max y values over all Spans's	*/
} SpanGroup;

/* A miPaintedSet structure is an array of SpanGroups, specifying the
   partition into differently painted subsets.  There is at most one
   SpanGroup for any pixel. */

typedef struct lib_miPaintedSet
{
  SpanGroup	**groups;	/* SpanGroup slots			*/
  int		size;		/* number of SpanGroup slots allocated	*/
  int		ngroups;	/* number of SpanGroup slots filled	*/
} _miPaintedSet;

namespace vigra_ext {

template<class DestIterator, class DestAccessor>
void copyPaintedSetToImage(DestIterator dest_upperleft,
                           DestIterator dest_lowerright,
                           DestAccessor da,
                           const miPaintedSet *paintedSet,
                           const Diff2D offset) {

    int dst_w = dest_lowerright.x - dest_upperleft.x;
    int dst_h = dest_lowerright.y - dest_upperleft.y;

    //cout << "paintedSet->ngroups=" << paintedSet->ngroups << endl;
    for (int group = 0; group < paintedSet->ngroups; group++) {
        //cout << "    group " << group << " count=" << paintedSet->groups[group]->group[0].count << endl;
        if (paintedSet->groups[group]->group[0].count > 0) {
            miPixel pixel = paintedSet->groups[group]->pixel;
            int spans = paintedSet->groups[group]->group[0].count;
            //cout << " group " << group << " spans=" << spans << endl;
            const miPoint *ppt = paintedSet->groups[group]->group[0].points;
            unsigned int *pwidth = paintedSet->groups[group]->group[0].widths;

            //if (ppt[0].y + offset.y >= dst_h) { cout << "min y coord is outside bb" << endl; continue; }
            //if (ppt[spans-1].y + offset.y < 0) { cout << "max y coord is outside bb" << endl; continue; }
            if (ppt[0].y + offset.y >= dst_h) continue;
            if (ppt[spans-1].y + offset.y < 0) continue;

            for (int i = 0; i < spans; i++) {
                int y = ppt[i].y + offset.y;
                //if (y < 0) { cout << "span y is less than 0" << endl; break; }
                if (y < 0) continue;
                //if (y >= dst_h) {cout << "span y is greater than dst_h" << endl; break; }
                if (y >= dst_h) continue;

                int width = pwidth[i];
                int xstart = ppt[i].x + offset.x;
                int xend = xstart + width - 1;

                int xstart_clip = (xstart < 0) ? 0 : xstart;
                int xend_clip = (xend >= dst_w) ? (dst_w-1) : xend;

                //int pixelsDrawn = 0;
                DestIterator dx = dest_upperleft + Diff2D(xstart_clip, y);
                for (int x = xstart_clip; x <= xend_clip; ++x, ++dx.x) {
                    da.set(pixel, dx);
                    //++pixelsDrawn;
                }
                //cout << " group " << group << " span " << i << " pixelsDrawn=" << pixelsDrawn << endl;
            }
        }
    }

};

template<class DestIterator, class DestAccessor>
void copyPaintedSetToImage(vigra::triple<DestIterator, DestIterator, DestAccessor> image,
                           const miPaintedSet *paintedSet,
                           const Diff2D offset) {
    copyPaintedSetToImage(image.first, image.second, image.third, paintedSet, offset);
};

} // namespace vigra_ext

#endif /* __XMIWRAPPER_H__ */
