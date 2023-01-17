/*
 * AMD PSP emulation
 *
 * Copyright (C) 2020 Robert Buhren <robert@robertbuhren.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * CCP5 zlib bindings
 */
#ifndef AMD_CCP_V5_ZLIB_H
#define AMD_CCP_V5_ZLIB_H

#include <zlib.h>
#include <string.h>
#include "exec/hwaddr.h"

#define CCP_ZLIB_CHUNK_SIZE 4096
typedef z_stream CcpV5ZlibState;

inline static int ccp_zlib_init(CcpV5ZlibState *s) {
  if( s != NULL) {
    memset(s, 0, sizeof(CcpV5ZlibState));
    if (inflateInit2(s, MAX_WBITS) != Z_OK)
      return -1;
  }
  return 0;
}


inline static int ccp_zlib_end(CcpV5ZlibState *s) {
  if (s != NULL) {
    if (inflateEnd(s) != Z_OK)
      return -1;
  }
  return 0;
}

inline static int ccp_zlib_inflate(CcpV5ZlibState *s, hwaddr in, hwaddr out, void* dst, void* src) {
  if ( s != NULL) {

    s->avail_in = in;
    s->next_in = src;
    s->avail_out = out;
    s->next_out = dst; 

    if( inflate(s, Z_NO_FLUSH) != Z_OK)
      return -1;

  }
  return 0;

}


#endif
