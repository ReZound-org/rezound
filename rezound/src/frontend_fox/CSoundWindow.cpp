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

static void playTrigger(void *Pthis);
static void pauseTrigger(void *Pthis);

/*
 * TODO
 * 	- figure out why I can't get all the decorations to show when the window pops up
 *		- well, all window managers EXCEPT enlightenment and blackbox to have a resizable border for CSoundWindow
 *
 * 	- change the clicking on the + and - signs go between 0, 25, 50, 100 
 *
 * 	- maybe replace the +/- signs with little magnifying glass icons
 *
 * 	- I may need to delete the FXIcon objects I'm creating
 *
 * 	- create a better way of placing the newly created sound window, below the mainWindow and right of the edit tool bar
 */

#include "CSoundWindow.h"
#include "CSoundFileManager.h"

#include <istring>

#include "../backend/CLoadedSound.h"

#include "../backend/Edits/CCueAction.h"
#include "CCueDialog.h"
#include "CCueListDialog.h"

#include "../images/images.h"

#include "settings.h"

#define DRAW_PLAY_POSITION_TIME 75


FXDEFMAP(CSoundWindow) CSoundWindowMap[]=
{
	//	  Message_Type			ID						Message_Handler
	
		// invoked when one of the check buttons down the left side is changed
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_MUTE_BUTTON,			CSoundWindow::onMuteButton),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_INVERT_MUTE_BUTTON,		CSoundWindow::onInvertMuteButton),
	FXMAPFUNC(SEL_RIGHTBUTTONRELEASE,	CSoundWindow::ID_INVERT_MUTE_BUTTON,		CSoundWindow::onInvertMuteButton),

		// invoked when horz zoom dial is changed
	FXMAPFUNC(SEL_CHANGED,			CSoundWindow::ID_HORZ_ZOOM_DIAL,		CSoundWindow::onHorzZoomDialChange),
		// events to quickly zoom all the way in or out
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_HORZ_ZOOM_DIAL_PLUS,		CSoundWindow::onHorzZoomDialPlusIndClick),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_HORZ_ZOOM_DIAL_MINUS,		CSoundWindow::onHorzZoomDialMinusIndClick),

		// invoked when vert zoom dial is changed
	FXMAPFUNC(SEL_CHANGED,			CSoundWindow::ID_VERT_ZOOM_DIAL,		CSoundWindow::onVertZoomDialChange),
		// events to quickly zoom all the way in or out
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_VERT_ZOOM_DIAL_PLUS,		CSoundWindow::onVertZoomDialPlusIndClick),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_VERT_ZOOM_DIAL_MINUS,		CSoundWindow::onVertZoomDialMinusIndClick),

	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_BOTH_ZOOM_DIAL_MINUS,		CSoundWindow::onBothZoomDialMinusIndClick),

		// timer event to draw the play status position
	FXMAPFUNC(SEL_TIMEOUT,			CSoundWindow::ID_DRAW_PLAY_POSITION,		CSoundWindow::onDrawPlayPosition),


	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_SELECT_START_SPINNER,		CSoundWindow::onSelectStartSpinnerChange),
	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_SELECT_STOP_SPINNER,		CSoundWindow::onSelectStopSpinnerChange),


	FXMAPFUNC(FXRezWaveView::SEL_SELECTION_CHANGED,CSoundWindow::ID_WAVEVIEW,		CSoundWindow::onSelectionChange),

		// cue handlers
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE,CSoundWindow::ID_WAVEVIEW,				CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE_AT_STOP_POSITION,CSoundWindow::ID_WAVEVIEW,	CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_ADD_CUE_AT_START_POSITION,CSoundWindow::ID_WAVEVIEW,	CSoundWindow::onAddCue),
	FXMAPFUNC(FXRezWaveView::SEL_REMOVE_CUE,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onRemoveCue),
	FXMAPFUNC(FXRezWaveView::SEL_EDIT_CUE,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onEditCue),
	FXMAPFUNC(FXRezWaveView::SEL_SHOW_CUE_LIST,CSoundWindow::ID_WAVEVIEW,			CSoundWindow::onShowCueList),

	FXMAPFUNC(SEL_COMMAND,			CSoundWindow::ID_ACTIVE_TOGGLE_BUTTON,		CSoundWindow::onActiveToggleButton),

	FXMAPFUNC(SEL_CLOSE,			0,						CSoundWindow::onCloseWindow),


	//FXMAPFUNC(SEL_CONFIGURE,		CSoundWindow::ID_THIS,			CSoundWindow::onResize),

};

FXIMPLEMENT(CSoundWindow,FXTopWindow,CSoundWindowMap,ARRAYNUMBER(CSoundWindowMap))


void playTrigger(void *Pthis)
{
	//printf("play trigger\n");
	CSoundWindow *that=(CSoundWindow *)Pthis;

	if(that->loadedSound->channel->isPlaying())
		that->playingLED->getFirst()->raise();
	else
		that->playingLED->getFirst()->lower();
	
	if(that->loadedSound->channel->isPlaying())
		// start the play status timer
		that->timerHandle=that->getApp()->addTimeout(DRAW_PLAY_POSITION_TIME,that,CSoundWindow::ID_DRAW_PLAY_POSITION);
}

void pauseTrigger(void *Pthis)
{
	//printf("pause trigger\n");
	CSoundWindow *that=(CSoundWindow *)Pthis;
	if(that->loadedSound->channel->isPaused())
		that->pausedLED->getFirst()->raise();
	else
		that->pausedLED->getFirst()->lower();
}



// ----------------------------------------------------------

CSoundWindow::CSoundWindow(FXWindow *mainWindow,CLoadedSound *_loadedSound) :
	FXTopWindow(mainWindow,_loadedSound->getSound()->getFilename().c_str(),NULL,NULL,DECOR_ALL, 95,mainWindow->getY()+mainWindow->getDefaultHeight()+45,650,400, 0,0,0,0, 0,0),

	loadedSound(_loadedSound),
	timerHandle(NULL),
	closing(false),

	prevW(-1),
	prevH(-1),

	activeToggleButton(gFocusMethod==fmFocusButton ? new FXToggleButton(this,"Not Active","Active",NULL,NULL,this,ID_ACTIVE_TOGGLE_BUTTON,TOGGLEBUTTON_NORMAL | LAYOUT_FILL_X|LAYOUT_SIDE_TOP, 0,0,0,0, 1,1,1,1) : NULL),

	statusPanel(new FXHorizontalFrame(this,FRAME_RIDGE | LAYOUT_SIDE_BOTTOM | LAYOUT_FILL_X, 0,0,0,0, 2,2,3,3, 0,0)),

	waveViewPanel(new FXPacker(this,FRAME_NONE | LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		horzZoomPanel(new FXPacker(waveViewPanel,LAYOUT_SIDE_BOTTOM | FRAME_RAISED | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,17, 1,1,1,1, 1,1)),
			horzZoomMinusInd(new FXButton(horzZoomPanel," - \tZoom Out Full",NULL,this,ID_HORZ_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			horzZoomDial(new FXDial(horzZoomPanel,this,ID_HORZ_ZOOM_DIAL,LAYOUT_SIDE_LEFT | DIAL_HORIZONTAL|DIAL_HAS_NOTCH | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0,0,150,0, 0,0,0,0)),
			horzZoomPlusInd(new FXButton(horzZoomPanel," + \tZoom In Full",NULL,this,ID_HORZ_ZOOM_DIAL_PLUS,FRAME_RAISED | LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			horzZoomValueLabel(new FXLabel(horzZoomPanel,"",NULL,LAYOUT_SIDE_LEFT | LAYOUT_FILL_Y)),
			bothZoomMinusButton(new FXButton(horzZoomPanel," -- \tHorizontally and Vertically Zoom Out Full",NULL,this,ID_BOTH_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_RIGHT | LAYOUT_FILL_Y)),
		vertZoomPanel(new FXPacker(waveViewPanel,LAYOUT_SIDE_RIGHT | FRAME_RAISED | LAYOUT_FILL_Y | LAYOUT_FIX_WIDTH, 0,0,17,0, 1,1,1,1, 1,1)),
			vertZoomPlusInd(new FXButton(vertZoomPanel,"+\tZoom In Full",NULL,this,ID_VERT_ZOOM_DIAL_PLUS,FRAME_RAISED | LAYOUT_SIDE_TOP | LAYOUT_FILL_X)),
			vertZoomDial(new FXDial(vertZoomPanel,this,LAYOUT_SIDE_TOP | ID_VERT_ZOOM_DIAL,DIAL_VERTICAL|DIAL_HAS_NOTCH | LAYOUT_FILL_X | LAYOUT_FIX_HEIGHT, 0,0,0,100, 0,0,0,0)),
			vertZoomMinusInd(new FXButton(vertZoomPanel,"-\tZoom Out Full",NULL,this,ID_VERT_ZOOM_DIAL_MINUS,FRAME_RAISED | LAYOUT_SIDE_TOP | LAYOUT_FILL_X)),
		mutePanel(loadedSound->getSound()->getChannelCount()<=1 ? NULL : new FXPacker(waveViewPanel,LAYOUT_SIDE_LEFT | FRAME_RAISED | LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
			upperMuteLabel(mutePanel==NULL ? NULL : new FXLabel(mutePanel,"M\tMute Channels",NULL,LABEL_NORMAL|LAYOUT_SIDE_TOP|LAYOUT_FIX_HEIGHT)),
			invertMuteButton(mutePanel==NULL ? NULL : new FXButton(mutePanel,"!\tInvert the Muted State of Each Channel or (right click) Unmute All Channels",NULL,this,ID_INVERT_MUTE_BUTTON,LAYOUT_FILL_X | FRAME_RAISED|FRAME_THICK | LAYOUT_SIDE_BOTTOM|LAYOUT_FIX_HEIGHT)),
			muteContents(mutePanel==NULL ? NULL : new FXVerticalFrame(mutePanel,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 0,0)),
		waveView(new FXRezWaveView(waveViewPanel,_loadedSound)),

	statusFont(getApp()->getNormalFont()),

	addCueActionFactory(NULL),
	removeCueActionFactory(NULL)
{
	waveView->setTarget(this);
	waveView->setSelector(ID_WAVEVIEW);

	if(activeToggleButton!=NULL)
	{
		activeToggleButton->setState(false);
		//activeToggleButton->setFocusRectangleStyle to nothing...
	}

	if(mutePanel!=NULL)
	{
		for(unsigned t=0;t<loadedSound->getSound()->getChannelCount();t++)
			muteButtons[t]=new FXCheckButton(muteContents,"",this,ID_MUTE_BUTTON,CHECKBUTTON_NORMAL|LAYOUT_CENTER_Y);
	}

	
	// create the font to use for status information
        FXFontDesc d;
        statusFont->getFontDesc(d);
        d.size-=10;
        statusFont=new FXFont(getApp(),d);


	// fill the status panel
	// ??? If I really wanted to make sure there was a min width to each section (and I may want to) I could put an FXFrame with zero height or 1 height in each on with a fixed width.. then things can get bigger as needed... but there would be a minimum size
	FXVerticalFrame *t;

	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,0,0, 0,8,0,0, 0,0);
		// add the actual LEDs to the packer for turning the LED on and off, done by raising or lowering the first child of the packer
		playingLED=new FXPacker(t,0, 0,0,0,0, 0,0,0,0);
			(new FXLabel(playingLED,"Playing:",new FXGIFIcon(getApp(),GreenLED1),JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
			(new FXLabel(playingLED,"Playing:",new FXGIFIcon(getApp(),OffLED1),JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
		pausedLED=new FXPacker(t,0, 0,0,0,0, 0,0,0,0);
			(new FXLabel(pausedLED,"Paused:",new FXGIFIcon(getApp(),YellowLED1),JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);
			(new FXLabel(pausedLED,"Paused:",new FXGIFIcon(getApp(),OffLED1),JUSTIFY_NORMAL | ICON_AFTER_TEXT | LAYOUT_FIX_X|LAYOUT_FIX_Y, 0,0))->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,200,0, 0,8,0,0, 0,0);
		audioDataSizeLabel=new FXLabel(t,"Audio Size; ",NULL,LAYOUT_RIGHT);
		audioDataSizeLabel->setFont(statusFont);
		poolFileSizeLabel=new FXLabel(t,"File Size: ",NULL,LAYOUT_RIGHT);
		poolFileSizeLabel->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,200,0, 0,8,0,0, 0,0);
		totalLengthLabel=new FXLabel(t,"Total: ",NULL,LAYOUT_RIGHT);
		totalLengthLabel->setFont(statusFont);
		selectionLengthLabel=new FXLabel(t,"Selection: ",NULL,LAYOUT_RIGHT);
		selectionLengthLabel->setFont(statusFont);
		
	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,0,0, 2,0,0,0, 0,0);
		selectStartSpinner=new FXSpinner(t,0,this,ID_SELECT_START_SPINNER, SPIN_NOTEXT);
		selectStartSpinner->setRange(-10,10);
		selectStartSpinner->setTipText("Increase/Decrease Start Position by One Sample");
		selectStopSpinner=new FXSpinner(t,0,this,ID_SELECT_STOP_SPINNER, SPIN_NOTEXT);
		selectStopSpinner->setRange(-10,10);
		selectStopSpinner->setTipText("Increase/Decrease Stop Position by One Sample");
		
	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,0,0, 0,8,0,0, 0,0);
		selectStartLabel=new FXLabel(t,"Start: ",NULL,LAYOUT_RIGHT);
		selectStartLabel->setFont(statusFont);
		selectStopLabel=new FXLabel(t,"Stop: ",NULL,LAYOUT_RIGHT);
		selectStopLabel->setFont(statusFont);

	new FXVerticalSeparator(statusPanel);
	t=new FXVerticalFrame(statusPanel,FRAME_NONE, 0,0,0,0, 2,0,0,0, 0,0);
		playPositionLabel=new FXLabel(t,"Playing: ",NULL,LAYOUT_LEFT);
		playPositionLabel->setFont(statusFont);

	// register to know when the sound start/stops and pauses/unpauses playing
	loadedSound->channel->addOnPlayTrigger(playTrigger,this);
	loadedSound->channel->addOnPauseTrigger(pauseTrigger,this);

	// set the ranges of the vertical and horiztonal zoom dials
	horzZoomDial->setRange(0,100);
	horzZoomDial->setRevolutionIncrement(100);
	vertZoomDial->setRange(0,100);
	vertZoomDial->setRevolutionIncrement(100);

	addCueActionFactory=new CAddCueActionFactory(gCueDialog);
	removeCueActionFactory=new CRemoveCueActionFactory;
	replaceCueActionFactory=new CReplaceCueActionFactory(gCueDialog);
}

CSoundWindow::~CSoundWindow()
{
	if(timerHandle!=NULL)
		getApp()->removeTimeout(timerHandle);

	/* looks like it wasn't going to fire after destruction has started
	closing=true;
	while(timerHandle!=NULL)
	{
		printf("waiting on timerHandle to fire\n");
		fxsleep(1000); // sleep for 1 millisecond
	}
	*/

	loadedSound->channel->removeOnPlayTrigger(playTrigger,this);
	loadedSound->channel->removeOnPauseTrigger(pauseTrigger,this);

	delete addCueActionFactory;
	delete removeCueActionFactory;

	// ??? do I need to delete the stuff I allocated

	delete statusFont;
}

void CSoundWindow::setActiveState(bool isActive)
{
	if(gFocusMethod==fmFocusButton)
	{
		activeToggleButton->setBackColor(isActive ? FXRGB(230,230,230) : FXRGB(25,25,25));
		activeToggleButton->setTextColor(isActive ? FXRGB(0,0,0) : FXRGB(255,255,255));
		activeToggleButton->setFrameStyle(isActive ? (FRAME_THICK|FRAME_RAISED) : (FRAME_THICK|FRAME_SUNKEN));
		activeToggleButton->setState(isActive);

		if(isActive)
		{
			gSoundFileManager->untoggleActiveForAllSoundWindows(this);
			raise();
		}
	}
	else if(gFocusMethod==fmSoundWindowList)
	{
		if(isActive)
		{
			show();
			gSoundFileManager->untoggleActiveForAllSoundWindows(this);
		}
		else
			hide();
	}
}

bool CSoundWindow::getActiveState() const
{
	if(gFocusMethod==fmFocusButton)
		return(activeToggleButton->getState() ? true : false);
	else if(gFocusMethod==fmSoundWindowList)
		return(shown());
	else
		throw(runtime_error(string(__func__)+" -- unhandle gFocusMethod: "+istring(gFocusMethod)));
}

/*
void CSoundWindow::show()
{
	FXTopWindow::show();
	//setDecorations(DECOR_NONE);
	//setDecorations(DECOR_ALL);
	printf("show\n");
}
*/

void CSoundWindow::drawPlayPosition(bool justErasing)
{
	if(!justErasing && !loadedSound->channel->isPlaying())
		return;

	const sample_pos_t length=loadedSound->channel->isPlayingSelectionOnly() ? (loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()) : loadedSound->channel->sound->getLength();
	const size_t sampleRate=loadedSound->channel->sound->getSampleRate();

	// quarter second minimum to draw the play status
	if(length>=sampleRate || length*4/sampleRate>=1 || loadedSound->channel->isPaused())
		waveView->drawPlayPosition(loadedSound->channel->getPosition(),justErasing);
	else
		waveView->drawPlayPosition(loadedSound->channel->getPosition(),true);
}

/*
// recursively call update for all descendants of a given window
void updateAll(FXWindow *w)
{
	w->update();
	for(int t=0;t<w->numChildren();t++)
		updateAll(w->childAtIndex(t));
}
*/

void CSoundWindow::updateFromEdit()
{
	waveView->updateFromEdit();
	onHorzZoomDialChange(NULL,0,NULL);
	//onVertZoomDialChange(NULL,0,NULL);
	updateAllStatusInfo();
	//updateAll(this);
	//layout();
}

void CSoundWindow::updateAllStatusInfo()
{
	audioDataSizeLabel->setText(("Audio Size: "+loadedSound->getSound()->getAudioDataSize()).c_str());
	poolFileSizeLabel->setText(("File Size: "+loadedSound->getSound()->getPoolFileSize()).c_str());

	// ??? this should depend on the FXRezWaveView's actual horzZoomFactor value because the FXDial doesn't represent how many samples a pixel represents
	int places;
	if(horzZoomDial->getValue()>90)
		places=5;
	else if(horzZoomDial->getValue()>80)
		places=4;
	else if(horzZoomDial->getValue()>60)
		places=3;
	else
		places=2;

	totalLengthLabel->setText(("Total: "+loadedSound->getSound()->getTimePosition(loadedSound->getSound()->getLength(),places)).c_str());

	updateSelectionStatusInfo();
	updatePlayPositionStatusInfo();
}

void CSoundWindow::updateSelectionStatusInfo()
{
	// ??? this should depend on the FXRezWaveView's actual horzZoomFactor value because the FXDial doesn't represent how many samples a pixel represents
	int places;
	if(horzZoomDial->getValue()>90)
		places=5;
	else if(horzZoomDial->getValue()>80)
		places=4;
	else if(horzZoomDial->getValue()>60)
		places=3;
	else 
		places=2;

	selectionLengthLabel->setText(("Selection: "+loadedSound->getSound()->getTimePosition(loadedSound->channel->getStopPosition()-loadedSound->channel->getStartPosition()+1,places)).c_str());
	selectStartLabel->setText(("Start: "+loadedSound->getSound()->getTimePosition(loadedSound->channel->getStartPosition(),places)).c_str());
	selectStopLabel->setText(("Stop: "+loadedSound->getSound()->getTimePosition(loadedSound->channel->getStopPosition(),places)).c_str());
}

void CSoundWindow::updatePlayPositionStatusInfo()
{
	int places;
	if(horzZoomDial->getValue()>90)
		places=5;
	else if(horzZoomDial->getValue()>80)
		places=4;
	else if(horzZoomDial->getValue()>60)
		places=3;
	else
		places=2;

	if(loadedSound->channel->isPlaying())
		playPositionLabel->setText(("Playing: "+loadedSound->getSound()->getTimePosition(loadedSound->channel->getPosition(),places)).c_str());
	else
		playPositionLabel->setText("");
}


// events I get from overriding a method

void CSoundWindow::create()
{
	FXTopWindow::create();

	//show(PLACEMENT_VISIBLE);

	#define OFFSET_AMOUNT 35
	
	// each time a window opens, offset it a little
	static FXint offset=-OFFSET_AMOUNT;
	offset=(offset+OFFSET_AMOUNT)%(OFFSET_AMOUNT*10);
	move(getX()+offset,getY()+offset);

	updateAllStatusInfo();

	if(mutePanel!=NULL)
	{
		// set the proper height of the labels above and below the panel with the mute checkboxes in it
		int top,height;
		waveView->getWaveSize(top,height);
		upperMuteLabel->setHeight(top);
		invertMuteButton->setHeight(waveView->getHeight()-(height+top)-2);
	}
}

void CSoundWindow::layout()
{
	FXTopWindow::layout();

	// ??? however, I could try using the SEL_CONFIGURE event for this (except it only happens on resize and not on startup)
	//I do this because on resize the max horzZoomFactor changes, perhaps in FXRezWaveViewScrollArea::layout I should adjust the zoomFactor by the ratio of the old and the new size if I can get the old and new width and height
	if(prevW!=width || prevH!=height)
	{
		prevW=width;
		prevH=height;
		onHorzZoomDialChange(NULL,0,NULL);
		onVertZoomDialChange(NULL,0,NULL);
	}

}

long CSoundWindow::onResize(FXObject *sender,FXSelector,void*)
{
	return(0);
}

/*
void CSoundWindow::raise()
{
	FXTopWindow::raise();
	printf("raised\n");
}
*/

// --- event handlers I setup  --------------------------------------------

// mute button
long CSoundWindow::onMuteButton(FXObject *sender,FXSelector,void*)
{
	for(unsigned t=0;t<loadedSound->getSound()->getChannelCount();t++)
		loadedSound->channel->setMute(t,muteButtons[t]->getCheck());

	return 1;
}

long CSoundWindow::onInvertMuteButton(FXObject *sender,FXSelector sel,void *ptr)
{
	if(SELTYPE(sel)==SEL_COMMAND)
	{
		for(unsigned t=0;t<loadedSound->getSound()->getChannelCount();t++)
			muteButtons[t]->setCheck(!muteButtons[t]->getCheck());
	}
	else
	{
		if(!invertMuteButton->underCursor())
			return(0);

		for(unsigned t=0;t<loadedSound->getSound()->getChannelCount();t++)
			muteButtons[t]->setCheck(false);
	}

	onMuteButton(sender,sel,ptr);

	return 1;
}


// little red play position bar draw event ... this method is called on a timer every fraction of a sec
long CSoundWindow::onDrawPlayPosition(FXObject *sender,FXSelector,void*)
{
	// timer has fired
	timerHandle=NULL;

	if(closing) // destructor is waiting on this event
		return 1;

	updatePlayPositionStatusInfo();

	if(loadedSound->channel->isPlaying())
	{
		drawPlayPosition(false);

		// ??? I could make the calculation of the next event more intelligent
		// 	- if the onscreen data is smaller I could register to get the timer faster 
		// reregister to get this event again
		timerHandle=getApp()->addTimeout(DRAW_PLAY_POSITION_TIME,this,ID_DRAW_PLAY_POSITION);
	}
	else
	{
		// erase the play position line and don't register to get the timer again
		drawPlayPosition(true);
	}

	return 1;
}


// horz zoom handlers

long CSoundWindow::onHorzZoomDialChange(FXObject *sender ,FXSelector sel,void *ptr)
{
	// map (v)0..100 ---> (z)1.0..max_horz_zoom_factor
	// that is the zoom dial's range ---> the zoom factor's range
	// except, we read the zoom dial's value backwards since we can't ...->setRange(100,0);
	double v=100.0-(double)horzZoomDial->getValue();

	// now apply a curve to v, because linear 0.0..100.0 jams most of the useful zoom factors near 0
	// I apply f(v)=(v^3)/(100^2)  which makes f(0)==0 and f(100)==100 but chages the linear curve to a parametric curve having more values near 0.0
	v=(v*v*v)/(100.0*100.0);

	double z=1.0+(((waveView->getMaxHorzZoomFactor()-1.0)*v)/100.0);

	FXint dummy;
	FXuint keyboardModifierState;
	horzZoomDial->getCursorPosition(dummy,dummy,keyboardModifierState);
	waveView->setHorzZoomFactor(z,keyboardModifierState);

	horzZoomValueLabel->setText(("  "+istring(horzZoomDial->getValue())+"%").c_str());

	if(sender!=NULL)
		waveView->redraw();

	updateAllStatusInfo();

	return 1;
}

long CSoundWindow::onHorzZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomDial->setValue(100);
	onHorzZoomDialChange(NULL,0,NULL);
	waveView->redraw();

	return 1;
}

long CSoundWindow::onHorzZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomDial->setValue(0);
	onHorzZoomDialChange(NULL,0,NULL);
	waveView->redraw();

	return 1;
}



// vert zoom handlers
long CSoundWindow::onVertZoomDialChange(FXObject *sender,FXSelector,void*)
{
	// map (v)0..100 ---> (z)1.0..max_vert_zoom_factor
	// that is the zoom dial's range ---> the zoom factor's range
	// except, we read the zoom dial's value backwards since we can't ...->setRange(100,0);
	double v=100.0-(double)vertZoomDial->getValue();

	// now apply a curve to v, because linear 0.0..100.0 jams most of the useful zoom factors near 0
	// I apply f(v)=(v^3)/(100^2)  which makes f(0)==0 and f(100)==100 but chages the linear curve to a parametric curve having more values near 0.0
	v=(v*v*v)/(100.0*100.0);

	double z=1.0+(((waveView->getMaxVertZoomFactor()-1.0)*v)/100.0);
	waveView->setVertZoomFactor(z);

	if(sender!=NULL)
		waveView->redraw();

	updateAllStatusInfo();

	return 1;
}

long CSoundWindow::onVertZoomDialPlusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	vertZoomDial->setValue(100);
	onVertZoomDialChange(NULL,0,NULL);
	waveView->redraw();
	return 1;
}

long CSoundWindow::onVertZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	vertZoomDial->setValue(0);
	onVertZoomDialChange(NULL,0,NULL);
	waveView->redraw();
	return 1;
}


long CSoundWindow::onBothZoomDialMinusIndClick(FXObject *sender,FXSelector sel,void *ptr)
{
	horzZoomDial->setValue(0);
	onHorzZoomDialChange(NULL,0,NULL);

	vertZoomDial->setValue(0);
	onVertZoomDialChange(NULL,0,NULL);

	waveView->redraw();
	return 1;
}



// selection position spinners
long CSoundWindow::onSelectStartSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	sample_pos_t newSelectStart=loadedSound->channel->getStartPosition()+selectStartSpinner->getValue();


	if(newSelectStart>=loadedSound->getSound()->getLength()-1)
		newSelectStart=loadedSound->getSound()->getLength()-1;
	if(newSelectStart<0)
		newSelectStart=0;

	loadedSound->channel->setStartPosition(newSelectStart);
	selectStartSpinner->setValue(0);
	waveView->updateFromSelectionChange(FXRezWaveView::lcpStart);
	return 1;
}

long CSoundWindow::onSelectStopSpinnerChange(FXObject *sender,FXSelector sel,void *ptr)
{
	sample_pos_t newSelectStop=loadedSound->channel->getStopPosition()+selectStopSpinner->getValue();

	if(newSelectStop>=loadedSound->getSound()->getLength()-1)
		newSelectStop=loadedSound->getSound()->getLength()-1;
	if(newSelectStop<0)
		newSelectStop=0;

	loadedSound->channel->setStopPosition(newSelectStop);
	selectStopSpinner->setValue(0);
	waveView->updateFromSelectionChange(FXRezWaveView::lcpStop);
	return 1;
}

long CSoundWindow::onSelectionChange(FXObject *sender,FXSelector sel,void *ptr)
{
	updateSelectionStatusInfo();
	return 1;
}

long CSoundWindow::onAddCue(FXObject *sender,FXSelector sel,void *ptr)
{
	try
	{
		CActionParameters actionParameters;

		// add the parameters for the dialog to display initially
		actionParameters.addStringParameter("Cue1");

		switch(SELTYPE(sel))
		{
		case FXRezWaveView::SEL_ADD_CUE:
			actionParameters.addSamplePosParameter(*((sample_pos_t *)ptr));
			break;

		case FXRezWaveView::SEL_ADD_CUE_AT_START_POSITION:
			actionParameters.addSamplePosParameter(loadedSound->channel->getStartPosition());
			break;

		case FXRezWaveView::SEL_ADD_CUE_AT_STOP_POSITION:
			actionParameters.addSamplePosParameter(loadedSound->channel->getStopPosition());
			break;
		}

		actionParameters.addBoolParameter(false);

		addCueActionFactory->performAction(loadedSound,&actionParameters,false);
	}
	catch(exception &e)
	{
		Error(e.what());
	}

	return 1;
}

long CSoundWindow::onRemoveCue(FXObject *sender,FXSelector sel,void *ptr)
{
	CActionParameters actionParameters;
	actionParameters.addUnsignedParameter(*((size_t *)ptr));
	removeCueActionFactory->performAction(loadedSound,&actionParameters);
	return 1;
}

long CSoundWindow::onEditCue(FXObject *sender,FXSelector sel,void *ptr)
{
	CActionParameters actionParameters;
	size_t cueIndex=*((size_t *)ptr);

	// add the parameters for the dialog to display initially
	actionParameters.addUnsignedParameter(cueIndex);
	actionParameters.addStringParameter(loadedSound->getSound()->getCueName(cueIndex));
	actionParameters.addSamplePosParameter(loadedSound->getSound()->getCueTime(cueIndex));
	actionParameters.addBoolParameter(loadedSound->getSound()->isCueAnchored(cueIndex));

	replaceCueActionFactory->performAction(loadedSound,&actionParameters);
	return 1;
}

long CSoundWindow::onShowCueList(FXObject *sender,FXSelector sel,void *ptr)
{
	gCueListDialog->setLoadedSound(loadedSound);
	gCueListDialog->execute(PLACEMENT_CURSOR);
	return 1;
}


// focusing handlers
long CSoundWindow::onActiveToggleButton(FXObject *sender,FXSelector sel,void *ptr)
{
	setActiveState(true);
	activeToggleButton->killFocus();
	return 1;
}

#include "../backend/file.h"
long CSoundWindow::onCloseWindow(FXObject *sender,FXSelector sel,void *ptr)
{
	gSoundFileManager->close(ASoundFileManager::ctSaveYesNoCancel,loadedSound);
	return 1;
}
