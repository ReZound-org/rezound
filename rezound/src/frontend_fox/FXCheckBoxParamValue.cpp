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

#include "FXCheckBoxParamValue.h"

#include <istring>

#include <CNestedDataFile/CNestedDataFile.h>
#define DOT (CNestedDataFile::delimChar)

/*
	- This is the text entry widget used over and over by ReZound on action dialogs
	- Its purpose is to select a boolean value for a parameter to an action
*/

FXDEFMAP(FXCheckBoxParamValue) FXCheckBoxParamValueMap[]=
{
	//Message_Type				ID					Message_Handler

	//FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_VALUE_SPINNER,	FXCheckBoxParamValue::onValueSpinnerChange),
	//FXMAPFUNC(SEL_COMMAND,			FXCheckBoxParamValue::ID_VALUE_TEXTBOX,	FXCheckBoxParamValue::onValueTextBoxChange),
};

FXIMPLEMENT(FXCheckBoxParamValue,FXVerticalFrame,FXCheckBoxParamValueMap,ARRAYNUMBER(FXCheckBoxParamValueMap))

FXCheckBoxParamValue::FXCheckBoxParamValue(FXComposite *p,int opts,const char *title,const bool checked) :
	FXVerticalFrame(p,opts|FRAME_RIDGE | LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0, 6,6,2,4, 2,0),

	checkBox(new FXCheckButton(this,title,NULL,0,CHECKBUTTON_NORMAL))
{
	checkBox->setCheck(checked);
}

FXCheckBoxParamValue::~FXCheckBoxParamValue()
{
}

const bool FXCheckBoxParamValue::getValue()
{
	return(checkBox->getCheck());
}

void FXCheckBoxParamValue::setValue(const bool checked)
{
	checkBox->setCheck(checked);
}

const string FXCheckBoxParamValue::getTitle() const
{
	return(checkBox->getText().text());
}

void FXCheckBoxParamValue::setTipText(const FXString &text)
{
	checkBox->setTipText(text);
}

FXString FXCheckBoxParamValue::getTipText() const
{
	return(checkBox->getTipText());	
}

void FXCheckBoxParamValue::readFromFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getTitle()+DOT+"value";
	if(f->keyExists(key.c_str()))
	{
		const bool v= f->getValue(key.c_str())=="true" ? true : false;
		setValue(v);
	}
	else
		setValue(false);
}

void FXCheckBoxParamValue::writeToFile(const string &prefix,CNestedDataFile *f)
{
	const string key=prefix+DOT+getTitle()+DOT;
	f->createKey((key+"value").c_str(),getValue() ? "true" : "false");
}


