/*
 *  milkyplay/LoaderSTM.cpp
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
 *  LoaderSTM.cpp
 *  MilkyPlay Module Loader: ScreamTracker 2
 *
 * Warning: This is an one-by-one conversion of an assembler version ;)
 *
 */
#include "Loaders.h"

const char* LoaderSTM::identifyModule(const mp_ubyte* buffer)
{
	// check for .STM module
	if (!memcmp(buffer+20,"!Scream!",8) ||
		!memcmp(buffer+20,"BMOD2STM",8)) 
	{
		return "STM";
	}

	// this is not an .STM
	return NULL;
}

mp_sint32 LoaderSTM::load(XMFileBase& f, XModule* module)
{
	mp_sint32 i,j;
	mp_ubyte buffer[128];
	mp_uint32 samplePtrs[31];

	module->cleanUp();

	// this will make code much easier to read
	TXMHeader*		header = &module->header;
	TXMInstrument*	instr  = module->instr;
	TXMSample*		smp	   = module->smp;
	TXMPattern*		phead  = module->phead;	
	
	// we're already out of memory here
	if (!phead || !instr || !smp)
		return -7;	
	
	f.read(buffer,1,48);
	
	memcpy(header->name,buffer,20);

	memcpy(header->sig,buffer+20,8);

	header->tempo = buffer[32]>>4;	// default tickspeed
	header->speed = 124;				// default tempo
	
	header->patnum = buffer[33]; // number of patterns

	header->mainvol = XModule::vol64to255(buffer[34]);

	header->flags = module->MODULE_OLDS3MVOLSLIDES | module->MODULE_STMARPEGGIO;
	
	header->insnum = 31;			// number of instruments
	//header->smpnum = 31;			// number of samples
	header->channum = 4;
	
	mp_uint32 s = 0;
	for (i = 0; i < header->insnum; i++) 
	{
		f.read(buffer,1,32);

		memcpy(instr[i].name,buffer,12);
		
		mp_uint32 size = LittleEndian::GET_WORD(buffer+16);		
		mp_uint32 loopstart = LittleEndian::GET_WORD(buffer+18);
		mp_uint32 loopend = LittleEndian::GET_WORD(buffer+20);
		mp_ubyte volume = XModule::vol64to255(buffer[22]);

		// fix for janis.stm
		if (size < 32) 
		{
			instr[i].samp = 0;
		}
		else
		{
			memcpy(smp[s].name,buffer,12);
			
			samplePtrs[s] = (mp_uint32)LittleEndian::GET_WORD(buffer+14)<<4;

			XModule::convertc4spd((LittleEndian::GET_WORD(buffer+24)*8363)/8448,&smp[s].finetune,&smp[s].relnote);
			
			smp[s].flags = 1;
			smp[s].samplen = size;
			smp[s].loopstart = loopstart;
			smp[s].looplen = loopend;
			smp[s].vol = volume;
		
			instr[i].samp = 1;
			for (mp_sint32 j = 0; j < 120; j++) 
				instr[i].snum[j] = s;
			
			if (smp[s].looplen < 0xFFFF && smp[s].looplen != 0)
			{
				if (smp[s].looplen > smp[s].samplen)
					smp[s].looplen = smp[s].samplen;

				if ((smp[s].loopstart+smp[s].looplen)>smp[s].samplen)
					smp[s].looplen-=(smp[s].loopstart+smp[s].looplen)-smp[s].samplen;
				
				smp[s].type=1;
			}
			else
				smp[s].looplen = 0;
		
			s++;
		}
		
	}
	
	header->smpnum = s;

	f.read(buffer,1,128);
	
	j = 0;
	for (i = 0; i < 128; i++)
	{
		if (buffer[i] == 'c')
			break;
		else
			header->ord[j++] = buffer[i]; 
	}

	header->ordnum = j;

	mp_ubyte* srcPattern = new mp_ubyte[64*4*4];
	if (srcPattern == NULL)
		return -7;

	for (i = 0; i < header->patnum;i++) 
	{
		f.read(srcPattern,1,64*4*4);
		
		phead[i].rows = 64;
		phead[i].effnum = 2;
		phead[i].channum = (mp_ubyte)header->channum;
		
		phead[i].patternData = new mp_ubyte[phead[i].rows*header->channum*6];
		
		// out of memory?
		if (phead[i].patternData == NULL)
		{
			delete[] srcPattern;
			return -7;
		}
		
		memset(phead[i].patternData,0,phead[i].rows*header->channum*6);
		
		mp_sint32 r,c,cnt = 0,offs = 0;
		for (r=0;r<64;r++) {
			for (c=0;c<header->channum;c++) {
				mp_ubyte note = srcPattern[offs];
				mp_ubyte ins = 0;
				mp_ubyte b1 = srcPattern[offs+1];
				mp_ubyte b2 = srcPattern[offs+2];
				mp_ubyte op = srcPattern[offs+3];
				
				mp_ubyte nEff = 0;
				mp_ubyte nOp = 0;

				if (note != 255) 
				{
					note = (((note>>4)+2)*12+(note&0xf))+1;
				}
				else 
					note = 0;
					
				ins = b1>>3;

				mp_ubyte vol = (b1&0x07)+((b2&0xF0)>>1);
				if (vol<=64)
				{
					phead[i].patternData[cnt+2] = 0x0C;
					phead[i].patternData[cnt+3] = XModule::vol64to255(vol);
				}

				switch (b2&0xf)
				{
					// Arpeggio
					case 0x00:
						if (op != 00)
						{
							nEff = 0x20;
							nOp = op;
						}
						break;

					// set tickspeed
					case 0x01:
						nEff = 0x1C;
						nOp = op>>4;
						break;

					// position jump
					case 0x02:
						nEff = 0x0B;
						nOp = op;

					// pattern break
					case 0x03:
						nEff = 0x0D;
						nOp = op;
						break;

					// s3m volslide
					case 0x04:
						nEff = 0x49;
						nOp = op;
						break;

					// porta down
					case 0x05:
						nEff = 0x48;
						nOp = op;
						break;
						
					// porta up
					case 0x06:
						nEff = 0x47;
						nOp = op;
						break;

					// porta to note
					case 0x07:
						nEff = 0x03;
						nOp = op;
						break;

					// vibrato
					case 0x08:
						nEff = 0x04;
						nOp = op;
						break;

					// tremor
					case 0x09:
						nEff = 0x1D;
						nOp = op;
						break;

					// s3m arpeggio (in combination with the STMARPEGGIO flag)
					case 0x0A:					
						nEff = 0x20;
						nOp = op;
						break;

					// vibrato + volume slide
					case 0x0B:
						nEff = 0x06;
						nOp = op;
						break;
					
					// tone porta + volume slide
					case 0x0C:
						nEff = 0x05;
						nOp = op;
						break;

					// tremolo
					case 0x0D:
						nEff = 0x07;
						nOp = op;
						break;

					// sample offset 
					case 0x0E:
					case 0x0F:
						nEff = 0x09;
						nOp = op;
						break;
				}

				phead[i].patternData[cnt]=note;
				phead[i].patternData[cnt+1]=ins;
				phead[i].patternData[cnt+4]=nEff;
				phead[i].patternData[cnt+5]=nOp;
				
				cnt+=6;

				offs+=4;
			}
		}
		
	}

	delete[] srcPattern;

	for (i = 0; i < header->smpnum; i++)
	{
		mp_uint32 smpOffs = samplePtrs[i];
		
		if (smpOffs)
		{
			f.seekWithBaseOffset(smpOffs);

			if (module->loadModuleSample(f, i) != 0)
				return -7;
		}
	}
	
	strcpy(header->tracker,"Screamtracker 2");
	
	module->setDefaultPanning();
	
	module->postProcessSamples();
	
	return 0;
}