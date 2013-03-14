//##########################################################################
//#                                                                        #
//#                            CLOUDCOMPARE                                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: EDF R&D / TELECOM ParisTech (ENST-TSI)             #
//#                                                                        #
//##########################################################################

#ifndef MOUSE_3D_INPUT_HEADER
#define MOUSE_3D_INPUT_HEADER

/** This file is inspired from the Qt wrapper for 3dConnexion devices graciously shared by Dabid Dibben:
	http://www.codegardening.com/2011/02/using-3dconnexion-mouse-with-qt.html
**/

#include "Mouse3DParameters.h"

//Qt
#include <QWidget>

//system
#include <vector>
#include <map>

#if defined(_WIN32) || defined(WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501  //target at least windows XP
#endif

#include <windows.h>

//! Class for connecting to and receiving data from a 3D Connexion 3D mouse
/**	This helper class manages the connection to a 3D mouse, collecting WM_INPUT
	messages from Windows and converting the data into 3D motion data.

	This class is based on the SDK from 3D Connexion but is modified to work with Qt.

	It is Windows only since it uses the WM_INPUT messages from windows directly
	rather than Qt events.

	Note that it needs to be compiled with _WIN32_WINNT defined as 0x0501 (windows XP)
	or later because the raw input API was added in Windows XP. This also means that
	Qt needs to be compiled with this enabled otherwise the QEventDispatcherWin32 blocks
	in processEvents because the raw input messages do not cause the thread to be woken if
	Qt is compiled for Win 2000 targets.

	Author: Dabid Dibben (http://www.codegardening.com)

**/
class Mouse3DInput : public QObject
{
	Q_OBJECT

public:

	//! Default constructor
	Mouse3DInput(QWidget* widget);
	//! Destructor
	~Mouse3DInput();

	//! Returns whether a 3D mouse has been detected or not
	static bool Is3dmouseAttached();

	//! Returns current mouse parameters
	Mouse3DParameters& getMouseParams() { return m_3dMouseParams; }
	//! Returns current mouse parameters (const version)
	const Mouse3DParameters& getMouseParams() const  { return m_3dMouseParams; }

	//! Called with the processed motion data when a 3D mouse event is received
	/** The default implementation emits a sigMove3d signal with the motion data
	*/
	virtual	void move3d(HANDLE device, std::vector<float>& motionData);

	//! Called when a 3D mouse key is pressed
	/** The default implementation emits a sigOn3dmouseKeyDown signal with the key code.
	*/
	virtual void on3dmouseKeyDown(HANDLE device, int virtualKeyCode);

	//! Called when a 3D mouse key is released
	/** The default implementation emits a sigOn3dmouseKeyUp signal with the key code.
	**/
	virtual void on3dmouseKeyUp(HANDLE device, int virtualKeyCode);

signals:

	void sigMove3d(std::vector<float>& motionData);
	void sigOn3dmouseKeyDown(int virtualKeyCode);
	void sigOn3dmouseKeyUp(int virtualKeyCode);

protected:

	//! Initialize the window to recieve raw-input messages
	/** This needs to be called initially so that Windows will send the messages from the 3D mouse to the window.
	**/
	bool initializeRawInput(HWND hwndTarget);

	static bool RawInputEventFilter(void* msg, long* result);

	//! Called when new raw input data is available
	void onRawInput(UINT nInputCode, HRAWINPUT hRawInput);

	//! Gets the raw input data from Windows
	/** Includes workaround for incorrect alignment of the RAWINPUT structure on x64 os
		when running as Wow64 (copied directly from 3DConnexion code)
	**/
	UINT getRawInputBuffer(PRAWINPUT pData, PUINT pcbSize, UINT cbSizeHeader);
	
	bool translateRawInputData(UINT nInputCode, PRAWINPUT pRawInput);
	
	//! Processes the raw input device data
	/** Does all the preprocessing of the rawinput device data before
		finally calling the move3d method.
	**/
	void on3dmouseInput();

	class InputData
	{
	public:
		
		InputData() : axes(6) {}

		bool isZero() const { return (0.0f == axes[0] &&
										0.0f == axes[1] &&
										0.0f == axes[2] &&
										0.0f == axes[3] &&
										0.0f == axes[4] &&
										0.0f == axes[5]);
		}

		int	timeToLive; // tell if the device was unplugged while sending data
		bool isDirty;
		std::vector<float> axes;
	};

	//! Associated window handle
	HWND m_window;

	//! Data cache to handle multiple rawinput devices
	std::map< HANDLE, InputData > m_device2Data;
	//! Data cache to handle multiple rawinput devices
	std::map< HANDLE, unsigned long > m_device2Keystate;

	//! 3D Mouse parameters
	Mouse3DParameters m_3dMouseParams;

	//! Used to calculate distance traveled since last event
	DWORD m_last3dmouseInputTime;

};

#endif //defined(_WIN32) || defined(WIN32)

#endif //MOUSE_3D_INPUT_HEADER