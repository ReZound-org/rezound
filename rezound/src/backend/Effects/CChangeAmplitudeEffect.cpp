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

#include "CChangeAmplitudeEffect.h"

#include <stdexcept>

#include "../CActionSound.h"
#include "../CActionParameters.h"

CChangeAmplitudeEffect::CChangeAmplitudeEffect(const CActionSound &actionSound,const CGraphParamValueNodeList &_volumeCurve) :
	AAction(actionSound),
	volumeCurve(_volumeCurve)
{
}

bool CChangeAmplitudeEffect::doActionSizeSafe(CActionSound &actionSound,bool prepareForUndo)
{
	if(prepareForUndo)
		moveSelectionToTempPools(actionSound,mmSelection,actionSound.selectionLength());

	const sample_pos_t start=actionSound.start;
	const sample_pos_t selectionLength=actionSound.selectionLength();

	for(unsigned i=0;i<actionSound.sound->getChannelCount();i++)
	{
		if(actionSound.doChannel[i])
		{
			CStatusBar statusBar("Changing Amplitude -- Channel "+istring(i),0,selectionLength,true);

			sample_pos_t srcp=0;
			for(unsigned x=0;x<volumeCurve.size()-1;x++)
			{
				sample_pos_t segmentStartPosition,segmentStopPosition;
				double segmentStartValue,segmentStopValue;
				sample_pos_t segmentLength;

				interpretGraphNodes(volumeCurve,x,selectionLength,segmentStartPosition,segmentStartValue,segmentStopPosition,segmentStopValue,segmentLength);

				segmentStartPosition+=actionSound.start;
				segmentStopPosition+=actionSound.start;

				// ??? I could check if segmentStartValue and segmentStopValue were the same, if so, don't interpolate between them

				// ??? change to use the same implementation (for undoable or not-undoable), but just get srcAccesser based off of prepare for undo

				// either get the data from the undo pool or get it right from the audio pool if we didn't prepare for undo
				if(prepareForUndo)
				{
					const CRezPoolAccesser src=actionSound.sound->getTempAudio(tempAudioPoolKey,i);
					CRezPoolAccesser dest=actionSound.sound->getAudio(i);
					for(sample_pos_t t=0;t<segmentLength;t++)
					{
						float scalar=(float)(segmentStartValue+(((segmentStopValue-segmentStartValue)*(double)(t))/(segmentLength)));
						dest[t+segmentStartPosition]=ClipSample((mix_sample_t)(src[srcp++]*scalar));

						if(statusBar.update(srcp))
						{ // cancelled
							undoActionSizeSafe(actionSound);
							return false;
						}
					}
				}
				else
				{
					CRezPoolAccesser a=actionSound.sound->getAudio(i);
					for(sample_pos_t t=segmentStartPosition;t<=segmentStopPosition;t++)
					{
						float scalar=(float)(segmentStartValue+(((segmentStopValue-segmentStartValue)*(double)(t-segmentStartPosition))/(segmentLength)));
						a[t]=ClipSample((mix_sample_t)(a[t]*scalar));

						if(statusBar.update(t-start))
						{
							actionSound.sound->invalidatePeakData(i,actionSound.start,t);
							return false;
						}
					}
					actionSound.sound->invalidatePeakData(i,actionSound.start,actionSound.stop);
				}
			}
		}
	}

	return(true);
}

AAction::CanUndoResults CChangeAmplitudeEffect::canUndo(const CActionSound &actionSound) const
{
	return(curYes);
}

void CChangeAmplitudeEffect::undoActionSizeSafe(const CActionSound &actionSound)
{
	restoreSelectionFromTempPools(actionSound,actionSound.start,actionSound.selectionLength());
}





// ---------------------------------------------

CChangeVolumeEffectFactory::CChangeVolumeEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog) :
	AActionFactory("Change Volume","Change Volume",false,channelSelectDialog,normalDialog,NULL)
{
}

CChangeAmplitudeEffect *CChangeVolumeEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	if(actionParameters->getGraphParameter("Volume Change").size()<2)
		throw(runtime_error(string(__func__)+" -- graph parameter 0 contains less than 2 nodes"));

	return(new CChangeAmplitudeEffect(actionSound,actionParameters->getGraphParameter("Volume Change")));
}


// ---------------------------------------------

CGainEffectFactory::CGainEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *normalDialog,AActionDialog *advancedDialog) :
	AActionFactory("Gain","Gain",true,channelSelectDialog,normalDialog,advancedDialog)
{
}

CChangeAmplitudeEffect *CGainEffectFactory::manufactureAction(const CActionSound &actionSound,const CActionParameters *actionParameters,bool advancedMode) const
{
	string parameterName;
	if(actionParameters->containsParameter("Gain")) // it's just that the frontend uses two different names for the same parameter because the dialog is different
		parameterName="Gain";
	else
		parameterName="Gain Curve";

	if(actionParameters->getGraphParameter(parameterName).size()<2)
		throw(runtime_error(string(__func__)+" -- graph parameter 0 contains less than 2 nodes"));

		return(new CChangeAmplitudeEffect(actionSound,actionParameters->getGraphParameter(parameterName)));
}


