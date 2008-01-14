/*
 *  tracker/RespondMessageBoxResample.h
 *
 *  Copyright 2008 Peter Barth
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  RespondMessageBoxResample.h
 *  MilkyTracker
 *
 *  Created by Peter Barth on 25.10.05.
 *
 */

#ifndef RESPONDMESSAGEBOXRESAMPLE__H
#define RESPONDMESSAGEBOXRESAMPLE__H

#include "RespondMessageBox.h"

class RespondMessageBoxResample : public RespondMessageBox
{
private:
	class PPListBox* listBoxes[3];
	pp_int32 currentSelectedListBox;
	pp_int32 relnote, finetune;
	pp_uint32 size, finalSize;
	pp_uint32 count;
	float c4spd;
	float originalc4spd;
	pp_int32 interpolationType;

public:
	RespondMessageBoxResample(PPScreen* screen, 
							  RespondListenerInterface* responder,
							  pp_int32 id);

	virtual void show();

	virtual pp_int32 handleEvent(PPObject* sender, PPEvent* event);	

	void setRelNote(pp_int32 note);
	void setFineTune(pp_int32 ft);
	void setC4Speed(float c4spd);
	void setSize(pp_uint32 size);
	
	float getC4Speed() const { return c4spd; }
	pp_int32 getRelNote() const { return relnote; }
	pp_int32 getFineTune() const { return finetune; }
	
	void setInterpolationType(pp_int32 interpolationType) { this->interpolationType = interpolationType; }
	pp_int32 getInterpolationType() const { return interpolationType; }
	
private:
	void listBoxEnterEditState(pp_int32 id);
	
	void updateListBoxes();
	void updateListBox(pp_int32 id, float val, pp_int32 numDecimals);
	
	void commitChanges();
	
	void switchListBox();

	void toC4Speed();
	void fromC4Speed();

	void calcSize();

	void validate();
};

#endif
