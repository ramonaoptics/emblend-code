/*
 * Copyright (C) 2004-2007 Andrew Mihal
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
#ifndef __GPU_H__
#define __GPU_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define GLEW_STATIC 1


#include <GL/glew.h>
#ifdef HAVE_APPLE_OPENGL_FRAMEWORK
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#define CHECK_GL() checkGLErrors(__LINE__, __FILE__)

void checkGLErrors(int line, char *file);
void printInfoLog(GLhandleARB obj);
bool checkFramebufferStatus();

bool initGPU(int *,char **);
bool configureGPUTextures(unsigned int k, unsigned int vars);
bool gpuGDAKernel(unsigned int k, unsigned int vars, double t, float *packedEData, float *packedPiData, float *packedOutData);
bool clearGPUTextures();
bool wrapupGPU(void);

#endif /* __GPU_H__ */
