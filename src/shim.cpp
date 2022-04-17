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
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <fcntl.h>

#include "arduinoshim.h"

#if defined(__FREEBSD__) || defined(__OPENBSD__)
#include <sys/sysctl.h>
#endif

#define SDL_InvalidParamError(param)    SDL::SetError("Parameter '%s' is invalid", (param))

/* QNX's /proc/self/exefile is a text file and not a symlink. */
#if !defined(__QNXNTO__)

static char *readSymLink(const char *path) {
  char *retval = NULL;
  ssize_t len = 64;
  ssize_t rc = -1;

  while (1) {
    char *ptr = (char *) SDL::realloc(retval, (size_t) len);
    if (ptr == NULL) {
      SDL::OutOfMemory();
      break;
    }

    retval = ptr;

    rc = readlink(path, retval, len);
    if (rc == -1) {
      break;  /* not a symlink, i/o error, etc. */
    } else if (rc < len) {
      retval[rc] = '\0';  /* readlink doesn't null-terminate. */
      return retval;  /* we're good to go. */
    }

    len *= 2;  /* grow buffer, try again. */
  }

  SDL::free(retval);
  return NULL;
}

#endif
namespace SDL {
char *GetBasePath(void)
{
  char *retval = NULL;

  #if defined(__FREEBSD__)
  char fullpath[PATH_MAX];
  size_t buflen = sizeof (fullpath);
  const int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
  if (sysctl(mib, SDL::arraysize(mib), fullpath, &buflen, NULL, 0) != -1) {
      retval = SDL::strdup(fullpath);
      if (!retval) {
          SDL::OutOfMemory();
          return NULL;
      }
  }
  #endif
  #if defined(__OPENBSD__)
  char **retvalargs;
  size_t len;
  const int mib[] = { CTL_KERN, KERN_PROC_ARGS, getpid(), KERN_PROC_ARGV };
  if (sysctl(mib, 4, NULL, &len, NULL, 0) != -1) {
      retvalargs = SDL::malloc(len);
      if (!retvalargs) {
          SDL::OutOfMemory();
          return NULL;
      }
      sysctl(mib, 4, retvalargs, &len, NULL, 0);
      retval = SDL::malloc(PATH_MAX + 1);
      if (retval)
          realpath(retvalargs[0], retval);

      SDL::free(retvalargs);
  }
  #endif
  #if defined(__SOLARIS__)
  const char *path = getexecname();
  if ((path != NULL) && (path[0] == '/')) { /* must be absolute path... */
      retval = SDL::strdup(path);
      if (!retval) {
          SDL::OutOfMemory();
          return NULL;
      }
  }
  #endif

  /* is a Linux-style /proc filesystem available? */
  if (!retval && (access("/proc", F_OK) == 0)) {
    /* !!! FIXME: after 2.0.6 ships, let's delete this code and just
                  use the /proc/%llu version. There's no reason to have
                  two copies of this plus all the #ifdefs. --ryan. */
    #if defined(__FREEBSD__)
    retval = readSymLink("/proc/curproc/file");
    #elif defined(__NETBSD__)
    retval = readSymLink("/proc/curproc/exe");
    #elif defined(__QNXNTO__)
    retval = SDL::LoadFile("/proc/self/exefile", NULL);
    #else
    retval = readSymLink("/proc/self/exe");  /* linux. */
    if (retval == NULL) {
      /* older kernels don't have /proc/self ... try PID version... */
      char path[64];
      const int rc = (int) SDL::snprintf(path, sizeof(path), "/proc/%llu/exe", (unsigned long long) getpid());
      if ((rc > 0) && ((unsigned) rc < sizeof(path))) {
        retval = readSymLink(path);
      }
    }
    #endif
  }

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

char *GetPrefPath(const char *org, const char *app)
{
  /*
   * We use XDG's base directory spec, even if you're not on Linux.
   *  This isn't strictly correct, but the results are relatively sane
   *  in any case.
   *
   * http://standards.freedesktop.org/basedir-spec/basedir-spec-latest.html
   */
  const char *envr = SDL::getenv("XDG_DATA_HOME");
  const char *append;
  char *retval = NULL;
  char *ptr = NULL;
  size_t len = 0;

  if (!app) {
    SDL_InvalidParamError("app");
    return NULL;
  }
  if (!org) {
    org = "";
  }

  if (!envr) {
    /* You end up with "$HOME/.local/share/Game Name 2" */
    envr = SDL::getenv("HOME");
    if (!envr) {
      /* we could take heroic measures with /etc/passwd, but oh well. */
      SDL::SetError("neither XDG_DATA_HOME nor HOME environment is set");
      return NULL;
    }
    append = "/.local/share/";
  } else {
    append = "/";
  }

  len = SDL::strlen(envr);
  if (envr[len - 1] == '/') {
    append += 1;
  }

  len += SDL::strlen(append) + SDL::strlen(org) + SDL::strlen(app) + 3;
  retval = (char *) SDL::malloc(len);
  if (!retval) {
    SDL::OutOfMemory();
    return NULL;
  }

  if (*org) {
    SDL::snprintf(retval, len, "%s%s%s/%s/", envr, append, org, app);
  } else {
    SDL::snprintf(retval, len, "%s%s%s/", envr, append, app);
  }

  for (ptr = retval + 1; *ptr; ptr++) {
    if (*ptr == '/') {
      *ptr = '\0';
      if (mkdir(retval, 0700) != 0 && errno != EEXIST)
        goto error;
      *ptr = '/';
    }
  }
  if (mkdir(retval, 0700) != 0 && errno != EEXIST) {
    error:
    SDL::SetError("Couldn't create directory '%s': '%s'", retval, strerror(errno));
    SDL::free(retval);
    return NULL;
  }

  return retval;
}
}
