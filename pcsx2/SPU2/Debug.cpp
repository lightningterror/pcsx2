/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2020  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "PrecompiledHeader.h"

#include "SPU2/Global.h"
#include "Config.h"

#include "common/FileSystem.h"

#include <cstdarg>

#ifdef PCSX2_DEVBUILD

static FILE* spu2Log = nullptr;

void SPU2::OpenFileLog()
{
	if (spu2Log)
		return;

	spu2Log = EmuFolders::OpenLogFile("SPU2Log.txt", "w");
	setvbuf(spu2Log, nullptr, _IONBF, 0);
}

void SPU2::CloseFileLog()
{
	if (!spu2Log)
		return;

	std::fclose(spu2Log);
	spu2Log = nullptr;
}

void SPU2::FileLog(const char* fmt, ...)
{
	if (!spu2Log)
		return;

	std::va_list ap;
	va_start(ap, fmt);
	std::vfprintf(spu2Log, fmt, ap);
	std::fflush(spu2Log);
	va_end(ap);
}

//Note to developer on the usage of ConLog:
//  while ConLog doesn't print anything if messages to console are disabled at the GUI,
//    it's still better to outright not call it on tight loop scenarios, by testing MsgToConsole() (which is inline and very quick).
//    Else, there's some (small) overhead in calling and returning from ConLog.
void SPU2::ConLog(const char* fmt, ...)
{
	if (!SPU2::MsgToConsole())
		return;

	std::va_list ap;
	va_start(ap, fmt);
	Console.FormatV(fmt, ap);
	va_end(ap);

	if (spu2Log)
	{
		va_start(ap, fmt);
		std::vfprintf(spu2Log, fmt, ap);
		std::fflush(spu2Log);
		va_end(ap);
	}
}

void V_VolumeSlide::DebugDump(FILE* dump, const char* title, const char* nameLR)
{
	fprintf(dump, "%s Volume for %s Channel:\t%x\n"
				  "  - Value:     %x\n"
				  "  - Mode:      %x\n"
				  "  - Increment: %x\n",
			title, nameLR, Reg_VOL, Value, Mode, Increment);
}

void V_VolumeSlideLR::DebugDump(FILE* dump, const char* title)
{
	Left.DebugDump(dump, title, "Left");
	Right.DebugDump(dump, title, "Right");
}

void V_VolumeLR::DebugDump(FILE* dump, const char* title)
{
	fprintf(dump, "Volume for %s (%s Channel):\t%x\n", title, "Left", Left);
	fprintf(dump, "Volume for %s (%s Channel):\t%x\n", title, "Right", Right);
}

void SPU2::DoFullDump()
{
	FILE* dump;

	if (SPU2::MemDump())
	{
		dump = EmuFolders::OpenLogFile("SPU2mem.dat", "wb");
		if (dump)
		{
			fwrite(_spu2mem, 0x200000, 1, dump);
			fclose(dump);
		}
	}
	if (SPU2::RegDump())
	{
		dump = EmuFolders::OpenLogFile("SPU2regs.dat", "wb");
		if (dump)
		{
			fwrite(spu2regs, 0x2000, 1, dump);
			fclose(dump);
		}
	}

	if (!SPU2::CoresDump())
		return;
	dump = EmuFolders::OpenLogFile("SPU2Cores.txt", "wt");
	if (dump)
	{
		for (u8 c = 0; c < 2; c++)
		{
			fprintf(dump, "#### CORE %d DUMP.\n", c);

			Cores[c].MasterVol.DebugDump(dump, "Master");

			Cores[c].ExtVol.DebugDump(dump, "External Data Input");
			Cores[c].InpVol.DebugDump(dump, "Voice Data Input [dry]");
			Cores[c].FxVol.DebugDump(dump, "Effects/Reverb [wet]");

			fprintf(dump, "Interrupt Address:          %x\n", Cores[c].IRQA);
			fprintf(dump, "DMA Transfer Start Address: %x\n", Cores[c].TSA);
			fprintf(dump, "External Input to Direct Output (Left):    %s\n", Cores[c].DryGate.ExtL ? "Yes" : "No");
			fprintf(dump, "External Input to Direct Output (Right):   %s\n", Cores[c].DryGate.ExtR ? "Yes" : "No");
			fprintf(dump, "External Input to Effects (Left):          %s\n", Cores[c].WetGate.ExtL ? "Yes" : "No");
			fprintf(dump, "External Input to Effects (Right):         %s\n", Cores[c].WetGate.ExtR ? "Yes" : "No");
			fprintf(dump, "Sound Data Input to Direct Output (Left):  %s\n", Cores[c].DryGate.SndL ? "Yes" : "No");
			fprintf(dump, "Sound Data Input to Direct Output (Right): %s\n", Cores[c].DryGate.SndR ? "Yes" : "No");
			fprintf(dump, "Sound Data Input to Effects (Left):        %s\n", Cores[c].WetGate.SndL ? "Yes" : "No");
			fprintf(dump, "Sound Data Input to Effects (Right):       %s\n", Cores[c].WetGate.SndR ? "Yes" : "No");
			fprintf(dump, "Voice Data Input to Direct Output (Left):  %s\n", Cores[c].DryGate.InpL ? "Yes" : "No");
			fprintf(dump, "Voice Data Input to Direct Output (Right): %s\n", Cores[c].DryGate.InpR ? "Yes" : "No");
			fprintf(dump, "Voice Data Input to Effects (Left):        %s\n", Cores[c].WetGate.InpL ? "Yes" : "No");
			fprintf(dump, "Voice Data Input to Effects (Right):       %s\n", Cores[c].WetGate.InpR ? "Yes" : "No");
			fprintf(dump, "IRQ Enabled:     %s\n", Cores[c].IRQEnable ? "Yes" : "No");
			fprintf(dump, "Effects Enabled: %s\n", Cores[c].FxEnable ? "Yes" : "No");
			fprintf(dump, "Mute Enabled:    %s\n", Cores[c].Mute ? "Yes" : "No");
			fprintf(dump, "Noise Clock:     %d\n", Cores[c].NoiseClk);
			fprintf(dump, "DMA Bits:        %d\n", Cores[c].DMABits);
			fprintf(dump, "Effects Start:   %x\n", Cores[c].EffectsStartA);
			fprintf(dump, "Effects End:     %x\n", Cores[c].EffectsEndA);
			fprintf(dump, "Registers:\n");
			fprintf(dump, "  - PMON:   %x\n", Cores[c].Regs.PMON);
			fprintf(dump, "  - NON:    %x\n", Cores[c].Regs.NON);
			fprintf(dump, "  - VMIXL:  %x\n", Cores[c].Regs.VMIXL);
			fprintf(dump, "  - VMIXR:  %x\n", Cores[c].Regs.VMIXR);
			fprintf(dump, "  - VMIXEL: %x\n", Cores[c].Regs.VMIXEL);
			fprintf(dump, "  - VMIXER: %x\n", Cores[c].Regs.VMIXER);
			fprintf(dump, "  - MMIX:   %x\n", Cores[c].Regs.VMIXEL);
			fprintf(dump, "  - ENDX:   %x\n", Cores[c].Regs.VMIXER);
			fprintf(dump, "  - STATX:  %x\n", Cores[c].Regs.VMIXEL);
			fprintf(dump, "  - ATTR:   %x\n", Cores[c].Regs.VMIXER);
			for (u8 v = 0; v < 24; v++)
			{
				fprintf(dump, "Voice %d:\n", v);
				Cores[c].Voices[v].Volume.DebugDump(dump, "");

				fprintf(dump, "  - ADSR Envelope: %x & %x\n"
							  "     - Ar: %x\n"
							  "     - Am: %x\n"
							  "     - Dr: %x\n"
							  "     - Sl: %x\n"
							  "     - Sr: %x\n"
							  "     - Sm: %x\n"
							  "     - Rr: %x\n"
							  "     - Rm: %x\n"
							  "     - Phase: %x\n"
							  "     - Value: %x\n",
						Cores[c].Voices[v].ADSR.regADSR1,
						Cores[c].Voices[v].ADSR.regADSR2,
						Cores[c].Voices[v].ADSR.AttackRate,
						Cores[c].Voices[v].ADSR.AttackMode,
						Cores[c].Voices[v].ADSR.DecayRate,
						Cores[c].Voices[v].ADSR.SustainLevel,
						Cores[c].Voices[v].ADSR.SustainRate,
						Cores[c].Voices[v].ADSR.SustainMode,
						Cores[c].Voices[v].ADSR.ReleaseRate,
						Cores[c].Voices[v].ADSR.ReleaseMode,
						Cores[c].Voices[v].ADSR.Phase,
						Cores[c].Voices[v].ADSR.Value);

				fprintf(dump, "  - Pitch:     %x\n", Cores[c].Voices[v].Pitch);
				fprintf(dump, "  - Modulated: %s\n", Cores[c].Voices[v].Modulated ? "Yes" : "No");
				fprintf(dump, "  - Source:    %s\n", Cores[c].Voices[v].Noise ? "Noise" : "Wave");
				fprintf(dump, "  - Direct Output for Left Channel:   %s\n", Cores[c].VoiceGates[v].DryL ? "Yes" : "No");
				fprintf(dump, "  - Direct Output for Right Channel:  %s\n", Cores[c].VoiceGates[v].DryR ? "Yes" : "No");
				fprintf(dump, "  - Effects Output for Left Channel:  %s\n", Cores[c].VoiceGates[v].WetL ? "Yes" : "No");
				fprintf(dump, "  - Effects Output for Right Channel: %s\n", Cores[c].VoiceGates[v].WetR ? "Yes" : "No");
				fprintf(dump, "  - Loop Start Address:  %x\n", Cores[c].Voices[v].LoopStartA);
				fprintf(dump, "  - Sound Start Address: %x\n", Cores[c].Voices[v].StartA);
				fprintf(dump, "  - Next Data Address:   %x\n", Cores[c].Voices[v].NextA);
				fprintf(dump, "  - Play Start Cycle:    %d\n", Cores[c].Voices[v].PlayCycle);
				fprintf(dump, "  - Play Status:         %s\n", (Cores[c].Voices[v].ADSR.Phase > 0) ? "Playing" : "Not Playing");
				fprintf(dump, "  - Block Sample:        %d\n", Cores[c].Voices[v].SCurrent);
			}
			fprintf(dump, "#### END OF DUMP.\n\n");
		}
		fclose(dump);
	}

	dump = EmuFolders::OpenLogFile("SPU2effects.txt", "wt");
	if (dump)
	{
		for (u8 c = 0; c < 2; c++)
		{
			fprintf(dump, "#### CORE %d EFFECTS PROCESSOR DUMP.\n", c);

			fprintf(dump, "  - IN_COEF_L:   %x\n", Cores[c].Revb.IN_COEF_R);
			fprintf(dump, "  - IN_COEF_R:   %x\n", Cores[c].Revb.IN_COEF_L);

			fprintf(dump, "  - APF1_VOL:    %x\n", Cores[c].Revb.APF1_VOL);
			fprintf(dump, "  - APF2_VOL:    %x\n", Cores[c].Revb.APF2_VOL);
			fprintf(dump, "  - APF1_SIZE:   %x\n", Cores[c].Revb.APF1_SIZE);
			fprintf(dump, "  - APF2_SIZE:   %x\n", Cores[c].Revb.APF2_SIZE);

			fprintf(dump, "  - IIR_VOL:     %x\n", Cores[c].Revb.IIR_VOL);
			fprintf(dump, "  - WALL_VOL:    %x\n", Cores[c].Revb.WALL_VOL);
			fprintf(dump, "  - SAME_L_SRC:  %x\n", Cores[c].Revb.SAME_L_SRC);
			fprintf(dump, "  - SAME_R_SRC:  %x\n", Cores[c].Revb.SAME_R_SRC);
			fprintf(dump, "  - DIFF_L_SRC:  %x\n", Cores[c].Revb.DIFF_L_SRC);
			fprintf(dump, "  - DIFF_R_SRC:  %x\n", Cores[c].Revb.DIFF_R_SRC);
			fprintf(dump, "  - SAME_L_DST:  %x\n", Cores[c].Revb.SAME_L_DST);
			fprintf(dump, "  - SAME_R_DST:  %x\n", Cores[c].Revb.SAME_R_DST);
			fprintf(dump, "  - DIFF_L_DST:  %x\n", Cores[c].Revb.DIFF_L_DST);
			fprintf(dump, "  - DIFF_R_DST:  %x\n", Cores[c].Revb.DIFF_R_DST);

			fprintf(dump, "  - COMB1_VOL:   %x\n", Cores[c].Revb.COMB1_VOL);
			fprintf(dump, "  - COMB2_VOL:   %x\n", Cores[c].Revb.COMB2_VOL);
			fprintf(dump, "  - COMB3_VOL:   %x\n", Cores[c].Revb.COMB3_VOL);
			fprintf(dump, "  - COMB4_VOL:   %x\n", Cores[c].Revb.COMB4_VOL);
			fprintf(dump, "  - COMB1_L_SRC: %x\n", Cores[c].Revb.COMB1_L_SRC);
			fprintf(dump, "  - COMB1_R_SRC: %x\n", Cores[c].Revb.COMB1_R_SRC);
			fprintf(dump, "  - COMB2_L_SRC: %x\n", Cores[c].Revb.COMB2_L_SRC);
			fprintf(dump, "  - COMB2_R_SRC: %x\n", Cores[c].Revb.COMB2_R_SRC);
			fprintf(dump, "  - COMB3_L_SRC: %x\n", Cores[c].Revb.COMB3_L_SRC);
			fprintf(dump, "  - COMB3_R_SRC: %x\n", Cores[c].Revb.COMB3_R_SRC);
			fprintf(dump, "  - COMB4_L_SRC: %x\n", Cores[c].Revb.COMB4_L_SRC);
			fprintf(dump, "  - COMB4_R_SRC: %x\n", Cores[c].Revb.COMB4_R_SRC);

			fprintf(dump, "  - APF1_L_DST:  %x\n", Cores[c].Revb.APF1_L_DST);
			fprintf(dump, "  - APF1_R_DST:  %x\n", Cores[c].Revb.APF1_R_DST);
			fprintf(dump, "  - APF2_L_DST:  %x\n", Cores[c].Revb.APF2_L_DST);
			fprintf(dump, "  - APF2_R_DST:  %x\n", Cores[c].Revb.APF2_R_DST);
			fprintf(dump, "#### END OF DUMP.\n\n");
		}
		fclose(dump);
	}
}

#endif
