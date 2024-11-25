/**
 * @file hexahedron/drivers/video.c
 * @brief Generic video driver
 * 
 * This video driver system handles abstracting the video layer away.
 * It supports text-only video drivers (but may cause gfx display issues) and it
 * supports pixel-based video drivers.
 * 
 * Currently, implementation is rough because we are at the beginnings of a new kernel,
 * but here is what is planned:
 * - Implement a list system to allow choosing
 * - Allow arguments to modify this
 * 
 * @copyright
 * This file is part of the Hexahedron kernel, which is part of reduceOS.
 * It is released under the terms of the BSD 3-clause license.
 * Please see the LICENSE file in the main repository for more details.
 * 
 * Copyright (C) 2024 Samuel Stuart
 */

#include <drivers/video.h>
#include <structs/list.h>


