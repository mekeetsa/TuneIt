; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CFFTWnd
LastTemplate=generic CWnd
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "tuneit.h"
LastPage=0

ClassCount=3
Class1=CFFTWnd
Class2=CTuneItApp

ResourceCount=2
Class3=CFlatButton
Resource1=IDC_ACCELERATOR
Resource2=IDD_DIALOGBAR

[CLS:CFFTWnd]
Type=0
BaseClass=CWnd
HeaderFile=TuneIt.h
ImplementationFile=FFTWnd.cpp
LastObject=CFFTWnd
Filter=W
VirtualFilter=WC

[CLS:CTuneItApp]
Type=0
BaseClass=CWinApp
HeaderFile=TuneIt.h
ImplementationFile=TuneIt.cpp
LastObject=CTuneItApp
Filter=N
VirtualFilter=AC

[ACL:IDC_ACCELERATOR]
Type=1
Class=CFFTWnd
Command1=IDM_MEASURE_WAVE_IN
Command2=IDM_FFTWIN_RECTANGULAR
Command3=IDM_VIEW_FREQ_LOG
Command4=IDM_MEASURE_WAVE_OUT
Command5=IDM_FFTWIN_HANNING
Command6=IDM_VIEW_FREQ_LIN
Command7=IDM_FFTWIN_BLACKMAN_HARRIS
Command8=IDM_VIEW_CHROMATIC
Command9=IDM_FFTWIN_KAISER_BESSEL
Command10=IDM_VIEW_LOG_TOGGLE
Command11=IDM_TOGGLE_TRACE_WINDOW
Command12=IDM_CLEAR_LEARNED_NOISE
Command13=IDM_START_LEARNING_NOISE
Command14=IDM_VIEW_ZOOM_OUT
Command15=IDM_TOGGLE_PK_WIN
Command16=IDM_TEST_BUTTON
Command17=IDM_OCTAVE_UP
Command18=IDM_CUROSR_DOWN
Command19=IDM_CUROSR_DOWN10
Command20=IDM_STOP_PLAYING
Command21=IDM_PLAY_TONE
Command22=IDM_TUNE_VIOLIN_G
Command23=IDM_TUNE_VIOLIN_D
Command24=IDM_TUNE_VIOLIN_A
Command25=IDM_TUNE_VIOLIN_E
Command26=IDM_TUNE_AUTO
Command27=IDM_CURSOR_LEFT
Command28=IDM_CURSOR_LEFT10
Command29=IDM_TOGGLE_PLAYING
Command30=IDM_DISPLAY_PEAK_INFO
Command31=IDM_WATERFALL_INC_SPEED
Command32=IDM_DISPLAY_FFT
Command33=IDM_FREEZE_WATERFALL
Command34=IDM_VIEW_WATERFALL_TOGGLE
Command35=IDM_WATERFALL_DEC_SPEED
Command36=IDM_TOGGLE_VIEW
Command37=IDM_CURSOR_RIGHT
Command38=IDM_CURSOR_RIGHT10
Command39=IDM_TOGGLE_VIEW
Command40=IDM_OCTAVE_DOWN
Command41=IDM_CURSOR_UP
Command42=IDM_CURSOR_UP10
CommandCount=42

[CLS:CFlatButton]
Type=0
HeaderFile=FlatButton.h
ImplementationFile=FlatButton.cpp
BaseClass=CButton
Filter=W
LastObject=CFlatButton
VirtualFilter=BWC

[DLG:IDD_DIALOGBAR]
Type=1
Class=?
ControlCount=1
Control1=IDC_SLIDER1,msctls_trackbar32,1342242827

