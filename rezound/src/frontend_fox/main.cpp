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

#include "../../config/common.h"
#include "fox_compat.h"

#include <fox/fx.h>

#include <stdexcept>
#include <clocale> // for gettext init

#include <CPath.h>
DECLARE_STATIC_CPATH // to declare CPath::dirDelim

#include "CMainWindow.h"
#include "CMetersWindow.h"
#include "CStatusComm.h"

#include "CSoundFileManager.h"
#include "CFrontendHooks.h"
#include "../backend/initialize.h"
#include "settings.h"

#include "CFOXIcons.h"

#include "CAboutDialog.h"

static void setupWindows(CMainWindow *mainWindow);
static void setupAccels(CMainWindow *mainWindow);

static void setLocaleFont(FXApp *application);


// --- FOR TESTING
static void countWidgets(FXWindow *w);
static void printNormalFontProperties(FXApp *application);
static void listFonts();
// ---

int main(int argc,char *argv[])
{
#ifdef ENABLE_NLS
	setlocale(LC_ALL,"");
	bindtextdomain(REZOUND_PACKAGE,DATA_DIR"/locale");
	textdomain(REZOUND_PACKAGE);
#endif

	try
	{
		FXApp* application=new FXApp("ReZound","NLT");
		application->init(argc,argv);

#ifdef ENABLE_NLS
		setLocaleFont(application);
#endif

		//printNormalFontProperties(application);
		//listFonts();

		FOXIcons=new CFOXIcons(application);

#if FOX_MAJOR >= 1 // otherwise they just look funny which is okay i guess
		// these icons have a special property that they don't need a transparent color
		FOXIcons->logo->setTransparentColor(FXRGB(0,0,0));
		FOXIcons->icon_logo_16->setTransparentColor(FXRGB(0,0,0));
		FOXIcons->icon_logo_32->setTransparentColor(FXRGB(0,0,0));
#endif

		/*
		// set application wide colors
		application->setBaseColor(FXRGB(193,163,102));
		... there are others that could be set ...
		*/

		// you have to do this for hints to be activated
		new FXToolTip(application,TOOLTIP_VARIABLE);

		// create the main window 
		CMainWindow *mainWindow=new CMainWindow(application);
		application->create();

		// unfortunately we have to create main window before we can pop up error messages
		// which means we will have to load plugins and add their buttons to an already
		// created window... because there could be errors while loading
		// ??? I could fix this by delaying the creation of buttons after the creation of the main window.. and I should do this, since if I ever support loading plugins, I need to be able to popup error dialogs while loading them
		gStatusComm=new CStatusComm(mainWindow);
		gFrontendHooks=new CFrontendHooks(mainWindow);

		// from here on we can create error messages
		//   ??? I suppose I could atleast print to strerr if gStatusComm was not created yet

		ASoundPlayer *soundPlayer=NULL;
		if(!initializeBackend(soundPlayer,argc,argv))
			return 0;

		// the backend needed to be setup before this stuff was done
		static_cast<CFrontendHooks *>(gFrontendHooks)->doSetupAfterBackendIsSetup();


		gSoundFileManager=new CSoundFileManager(mainWindow,soundPlayer,gSettingsRegistry);

		mainWindow->getMetersWindow()->setSoundPlayer(soundPlayer);

		// create all the dialogs 
		setupWindows(mainWindow);

		// setup an FXAccelTable which allows for keys to invoke actions
		setupAccels(mainWindow);

		// load any sounds that were from the previous session
		const vector<string> errors=gSoundFileManager->loadFilesInRegistry();
		for(size_t t=0;t<errors.size();t++)
			Error(errors[t]);

		// give the backend another oppertunity to handle arguments (like loading files)
		if(!handleMoreBackendArgs(gSoundFileManager,argc,argv))
			return 0;

		gAboutDialog->showOnStartup();
		mainWindow->show();
		application->run();

		//countWidgets(application->getRootWindow()); printf("wc: %d\n",wc);

		delete gSoundFileManager;

		deinitializeBackend();

		// ??? delete gFrontendHooks
		// ??? delete gStatusComm;

		delete FOXIcons;
		
		//delete application; dies while removing an accelerator while destroying an FXMenuCommand... I'm not sure why it's a problem
	}
	catch(exception &e)
	{
		if(gStatusComm!=NULL)
			Error(e.what());
		else
			fprintf(stderr,"exception -- %s\n",e.what());

		return 1;
	}
	catch(...)
	{
		if(gStatusComm!=NULL)
			Error("unknown exception caught\n");
		else
			fprintf(stderr,"unknown exception caught\n");

		return 1;
	}

	return 0;
}



#include "CChannelSelectDialog.h"
#include "CPasteChannelsDialog.h"
#include "CCueDialog.h"
#include "CCueListDialog.h"
#include "CUserNotesDialog.h"
#include "CCrossfadeEdgesDialog.h"

void setupWindows(CMainWindow *mainWindow)
{
		gAboutDialog=new CAboutDialog(mainWindow);

		// create the channel select dialog that AActionFactory is given to use often
		gChannelSelectDialog=new CChannelSelectDialog(mainWindow);

		// create the channel select dialog that AActionFactory uses for selecting channels to paste to
		gPasteChannelsDialog=new CPasteChannelsDialog(mainWindow);

		// create the dialog used to obtain the name for a new cue
		gCueDialog=new CCueDialog(mainWindow);

		// create the dialog used to manipulate a list of cues
		gCueListDialog=new CCueListDialog(mainWindow);

		// create the dialog used to make user notes saved with the sound
		gUserNotesDialog=new CUserNotesDialog(mainWindow);

		// create the dialog used to set the length of the crossfade on the edges
		gCrossfadeEdgesDialog=new CCrossfadeEdgesDialog(mainWindow);

		// create the tool bars in CMainWindow
		mainWindow->createMenus();
}

#include <fox/fxkeys.h>
void setupAccels(CMainWindow *mainWindow)
{
	FXAccelTable *at=mainWindow->getAccelTable();


/* these are not necessary since they are hot-keys on the 'Control' menu
	// play controls
	at->addAccel(KEY_a ,mainWindow,MKUINT(CMainWindow::ID_PLAY_SELECTION_ONCE,SEL_COMMAND));
	at->addAccel(KEY_s ,mainWindow,MKUINT(CMainWindow::ID_STOP,SEL_COMMAND));

	// view
	at->addAccel(KEY_z ,mainWindow,MKUINT(CMainWindow::ID_FIND_SELECTION_START,SEL_COMMAND));
	at->addAccel(KEY_x ,mainWindow,MKUINT(CMainWindow::ID_FIND_SELECTION_STOP,SEL_COMMAND));
*/

	// shuttle (is on the 'Control' menu, but I need a key-up event handler as well)
	at->addAccel(KEY_1 ,mainWindow,MKUINT(CMainWindow::ID_SHUTTLE_BACKWARD,SEL_COMMAND),MKUINT(CMainWindow::ID_SHUTTLE_RETURN,SEL_COMMAND));
	//on the 'Control' menu:  at->addAccel(KEY_2 ,mainWindow,MKUINT(CMainWindow::ID_SHUTTLE_INCREASE_RATE,SEL_COMMAND));
	at->addAccel(KEY_3 ,mainWindow,MKUINT(CMainWindow::ID_SHUTTLE_FORWARD,SEL_COMMAND),MKUINT(CMainWindow::ID_SHUTTLE_RETURN,SEL_COMMAND));


	// sound switching
	at->addAccel(MKUINT(KEY_1,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_2,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_3,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_4,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_5,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_6,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_7,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_8,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_9,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_0,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
	at->addAccel(MKUINT(KEY_quoteleft,ALTMASK),mainWindow,MKUINT(CMainWindow::ID_SOUND_LIST_HOTKEY,SEL_COMMAND));
}


#ifdef ENABLE_NLS
void setLocaleFont(FXApp *application)
{
	/*??? it'd be nice if I knew if the font choice was overridden in the FOX registry... I should also have a font dialog for choosing in the config system later */
	const string lang=setlocale(LC_MESSAGES,NULL);
	if(lang=="")
		return;
	else if(lang=="ru" || lang=="ru_RU")
	{ // setup KOI-8 encoded font
		FXFontDesc desc={
			"helvetica",
			100,
			FONTWEIGHT_BOLD,
			FONTSLANT_REGULAR,
			FONTENCODING_KOI8_R,
			FONTSETWIDTH_DONTCARE,
			0
		};

		application->setNormalFont(new FXFont(application,desc));
	}
}
#endif


// --- FUNCTIONS ONLY USED WHEN TESTING THINGS ---

#if 0
int wc=1;
void countWidgets(FXWindow *w)
{
	w=w->getFirst();
	if(w)
	{
		do
		{
			wc++;
			countWidgets(w);
		}
		while((w=w->getNext()));
	}
}
#endif

#if 0
void printNormalFontProperties(FXApp *application)
{
	FXFontDesc desc;
	application->getNormalFont()->getFontDesc(desc);
	printf("normal font {\n");
	printf("\tface: '%s'\n",desc.face);
	printf("\tsize: %d\n",desc.size);
	printf("\tweight: %d\n",desc.weight);
	printf("\tslant: %d\n",desc.slant);
	printf("\tencoding: %d\n",desc.encoding);
	printf("\tsetwidth: %d\n",desc.setwidth);
	printf("\tflags: %d\n",desc.flags);
	printf("}\n");
}
#endif

#if 0
void listFonts()
{
	printf("font_listing {\n");

	FXFontDesc *fonts;
	FXuint numfonts;
	if(FXFont::listFonts(fonts,numfonts,"helvetica"))
	{
		printf("\tnum fonts: %d\n",numfonts);
		for(FXuint t=0;t<numfonts;t++)
			printf("\t%d: face: '%s' encoding: %d\n",t,fonts[t].face,fonts[t].encoding);
	}
	
	printf("}\n");
}
#endif


