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

#ifndef __CChangeRateEffect_H__
#define __CChangeRateEffect_H__

#include "../../../config/common.h"

#include "../AAction.h"

#include "../CGraphParamValueNode.h"

class CChangeRateEffect : public AAction
{
public:
	CChangeRateEffect(const AActionFactory *factory,const CActionSound *actionSound,const CGraphParamValueNodeList &rateCurve);
	virtual ~CChangeRateEffect();

protected:
	bool doActionSizeSafe(CActionSound *actionSound,bool prepareForUndo);
	void undoActionSizeSafe(const CActionSound *actionSound);
	CanUndoResults canUndo(const CActionSound *actionSound) const;

private:
	sample_pos_t undoRemoveLength;
	CGraphParamValueNodeList rateCurve;

	sample_pos_t getNewSelectionLength(const CActionSound *actionSound) const;
	sample_fpos_t getWriteLength(sample_pos_t oldLength,const CGraphParamValueNode &rateNote1,const CGraphParamValueNode &rateNode2) const;
	    
};


class CSimpleChangeRateEffectFactory : public AActionFactory
{
public:
	CSimpleChangeRateEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CSimpleChangeRateEffectFactory();

	CChangeRateEffect *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

class CCurvedChangeRateEffectFactory : public AActionFactory
{
public:
	CCurvedChangeRateEffectFactory(AActionDialog *channelSelectDialog,AActionDialog *dialog);
	virtual ~CCurvedChangeRateEffectFactory();

	CChangeRateEffect *manufactureAction(const CActionSound *actionSound,const CActionParameters *actionParameters) const;
};

#endif
