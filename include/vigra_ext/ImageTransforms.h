// -*- c-basic-offset: 4 -*-
/** @file ImageTransforms.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: ImageTransforms.h,v 1.2 2007/01/27 05:00:35 acmihal Exp $
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _IMAGETRANSFORMS_H
#define _IMAGETRANSFORMS_H

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/Interpolators.h>
#include <vigra/impex.hxx>
#include <vigra_ext/impexalpha.hxx>

#include <common/math.h>
#include <common/utils.h>

#include "MultiThreadOperations.h"
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

namespace vigra_ext
{


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the bounding
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         TRANSFORM & transform,
                         vigra::Diff2D destUL,
                         Interpolator interp,
                         bool warparound,
                         utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;

    vigra_ext::ImageInterpolator<SrcImageIterator, SrcAccessor, Interpolator>
                                 interpol (src, interp, warparound);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create mask y iterator
    AlphaImageIterator ydm(alpha.first);
    // loop over the image and transform
    typename SrcAccessor::value_type tempval;

    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydm.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdm(ydm);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdm.x)
        {
            double sx,sy;
            if (transform.transformImgCoord(sx,sy,x,y)) {
                if (interpol.operator()(sx, sy, tempval)){
                    // vignetting correction (might need to add random noise..)
                    dest.third.set(tempval, xd);
                    alpha.second.set(255, xdm);
                } else {
                    alpha.second.set(0, xdm);
                }
            } else {
                alpha.second.set(0, xdm);
            }
        }
        if (destSize.y > 100) {
            if ((y-ystart)%(destSize.y/20) == 0) {
                prog.setProgress(((double)y-ystart)/destSize.y);
            }
        }
    }
    prog.popTask();
}


/** functor version (for threaded remapping) */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
struct TransformImageIntern
{
    vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src;
    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest;
    std::pair<AlphaImageIterator, AlphaAccessor> alpha;
    const TRANSFORM & transform;
    vigra::Diff2D destUL;
    Interpolator interp;
    bool warparound;
    utils::MultiProgressDisplay & prog;

    TransformImageIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                              vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                              std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                              const TRANSFORM & transform,
                              vigra::Diff2D destUL,
                              Interpolator interp,
                              bool warparound,
                              utils::MultiProgressDisplay & prog)
    : src(src), dest(dest), alpha(alpha), transform(transform), destUL(destUL), interp(interp), warparound(warparound),
      prog(prog)
    {
    }

    void operator()()
    {
        DEBUG_DEBUG("Starting threaded remap, destUL: " << destUL <<  " area: " << dest.second - dest.first);
        transformImageIntern(src, dest, alpha, transform, destUL, interp, warparound, prog);
        DEBUG_DEBUG("Finished threaded remap, destUL: " << destUL <<  " area: " << dest.second - dest.first);
    }
};


/** transform input images with alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                               std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                               vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                               std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                               TRANSFORM & transform,
                               vigra::Diff2D destUL,
                               Interpolator interp,
                               bool warparound,
                               utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;

    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;

    vigra_ext::ImageMaskInterpolator<SrcImageIterator, SrcAccessor, SrcAlphaIterator,
                                     SrcAlphaAccessor, Interpolator>
                                    interpol (src, srcAlpha, interp, warparound);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    AlphaImageIterator ydist(alpha.first);

    typename SrcAccessor::value_type tempval;

    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        AlphaImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            if (transform.transformImgCoord(sx,sy,x,y)) {
                // try to interpolate.
                if (interpol(sx, sy, tempval)) {
                    dest.third.set(tempval, xd);
                    alpha.second.set(255, xdist);
                } else {
                    // point outside of image or mask
                    alpha.second.set(0, xdist);
                }
            } else {
                alpha.second.set(0, xdist);
            }
        }
        if (destSize.y > 100) {
            if ((y-ystart)%(destSize.y/20) == 0) {
                prog.setProgress(((double)y-ystart)/destSize.y);
            }
        }
    }
    prog.popTask();
};


/** functor version (for threaded remapping) */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
struct TransformImageAlphaIntern
{
    vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src;
    std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha;
    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest;
    std::pair<AlphaImageIterator, AlphaAccessor> alpha;
    const TRANSFORM & transform;
    vigra::Diff2D destUL;
    Interpolator interp;
    bool warparound;
    utils::MultiProgressDisplay & prog;

    TransformImageAlphaIntern(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                              std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                              vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                              std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                              const TRANSFORM & transform,
                              vigra::Diff2D destUL,
                              Interpolator interp,
                              bool warparound,
                              utils::MultiProgressDisplay & prog)
    : src(src), srcAlpha(srcAlpha), dest(dest), alpha(alpha), transform(transform), destUL(destUL), interp(interp), warparound(warparound),
      prog(prog)
    {
    }

    void operator()()
    {
        DEBUG_DEBUG("Starting threaded remap, destUL: " << destUL <<  " area: " << dest.second - dest.first);
        transformImageAlphaIntern(src, srcAlpha, dest, alpha, transform, destUL, interp, warparound, prog);
        DEBUG_DEBUG("Finished threaded remap, destUL: " << destUL <<  " area: " << dest.second - dest.first);
    }
};


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class DistImageIterator, class DistAccessor,
          class Interpolator>
void transformImageDist(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                        vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                        vigra::Diff2D destUL,
                        TRANSFORM & transform,
                        vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist,
                        Interpolator & interp,
                        utils::MultiProgressDisplay & prog)
{
    vigra::Diff2D destSize = dest.second - dest.first;
    vigra::Diff2D distSize = centerDist.second - centerDist.first;

    bool calcDist=true;
    if (distSize.x == 0 && distSize.y == 0) {
        calcDist=false;
    }

    if (calcDist) {
        DEBUG_ASSERT(distSize == destSize);
    }
    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    prog.pushTask(utils::ProgressTask("Remapping", "", 1.0/(yend-ystart)));

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    vigra_ext::InterpolatingAccessor<SrcAccessor,
                          typename SrcAccessor::value_type,
                          Interpolator> interpol(src.third, interp);


//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    DistImageIterator ydist(centerDist.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        DistImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            if (transform.transformImgCoord(sx,sy,x,y)) {
                // make sure that the interpolator doesn't
                // access pixels outside.. Should we introduce
                // some sort of border treatment?
                if (sx < interp.size/2 -1
                    || sx > srcSize.x-interp.size/2 - 1
                    || sy < interp.size/2 - 1
                    || sy > srcSize.y-interp.size/2 - 1)
                {
                    if (calcDist) {
                        // save an invalid distance
                        *xdist = FLT_MAX;
                    }
                    // nothing..
                } else {
    //                cout << x << "," << y << " -> " << sx << "," << sy << " " << std::endl;
    
    //                nearest neighbour
    //                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                    // use given interpolator function.
                    *xd = interpol(src.first, sx, sy);
                    if (calcDist) {
                        double mx = sx - srcMiddle.x;
                        double my = sy - srcMiddle.y;
                        *xdist = sqrt(mx*mx + my*my);
                    }
                }
            } else {
                *xdist = FLT_MAX;
            }
        }
        if ((y-ystart)%100 == 0) {
            prog.setProgress(((double)y-ystart)/(yend-ystart));
        }
    }
    prog.popTask();
}


/// multithreaded image transformation.


/// multithreaded image transformation.
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageAlphaInternMT(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                                 std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                                 vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                                 std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                                 TRANSFORM & transform,
                                 vigra::Diff2D destUL,
                                 Interpolator interp,
                                 bool warparound,
                                 utils::MultiProgressDisplay & prog)
{
    // divide output image into multiple areas
    unsigned int nThreads = ThreadManager::get().getNThreads();

    vigra::Diff2D destSize = dest.second - dest.first;

    // limit amount of threads if only a few lines are given.
    if (destSize.y < (int) nThreads) {
        nThreads = destSize.y;
    }

    if (nThreads == 1) {
        transformImageAlphaIntern(src, srcAlpha, dest, alpha, transform,
                                  destUL, interp, warparound, prog);
        return;
    }

    DEBUG_DEBUG("creating " << nThreads << " threads for remapping");

    unsigned int chunkSize = destSize.y / nThreads;
// DGSW FIXME - Unreferenced
//	unsigned int lastChunkSize = destSize.y - (nThreads-1) * chunkSize;

    // create a threads to remap each area
    boost::thread_group threads;

    // create first thread with progress counter
    DestImageIterator destStart = dest.first;
    DestImageIterator destEnd = dest.second;
    destEnd.y -= destSize.y - chunkSize;
    AlphaImageIterator destAStart = alpha.first;

    typedef TransformImageAlphaIntern<SrcImageIterator, SrcAccessor, SrcAlphaIterator, SrcAlphaAccessor, DestImageIterator, DestAccessor,
                                      TRANSFORM, AlphaImageIterator, AlphaAccessor, Interpolator> RFunctor;

    utils::MultiProgressDisplay dummyProg;
    unsigned int i;
    for (i = 0; i < nThreads-1; ++i) {

        RFunctor t(src, srcAlpha, vigra::triple<DestImageIterator, DestImageIterator, DestAccessor>(destStart, destEnd, dest.third),
                                                vigra::pair<AlphaImageIterator, AlphaAccessor>(destAStart, alpha.second), 
                                                transform, destUL, interp, warparound, dummyProg);
        boost::function0<void> f;
        f = t;
    	DEBUG_DEBUG("Starting thread " << i);
        threads.create_thread(f);

        destStart.y += chunkSize;
        destEnd.y += chunkSize;
        destAStart.y += chunkSize;
        destUL.y += chunkSize;
    }
    // last chunk
    destEnd = dest.second;
    // remap last chunk in current thread.
    if (i == nThreads-1) {
        DEBUG_DEBUG("remapping in main thread, destUL: " << destUL <<  " area: " << dest.second - dest.first);
        transformImageAlphaIntern( src, srcAlpha,
                                  vigra::triple<DestImageIterator, DestImageIterator, DestAccessor>(destStart, destEnd, dest.third),
                                  vigra::pair<AlphaImageIterator, AlphaAccessor>(destAStart, alpha.second), 
                                  transform, destUL, interp, warparound, prog);
    }

    DEBUG_DEBUG("Waiting for threads to join");
    threads.join_all();
    DEBUG_DEBUG("Threads joined");
}

template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class TRANSFORM,
          class AlphaImageIterator, class AlphaAccessor,
          class Interpolator>
void transformImageInternMT(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                            vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                            std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                            TRANSFORM & transform,
                            vigra::Diff2D destUL,
                            Interpolator interp,
                            bool warparound,
                            utils::MultiProgressDisplay & prog)
{
    // divide output image into multiple areas
    unsigned int nThreads = ThreadManager::get().getNThreads();

    vigra::Diff2D destSize = dest.second - dest.first;

    // limit amount of threads if only a few lines are given.
    if (destSize.y < (int) nThreads) {
        nThreads = destSize.y;
    }

    if (nThreads == 1) {
        transformImageIntern(src, dest, alpha, transform,
                                  destUL, interp, warparound, prog);
        return;
    }

    DEBUG_DEBUG("creating " << nThreads << " threads for remapping");

    unsigned int chunkSize = destSize.y / nThreads;
// DGSW FIXME - Unreferenced
//	unsigned int lastChunkSize = destSize.y - (nThreads-1) * chunkSize;

    // create a threads to remap each area
    boost::thread_group threads;

    // create first thread with progress counter
    DestImageIterator destStart = dest.first;
    DestImageIterator destEnd = dest.second;
    destEnd.y -= destSize.y - chunkSize;
    AlphaImageIterator destAStart = alpha.first;

    typedef TransformImageIntern<SrcImageIterator, SrcAccessor, DestImageIterator, DestAccessor,
                                      TRANSFORM, AlphaImageIterator, AlphaAccessor, Interpolator> RFunctor;

    utils::MultiProgressDisplay dummyProg;
    unsigned int i;
    for (i = 0; i < nThreads-1; ++i) {

        RFunctor t(src, vigra::triple<DestImageIterator, DestImageIterator, DestAccessor>(destStart, destEnd, dest.third),
                   vigra::pair<AlphaImageIterator, AlphaAccessor>(destAStart, alpha.second), 
                   transform, destUL, interp, warparound, dummyProg);
        boost::function0<void> f;
        f = t;
    	DEBUG_DEBUG("Starting thread " << i);
        threads.create_thread(f);

        destStart.y += chunkSize;
        destEnd.y += chunkSize;
        destAStart.y += chunkSize;
        destUL.y += chunkSize;
    }
    // last chunk
    destEnd = dest.second;
    // remap last chunk in current thread.
    if (i == nThreads-1) {
        DEBUG_DEBUG("remapping in main thread, destUL: " << destUL <<  " area: " << dest.second - dest.first);
        transformImageIntern( src,
                              vigra::triple<DestImageIterator, DestImageIterator, DestAccessor>(destStart, destEnd, dest.third),
                              vigra::pair<AlphaImageIterator, AlphaAccessor>(destAStart, alpha.second), 
                              transform, destUL, interp, warparound, prog);
    }

    DEBUG_DEBUG("Waiting for threads to join");
    threads.join_all();
    DEBUG_DEBUG("Threads joined");
}


/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interpol Interpolation algorithm that should be used.
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                    vigra::Diff2D destUL,
                    TRANSFORM & transform,
                    bool warparound,
                    Interpolator interpol,
                    utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageIntern(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_cubic(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline16(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline36(), warparound,
                                 progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_spline64(), warparound,
                                 progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<8>(), warparound,
                                 progress);
	break;
    case INTERP_BILINEAR:
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_bilin(), warparound,
                                 progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_nearest(), warparound,
                                 progress);
	break;
    case INTERP_SINC_1024:
	transformImageInternMT(src, dest, alpha, transform, destUL,
                                 vigra_ext::interp_sinc<32>(), warparound,
                                 progress);
	break;
    }
}

/** Transform image, and respect a possible alpha channel */
template <class SrcImageIterator, class SrcAccessor,
          class SrcAlphaIterator, class SrcAlphaAccessor,
          class DestImageIterator, class DestAccessor,
          class AlphaImageIterator, class AlphaAccessor,
          class TRANSFORM>
void transformImageAlpha(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         std::pair<SrcAlphaIterator, SrcAlphaAccessor> srcAlpha,
                         vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                         std::pair<AlphaImageIterator, AlphaAccessor> alpha,
                         vigra::Diff2D destUL,
                         TRANSFORM & transform,
                         bool warparound,
                         Interpolator interpol,
                         utils::MultiProgressDisplay & progress)
{
    switch (interpol) {
    case INTERP_CUBIC:
	DEBUG_DEBUG("using cubic interpolator");
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
				              vigra_ext::interp_cubic(), warparound,
                              progress);
	break;
    case INTERP_SPLINE_16:
	DEBUG_DEBUG("interpolator: spline16");
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_spline16(), warparound,
		                      progress);
	break;
    case INTERP_SPLINE_36:
	DEBUG_DEBUG("interpolator: spline36");
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_spline36(),  warparound,
		                      progress);
	break;
    case INTERP_SPLINE_64:
	DEBUG_DEBUG("interpolator: spline64");
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_spline64(),  warparound,
		                      progress);
	break;
    case INTERP_SINC_256:
	DEBUG_DEBUG("interpolator: sinc 256");
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_sinc<8>(), warparound,
		                      progress);
	break;
    case INTERP_BILINEAR:
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_bilin(), warparound,
		                      progress);
	break;
    case INTERP_NEAREST_NEIGHBOUR:
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_nearest(), warparound,
		                      progress);
	break;
    case INTERP_SINC_1024:
	transformImageAlphaInternMT(src,srcAlpha, dest, alpha, transform, destUL,
                              vigra_ext::interp_sinc<32>(), warparound,
		                      progress);
	break;
    }
}

template <class T, int nr>
void fillVector(T vec[3], T &val, int len)
{
    for (int i=0; i<len; i++) vec[i] = val;
}


}; // namespace

#endif // _IMAGETRANSFORMS_H
