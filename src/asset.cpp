/*
	asset.cpp - LinApple asset management
	<one line to give the program's name and a brief idea of what it does.>
	Copyright (C) 2018  T. Joseph Carter

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

// #include <sys/param.h>

#include "asset.h"
#include "stdafx.h"  // for Disk.h DiskInsert()
#include "shim.h"  // SDL::GetBasePath()

#include "../res/font.xpm"
#include "../res/icon.xpm"
#include "../res/splash.xpm"

#define ASSET_MASTER_DSK     "Master.dsk"

assets_t *assets = NULL;

#ifdef ASSET_DIR
static char system_assets[] = ASSET_DIR "/";
#else
static char system_assets[] = "./";
#endif
static char *system_exedir = NULL;

constexpr size_t PATH_MAX=240;

SDL::Surface *Asset_LoadBMP(const char *filename)
{
  SDL::Surface *surf;

  std::string path = SPrintf(system_assets, filename);
  surf = SDL::LoadBMP(path.c_str());
  if (NULL == surf) {
    path = SPrintf(system_exedir, filename);
    surf = SDL::LoadBMP(path.c_str());
  }

  if (NULL != surf) {
    ErrDumpln("Asset_LoadBMP: Loaded ", filename, " from ", path);
  } else {
    ErrDump("Asset_LoadBMP: Couldn't load ", filename, " in either ",system_assets," or ",system_exedir,"!");
  }

  return surf;
}

bool Asset_Init(void)
{
  system_exedir = SDL::GetBasePath();
  if (NULL == system_exedir) {
    ErrDumpln("Asset_Init: Warning: SDL::GetBasePath() returned NULL, using \"./\"");
    system_exedir = SDL::strdup("./");
  }

  assets = (assets_t *) SDL::calloc(1, sizeof(assets_t));
  if (NULL == assets) {
    ErrDumpln("Asset_Init: Allocating assets: ", SDL::GetError());
    return false;
  }

  assets->icon = IMG_ReadXPMFromArray(icon_xpm);
  if (NULL == assets->icon) {
    return false;
  }

  assets->font = IMG_ReadXPMFromArray(font_xpm);
  if (NULL == assets->font) {
    return false;
  }

  assets->splash = IMG_ReadXPMFromArray(splash_xpm);
  if (NULL == assets->splash) {
    return false;
  }

  return true;
}

void Asset_Quit(void)
{
  if (NULL != assets) {
    if (NULL != assets->icon) {
      SDL::FreeSurface(assets->icon);
      assets->icon = NULL;
    }

    if (NULL != assets->font) {
      SDL::FreeSurface(assets->font);
      assets->font = NULL;
    }

    if (NULL != assets->splash) {
      SDL::FreeSurface(assets->splash);
      assets->splash = NULL;
    }

    if (NULL != system_exedir) {
      SDL::free(system_exedir);
      system_exedir = NULL;
    }

    SDL::free(assets);
  }
}

int Asset_FindMasterDisk(char *path_out)
{
  // {path_out} gets the path found.
  // Returns non-zero if no path was found.
  //
  // TODO use XDG lookups eg XDG_CONFIG_HOME, XDG_CONFIG_PATHS.
  // TODO the last ditch paths are bunk -- look for better conventions.

  int err = 255;
  const int count = 5;
  char *paths[count];
  char path[MAX_PATH+1];

  // Allocate.
  for (int i=0; i<count; i++)
    paths[i] = (char *)SDL::malloc(sizeof(char[PATH_MAX+1]));

  // Define search paths in precedence order.
  strcpy(paths[0], ".");
  strcpy(paths[1], "share/applino"); // testing convenience
  strcpy(paths[2], SDL::getenv("HOME"));
  strcat(paths[2], "/.local/share/applino");
  strcpy(paths[3], "/usr/local/share/applino");
  strcpy(paths[4], "/usr/share/applino");

  for (auto p: paths) {
    std::string path = SPrintf(p, "/", ASSET_MASTER_DSK);
    ErrDumpln("[debug] Searching: ", p, " for ", ASSET_MASTER_DSK);
    FILE *fp = fopen(path.c_str(), "r");
    if (fp) {
      fclose(fp);
      strcpy(path_out, path.c_str());
      err = 0;
      break;
    }
  }

  if (err)
  {
    ErrDumpln("[warn ] could not find ",ASSET_MASTER_DSK," at any of:");
    for (auto i=0; i<count; i++)
      ErrDumpln("[warn ] ", paths[i]);
  }
  else
  {
    ErrDumpln("[info ] Master disk: ", path);
  }

  // Deallocate.
  for (auto i=0; i<count; i++)
    SDL::free(paths[i]);

  return err;
}

int Asset_InsertMasterDisk(void)
{
  char *path = (char *) SDL::malloc(sizeof(char[PATH_MAX+1]));

  int err = Asset_FindMasterDisk(path);
  if (err) {
    SDL::free(path);
    return 255;
  }

  int rc = DiskInsert(0, path, 0, 0);

  SDL::free(path);
  return rc;
}
