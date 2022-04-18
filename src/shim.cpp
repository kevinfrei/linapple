// This file contains shim functions we're using temporarily before we port
// the whole codebase to SDL2

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// These come right from SDL2

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent filesystem routines                                */

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>

#include "arduinoshim.h"

#define SDL_InvalidParamError(param)    SDL::SetError("Parameter '%s' is invalid", (param))

namespace SDL {
char *GetBasePath(void)
{
  char *retval = NULL;

  /* If we had access to argv[0] here, we could check it for a path,
      or troll through $PATH looking for it, too. */

  if (retval != NULL) { /* chop off filename. */
    char *ptr = SDL::strrchr(retval, '/');
    if (ptr != NULL) {
      *(ptr + 1) = '\0';
    } else {  /* shouldn't happen, but just in case... */
      SDL::free(retval);
      retval = NULL;
    }
  }

  if (retval != NULL) {
    /* try to shrink buffer... */
    char *ptr = (char *) SDL::realloc(retval, strlen(retval) + 1);
    if (ptr != NULL)
      retval = ptr;  /* oh well if it failed. */
  }

  return retval;
}

}
