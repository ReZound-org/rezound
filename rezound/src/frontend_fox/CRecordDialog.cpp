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

#include "CRecordDialog.h"

#include "../backend/ASoundRecorder.h"

#include <istring>

#define STATUS_UPDATE_TIME 100

CRecordDialog *gRecordDialog;

FXDEFMAP(CRecordDialog) CRecordDialogMap[]=
{
//	Message_Type			ID					Message_Handler
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_START_BUTTON,		CRecordDialog::onStartButton),
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_STOP_BUTTON,		CRecordDialog::onStopButton),
	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_REDO_BUTTON,		CRecordDialog::onRedoButton),

	FXMAPFUNC(SEL_TIMEOUT,		CRecordDialog::ID_STATUS_UPDATE,	CRecordDialog::onStatusUpdate),

	FXMAPFUNC(SEL_COMMAND,		CRecordDialog::ID_CLEAR_CLIP_COUNT_BUTTON,CRecordDialog::onClearClipCountButton),
};
		

FXIMPLEMENT(CRecordDialog,FXModalDialogBox,CRecordDialogMap,ARRAYNUMBER(CRecordDialogMap))



// ----------------------------------------

CRecordDialog::CRecordDialog(FXWindow *mainWindow) :
	FXModalDialogBox(mainWindow,"Record",350,300,FXModalDialogBox::ftVertical),

	recorder(NULL),
	showing(false),
	timerHandle(NULL)

{
	getFrame()->setHSpacing(1);
	getFrame()->setVSpacing(1);

	FXPacker *frame1;
	FXPacker *frame2;
	FXPacker *frame3;
	FXPacker *frame4;

	frame1=new FXHorizontalFrame(getFrame(),LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
		frame2=new FXVerticalFrame(frame1,LAYOUT_FILL_X|LAYOUT_FILL_Y, 0,0,0,0, 0,0,0,0, 1,1);
			frame3=new FXVerticalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				new FXButton(frame3,"Add Cue",NULL,this,ID_ADD_CUE_BUTTON);
				new FXButton(frame3,"Add Anchored Cue",NULL,this,ID_ADD_ANCHORED_CUE_BUTTON);
					// but what name shall I give the cues
					// the checkbox should indicate that cues will be placed at each time the sound is stopped and started
				//surroundWithCuesButton=new FXCheckButton(frame3,"Surround With Cues");
			frame3=new FXHorizontalFrame(frame2,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
				frame3->setHSpacing(0);
				frame3->setVSpacing(0);
				frame4=new FXVerticalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					new FXLabel(frame4,"Rec Length: ",NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT);
					new FXLabel(frame4,"Rec Size: ",NULL,LAYOUT_FILL_X|JUSTIFY_RIGHT);
				frame4=new FXVerticalFrame(frame3,LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 0,0);
					recordedLengthStatusLabel=new FXLabel(frame4,"00:00.000",NULL,LAYOUT_FILL_X|JUSTIFY_LEFT);
					recordedSizeStatusLabel=new FXLabel(frame4,"0",NULL,LAYOUT_FILL_X|JUSTIFY_LEFT);
		frame2=new FXVerticalFrame(frame1,FRAME_RAISED | LAYOUT_FILL_X|LAYOUT_FILL_Y);
			frame3=new FXHorizontalFrame(frame2);
				new FXButton(frame3,"Reset",NULL,this,ID_CLEAR_CLIP_COUNT_BUTTON);
				clipCountLabel=new FXLabel(frame3,"Clip Count: 0",NULL);
			meterFrame=new FXHorizontalFrame(frame2,LAYOUT_FILL_X|LAYOUT_FILL_Y);
	frame1=new FXVerticalFrame(getFrame(),FRAME_RAISED | LAYOUT_FILL_X, 0,0,0,0, 0,0,0,0, 1,1);
		frame2=new FXHorizontalFrame(frame1,LAYOUT_CENTER_X);
			new FXButton(frame2,"Record/\nResume",NULL,this,ID_START_BUTTON);
			new FXButton(frame2,"Stop/\nPause",NULL,this,ID_STOP_BUTTON);
			new FXButton(frame2,"Redo",NULL,this,ID_REDO_BUTTON);
		frame2=new FXHorizontalFrame(frame1,LAYOUT_CENTER_X);
			setDurationButton=new FXCheckButton(frame2,"Limit Duration to ");
			durationEdit=new FXTextField(frame2,12);
			durationEdit->setText("MM:SS.sss");
				
		
}

void CRecordDialog::cleanupMeters()
{
	while(!meters.empty())
	{
		delete meters[0];
		meters.erase(meters.begin());
	}
}

//??? include this to get colors.. but they should be in the settings registry actually
#include "drawPortion.h"
void CRecordDialog::setMeterValue(unsigned channel,float value)
{
	FXProgressBar *meter=meters[channel];
	if(value>=1.0)
		meter->setBarColor(clippedWaveformColor);
	else
		// need to make this more usefully be green for comfortable values, to yellow, to red at close to clipping
		meter->setBarColor(FXRGB((int)(value*255),(int)((1.0-value)*255),0));

	meter->setProgress((FXint)(value*1000));
}

long CRecordDialog::onStatusUpdate(FXObject *sender,FXSelector sel,void *ptr)
{
	if(!showing)
		return(0);

	for(unsigned i=0;i<meters.size();i++)
		setMeterValue(i,recorder->getAndResetLastPeakValue(i));

	clipCountLabel->setText(("Clip Count: "+istring(recorder->clipCount)).c_str());

	recordedLengthStatusLabel->setText(recorder->getRecordedLength().c_str());
	recordedSizeStatusLabel->setText(recorder->getRecordedSize().c_str());

	// schedule for the next status update
	timerHandle=getApp()->addTimeout(STATUS_UPDATE_TIME,this,CRecordDialog::ID_STATUS_UPDATE);
	return(1);
}

bool CRecordDialog::show(ASoundRecorder *_recorder)
{
	recorder=_recorder;
	clearClipCount();

	for(unsigned i=0;i<recorder->getChannelCount();i++)
	{
		FXProgressBar *meter=new FXProgressBar(meterFrame,NULL,0,PROGRESSBAR_NORMAL|PROGRESSBAR_VERTICAL | LAYOUT_FILL_Y);
		meter->setBarBGColor(FXRGB(0,0,0));
		meter->setTotal(1000);
		meters.push_back(meter);
	}
	meterFrame->recalc();

	onStopButton(NULL,0,NULL); // to clear status labels
	try
	{
		showing=true;
		timerHandle=getApp()->addTimeout(STATUS_UPDATE_TIME,this,CRecordDialog::ID_STATUS_UPDATE);

		if(FXDialogBox::execute(PLACEMENT_CURSOR))
		{
			// if nothing was ever recorded, I should return false

			showing=false;
			cleanupMeters();
			return(true);
		}
		showing=false;
		cleanupMeters();
		return(false);
	}
	catch(...)
	{
		cleanupMeters();
		throw;
	}
}

long CRecordDialog::onStartButton(FXObject *sender,FXSelector sel,void *ptr)
{
	clearClipCount();
	recorder->start();
	return 1;
}

long CRecordDialog::onStopButton(FXObject *sender,FXSelector sel,void *ptr)
{
	recorder->stop();

	return 1;
}

long CRecordDialog::onRedoButton(FXObject *sender,FXSelector sel,void *ptr)
{
	recorder->redo();
	clearClipCount();
	return 1;
}

long CRecordDialog::onAddCueButton(FXObject *sender,FXSelector sel,void *ptr)
{
	return 1;
}

void CRecordDialog::clearClipCount()
{
	recorder->clipCount=0;
	clipCountLabel->setText("Clip Count: 0");
}

long CRecordDialog::onClearClipCountButton(FXObject *sender,FXSelector sel,void *ptr)
{
	clearClipCount();
	return 1;
}

