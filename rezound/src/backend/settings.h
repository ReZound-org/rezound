/* 
 * Copyright (C) 2002 - David W. Durham
 * 
 * This file is part of ReZound, an audio editing application.
 * 
 * ReZound is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * ReZound is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */

#ifndef __backend_setting_H__
#define __backend_setting_H__

#include "../../config/common.h"


/*
 * This is a repository to which settings can be saved
 */
class CNestedDataFile;
extern CNestedDataFile *gSettingsRegistry;

/*
 * This is the directory that a open/save dialog should open to
 */
#include <string>
extern string gPromptDialogDirectory;


/*
 * These are the paths to the /usr/share/rezound or ~/.rezound directories
 */
extern string gUserDataDirectory;	// "~/.rezound"
extern string gSysDataDirectory;	// "/usr/share/rezound" (or whereever the --prefix was set)

extern string gUserPresetsFile;		// "~/.rezound/presets.dat"
extern string gSysPresetsFile;		// "/usr/share/rezound/presets.dat"


/*
 * These are the parameters of whether or not to snap the selection positions
 * to the cue position, and if so, how far from a cue to does the position have
 * to be before it snaps to it.
 */
extern bool gSnapToCues;
extern unsigned gSnapToCueDistance;

/*
 * True if the sound windows should follow the play position
 */
extern bool gFollowPlayPosition;

/*
 * The initial about of audio to show on screen in a newly created sound window
 */
extern double gInitialLengthToShow;

#endif