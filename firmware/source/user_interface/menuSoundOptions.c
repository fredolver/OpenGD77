/*
 * Copyright (C)2019 Roger Clark. VK3KYY / G4KYF
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <hardware/UC1701.h>
#include <settings.h>
#include <user_interface/menuSystem.h>
#include <user_interface/uiLocalisation.h>
#include <user_interface/uiUtilities.h>

static void updateScreen(void);
static void handleEvent(uiEvent_t *ev);

static menuStatus_t menuSoundExitCode = MENU_STATUS_SUCCESS;

enum SOUND_MENU_LIST { OPTIONS_MENU_TIMEOUT_BEEP = 0, OPTIONS_MENU_BEEP_VOLUME, OPTIONS_MENU_DMR_BEEP,
						OPTIONS_MIC_GAIN_DMR, OPTIONS_MIC_GAIN_FM,
						OPTIONS_VOX_THRESHOLD, OPTIONS_VOX_TAIL, OPTIONS_AUDIO_PROMPT_MODE,
						NUM_SOUND_MENU_ITEMS};

menuStatus_t menuSoundOptions(uiEvent_t *ev, bool isFirstRun)
{
	if (isFirstRun)
	{
		// Store original settings, used on cancel event.
		memcpy(&originalNonVolatileSettings, &nonVolatileSettings, sizeof(settingsStruct_t));
		updateScreen();
		return (MENU_STATUS_LIST_TYPE | MENU_STATUS_SUCCESS);
	}
	else
	{
		menuSoundExitCode = MENU_STATUS_SUCCESS;

		if (ev->hasEvent)
			handleEvent(ev);
	}
	return menuSoundExitCode;
}

static void updateScreen(void)
{
	int mNum = 0;
	static const int bufferLen = 17;
	char buf[bufferLen];
	char * const *leftSide = NULL;// initialise to please the compiler
	char * const *rightSideConst = NULL;// initialise to please the compiler
	char rightSideVar[bufferLen];


	ucClearBuf();
	menuDisplayTitle(currentLanguage->sound_options);

	// Can only display 3 of the options at a time menu at -1, 0 and +1
	for(int i = -1; i <= 1; i++)
	{
		mNum = menuGetMenuOffset(NUM_SOUND_MENU_ITEMS, i);
		buf[0] = 0;

		rightSideVar[0] = 0;

		switch(mNum)
		{
			case OPTIONS_MENU_TIMEOUT_BEEP:
				leftSide = (char * const *)&currentLanguage->timeout_beep;
				if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
				{
					rightSideConst = (char * const *)&currentLanguage->n_a;
				}
				else
				{
					if (nonVolatileSettings.txTimeoutBeepX5Secs != 0)
					{
						snprintf(rightSideVar, bufferLen, "%d",nonVolatileSettings.txTimeoutBeepX5Secs * 5);
					}
					else
					{
						rightSideConst = (char * const *)&currentLanguage->n_a;
					}
				}
				break;
			case OPTIONS_MENU_BEEP_VOLUME: // Beep volume reduction
				leftSide = (char * const *)&currentLanguage->beep_volume;
				if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
				{
					rightSideConst = (char * const *)&currentLanguage->n_a;
				}
				else
				{
					snprintf(rightSideVar, bufferLen, "%ddB", (2 - nonVolatileSettings.beepVolumeDivider) * 3);
					soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
				}

				break;
			case OPTIONS_MENU_DMR_BEEP:
				leftSide = (char * const *)&currentLanguage->dmr_beep;
				if (nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_SILENT)
				{
					rightSideConst = (char * const *)&currentLanguage->n_a;
				}
				else
				{
					const char * const *beepTX[] = {&currentLanguage->none, &currentLanguage->start, &currentLanguage->stop, &currentLanguage->both};
					rightSideConst = (char * const *)beepTX[nonVolatileSettings.beepOptions];
				}
				break;
			case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
				leftSide = (char * const *)&currentLanguage->dmr_mic_gain;
				snprintf(rightSideVar, bufferLen, "%ddB", (nonVolatileSettings.micGainDMR - 11) * 3);
				break;
			case OPTIONS_MIC_GAIN_FM: // FM Mic gain
				leftSide = (char * const *)&currentLanguage->fm_mic_gain;
				snprintf(rightSideVar, bufferLen, "%d", (nonVolatileSettings.micGainFM - 16));
				break;
			case OPTIONS_VOX_THRESHOLD:
				leftSide = (char * const *)&currentLanguage->vox_threshold;
				snprintf(rightSideVar, bufferLen, "%d", nonVolatileSettings.voxThreshold);
				break;
			case OPTIONS_VOX_TAIL:
				leftSide = (char * const *)&currentLanguage->vox_tail;
				if (nonVolatileSettings.voxThreshold != 0)
				{
					float tail = (nonVolatileSettings.voxTailUnits * 0.5);
					uint8_t secs = (uint8_t)tail;
					uint8_t fracSec = (tail - secs) * 10;

					snprintf(rightSideVar, bufferLen, "%d.%ds",secs, fracSec);
				}
				else
				{
					rightSideConst = (char * const *)&currentLanguage->n_a;
				}
				break;
			case OPTIONS_AUDIO_PROMPT_MODE:
				{
					leftSide = (char * const *)&currentLanguage->audio_prompt;
					const char * const *audioPromptOption[] = {&currentLanguage->silent, &currentLanguage->normal, &currentLanguage->beep, &currentLanguage->voice};
					rightSideConst = (char * const *)audioPromptOption[nonVolatileSettings.audioPromptMode];
				}
				break;
		}

		snprintf(buf, bufferLen, "%s:%s", *leftSide, (rightSideVar[0]?rightSideVar:*rightSideConst));

		if (i==0 && nonVolatileSettings.audioPromptMode == AUDIO_PROMPT_MODE_VOICE)
		{
			voicePromptsInit();
			voicePromptsAppendLanguageString((const char * const *)leftSide);
			if (rightSideVar[0] !=0)
			{
				voicePromptsAppendString(rightSideVar);
			}
			else
			{
				voicePromptsAppendLanguageString((const char * const *)rightSideConst);
			}
			voicePromptsPlay();
		}

		buf[bufferLen - 1] = 0;
		menuDisplayEntry(i, mNum, buf);
	}

	ucRender();
	displayLightTrigger();
}

static void handleEvent(uiEvent_t *ev)
{
	displayLightTrigger();

	if (ev->events & KEY_EVENT)
	{
		if (KEYCHECK_PRESS(ev->keys,KEY_DOWN) && gMenusEndIndex!=0)
		{
			menuSystemMenuIncrement(&gMenusCurrentItemIndex, NUM_SOUND_MENU_ITEMS);
			menuSoundExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_UP))
		{
			menuSystemMenuDecrement(&gMenusCurrentItemIndex, NUM_SOUND_MENU_ITEMS);
			menuSoundExitCode |= MENU_STATUS_LIST_TYPE;
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_RIGHT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.txTimeoutBeepX5Secs < 4)
						{
							nonVolatileSettings.txTimeoutBeepX5Secs++;
						}
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepVolumeDivider > 0)
						{
							nonVolatileSettings.beepVolumeDivider--;
						}
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepOptions < (BEEP_TX_START | BEEP_TX_STOP))
						{
							nonVolatileSettings.beepOptions++;
						}
					}
					break;
				case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
					if (nonVolatileSettings.micGainDMR < 15)
					{
						nonVolatileSettings.micGainDMR++;
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
				case OPTIONS_MIC_GAIN_FM: // FM Mic gain
					if (nonVolatileSettings.micGainFM < 31)
					{
						nonVolatileSettings.micGainFM++;
						setMicGainFM(nonVolatileSettings.micGainFM);
					}
					break;
				case OPTIONS_VOX_THRESHOLD:
					if (nonVolatileSettings.voxThreshold < 30)
					{
						nonVolatileSettings.voxThreshold++;
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_VOX_TAIL:
					if (nonVolatileSettings.voxTailUnits < 10) // 5 seconds max
					{
						nonVolatileSettings.voxTailUnits++;
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_AUDIO_PROMPT_MODE:
					if (nonVolatileSettings.audioPromptMode < (NUM_AUDIO_PROMPT_MODES - 2 + (int)voicePromptDataIsLoaded ))
					{
						nonVolatileSettings.audioPromptMode++;
					}
					break;

			}
		}
		else if (KEYCHECK_PRESS(ev->keys,KEY_LEFT))
		{
			switch(gMenusCurrentItemIndex)
			{
				case OPTIONS_MENU_TIMEOUT_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.txTimeoutBeepX5Secs > 0)
						{
							nonVolatileSettings.txTimeoutBeepX5Secs--;
						}
					}
					break;
				case OPTIONS_MENU_BEEP_VOLUME:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepVolumeDivider < 10)
						{
							nonVolatileSettings.beepVolumeDivider++;
						}
					}
					break;
				case OPTIONS_MENU_DMR_BEEP:
					if (nonVolatileSettings.audioPromptMode != AUDIO_PROMPT_MODE_SILENT)
					{
						if (nonVolatileSettings.beepOptions > BEEP_TX_NONE)
						{
							nonVolatileSettings.beepOptions--;
						}
					}
					break;
				case OPTIONS_MIC_GAIN_DMR: // DMR Mic gain
					if (nonVolatileSettings.micGainDMR > 0)
					{
						nonVolatileSettings.micGainDMR--;
						setMicGainDMR(nonVolatileSettings.micGainDMR);
					}
					break;
				case OPTIONS_MIC_GAIN_FM: // FM Mic gain
					if (nonVolatileSettings.micGainFM > 1) // Limit to min 1, as 0: no audio
					{
						nonVolatileSettings.micGainFM--;
						setMicGainFM(nonVolatileSettings.micGainFM);
					}
					break;
				case OPTIONS_VOX_THRESHOLD:
					// threshold of 1 is too low. So only allow the value to go down to 2.
					if (nonVolatileSettings.voxThreshold > 2)
					{
						nonVolatileSettings.voxThreshold--;
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_VOX_TAIL:
					if (nonVolatileSettings.voxTailUnits > 1) // .5 minimum
					{
						nonVolatileSettings.voxTailUnits--;
						voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
					}
					break;
				case OPTIONS_AUDIO_PROMPT_MODE:
					if (nonVolatileSettings.audioPromptMode > AUDIO_PROMPT_MODE_SILENT)
					{
						nonVolatileSettings.audioPromptMode--;
					}
					break;
			}
		}
		else if (KEYCHECK_SHORTUP(ev->keys,KEY_GREEN))
		{
			// All parameters has already been applied
			SETTINGS_PLATFORM_SPECIFIC_SAVE_SETTINGS(false);// Some platform require the settings to be saved immediately

			menuSystemPopAllAndDisplayRootMenu();
			return;
		}
		else if (KEYCHECK_SHORTUP(ev->keys,KEY_RED))
		{
			// Restore original settings.
			memcpy(&nonVolatileSettings, &originalNonVolatileSettings, sizeof(settingsStruct_t));
			soundBeepVolumeDivider = nonVolatileSettings.beepVolumeDivider;
			setMicGainDMR(nonVolatileSettings.micGainDMR);
			setMicGainFM(nonVolatileSettings.micGainFM);
			voxSetParameters(nonVolatileSettings.voxThreshold, nonVolatileSettings.voxTailUnits);
			menuSystemPopPreviousMenu();
			return;
		}
	}
	updateScreen();
}
