/*  ZeroSPU2
 *  Copyright (C) 2006-2007 zerofrog
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
 
 // Modified by arcum42@gmail.com
 
 #ifndef __LINUX_H__
 #define __LINUX_H__
 
#include <assert.h>
#include <stdlib.h>

#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#include <zerospu2.h>

// Make it easier to check and set checkmarks in the gui
#define is_checked(main_widget, widget_name) (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)))) 
#define set_checked(main_widget,widget_name, state) gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(lookup_widget(main_widget, widget_name)), state)

// Alsa & OSS defines
#ifdef ZEROSPU2_OSS

#define OSS_MODE_STEREO	1
#define SOUNDSIZE 76800

// Pull in from OSS.cpp
extern int OSSSetupSound();
extern void OSSRemoveSound();
extern int OSSSoundGetBytesBuffered();
extern void OSSSoundFeedVoiceData(unsigned char* pSound,long lBytes);

#else

#define ALSA_PCM_NEW_HW_PARAMS_API
#define ALSA_PCM_NEW_SW_PARAMS_API

#ifdef ALSA_MEM_DEF
#define ALSA_MEM_EXTERN
#else
#define ALSA_MEM_EXTERN extern
#endif

#define SOUNDSIZE 500000

// Pull in from Alsa.cpp
extern int AlsaSetupSound();
extern void AlsaRemoveSound();
extern int AlsaSoundGetBytesBuffered();
extern void AlsaSoundFeedVoiceData(unsigned char* pSound,long lBytes);
#endif

extern int SetupSound();
extern void RemoveSound();
extern int SoundGetBytesBuffered();
extern void SoundFeedVoiceData(unsigned char* pSound,long lBytes);

#endif // __LINUX_H__