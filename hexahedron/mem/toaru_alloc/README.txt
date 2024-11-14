toaru_alloc allocator for Hexahedron
======================================


---------- DESCRIPTION ----------
toaru_alloc uses a stable slab allocator that is known good and working, sourced from ToaruOS.
It is designed to be the fallback case, and may occasionally be used as the default (before Hexalloc slab-style is complete).


----------- LICENSING -----------
(pulled directly from malloc.c)

 * This file is part of ToaruOS and is released under the terms
 * of the NCSA / University of Illinois License - see LICENSE.md
 * Copyright (c) 2010-2021 K. Lange.  All rights reserved.
 *
 * Developed by: K. Lange <klange@toaruos.org>
 *               Dave Majnemer <dmajnem2@acm.uiuc.edu>
 *               Assocation for Computing Machinery
 *               University of Illinois, Urbana-Champaign
 *               http://acm.uiuc.edu

 