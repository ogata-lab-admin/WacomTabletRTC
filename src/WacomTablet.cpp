// -*- C++ -*-
/*!
* @file  WacomTablet.cpp
* @brief ModuleDescription
* @date $Date$
*
* $Id$
*/
#include <Windows.h>

#include "WacomTablet.h"
#include "wintab.h"			// NOTE: get from wactab header package
#define PACKETDATA      (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | \
	PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE      0
#include "pktdef.h"			// NOTE: get from wactab header package

#include "msgpack.h"
#include "Utils.h"
#include  <math.h>
#include <string.h>


//from tilttest.h
#define IDM_ABOUT 100


#define FIX_DOUBLE(x)   ((double)(INT(x))+((double)FRAC(x)/65536))
#define pi 3.14159265359

HANDLE          hInst;               /* Handle for instance */
POINT           ptNew={0.0,0.0};               /* XY value storage */
UINT            prsNew=1;              /* Pressure value storage */
UINT            curNew=1;              /* Cursor number storage */
ORIENTATION     ortNew={1,1,1};              /* Tilt value storage */
RECT            rcDraw;              /* Size of draw area */
RECT            rcInfoTilt;          /* Size of tilt info box */
RECT            rcInfoName;          /* Size of cursor name box */
BOOL            tilt_support = TRUE; /* Is tilt supported */
double          aziFactor = 1;       /* Azimuth factor */
double          altFactor = 1;       /* Altitude factor */
double          altAdjust = 1;       /* Altitude zero adjust */

RECT            rcClient;            /* Size of current Client */
RECT            rcInfoGen;           /* Size of testing box */
HDC hDC;

PACKET          pkt;             /* the current packet */

HWND hWnd;
MSG msg;

HCTX hTab=NULL;


HINSTANCE hInstance;
HINSTANCE hPrevInstance;
PSTR lpCmdLine;
int nCmdShow;


HWND hWndConsole;

BOOL SetupHDC(void){
	TEXTMETRIC      textmetric;       /* Structure for font info */
	int             nLineH;           /* Holds the text height */
	int             Xinch, Yinch;     /* Holds the number of pixels per inch */
	int             Hres, Vres;       /* Holds the screen resolution */


	    /* Get Device Context and setup a rects  to write packet info */
    hDC = GetDC(hWnd);
    if (!hDC)
	return FALSE;
    GetTextMetrics(hDC, &textmetric);
    nLineH = textmetric.tmExternalLeading + textmetric.tmHeight;
    Xinch = GetDeviceCaps(hDC, LOGPIXELSX);
    Yinch = GetDeviceCaps(hDC, LOGPIXELSY);
    Hres = GetDeviceCaps(hDC, HORZRES);
    Vres = GetDeviceCaps(hDC, VERTRES);
    ReleaseDC(hWnd, hDC);

	GetClientRect(hWnd, &rcClient);
	rcInfoTilt = rcClient;
    rcInfoTilt.left   = Xinch / 8;
    rcInfoTilt.top    = Yinch / 8;
    rcInfoTilt.bottom = rcInfoTilt.top + nLineH;
    rcInfoName = rcInfoTilt;
    rcInfoName.top    += nLineH;
    rcInfoName.bottom += nLineH;
    rcInfoGen = rcInfoName;
    rcInfoGen.top    += nLineH;
    rcInfoGen.bottom += nLineH;
    rcDraw = rcInfoGen;
    rcDraw.left   = 0;
    rcDraw.top   += nLineH;
    rcDraw.bottom = rcClient.bottom;

}


BOOL LoadAndCheckWintab(void){

	char            WName[50];        /* String to hold window name */
	struct          tagAXIS TpOri[3]; /* The capabilities of tilt */
	double          tpvar;            /* A temp for converting fix to double */

	if ( !LoadWintab( ) )
	{

		return FALSE;
	}

	/* check if WinTab available. */
	if (!gpWTInfoA(0, 0, NULL)) {

		return FALSE;
	}

	/* check if WACOM available. */
    gpWTInfoA(WTI_DEVICES, DVC_NAME, WName);
    if (strncmp(WName,"WACOM",5)) {

//      return FALSE;
    }

	/* get info about tilt */
	tilt_support = gpWTInfoA(WTI_DEVICES,DVC_ORIENTATION,&TpOri);
	if (tilt_support) {
		/* does the tablet support azimuth and altitude */
		if (TpOri[0].axResolution && TpOri[1].axResolution) {

			/* convert azimuth resulution to double */
			tpvar = FIX_DOUBLE(TpOri[0].axResolution);
			/* convert from resolution to radians */
			aziFactor = tpvar/(2*pi);  
			
			/* convert altitude resolution to double */
			tpvar = FIX_DOUBLE(TpOri[1].axResolution);
			/* scale to arbitrary value to get decent line length */ 
			altFactor = tpvar/1000; 
			 /* adjust for maximum value at vertical */
			altAdjust = (double)TpOri[1].axMax/altFactor;
		}
		else {  /* no so dont do tilt stuff */
			tilt_support = FALSE;
		}
	}
	

	return TRUE;
}

HCTX static NEAR TabletInit(HWND hWnd)
{
	LOGCONTEXT      lcMine;           /* The context of the tablet */
	AXIS            TabletX, TabletY; /* The maximum tablet size */

	/* get default region */
	gpWTInfoA(WTI_DEFCONTEXT, 0, &lcMine);

	/* modify the digitizing region */
	wsprintf(lcMine.lcName, "TiltTest Digitizing %x", hInst);
	lcMine.lcOptions |= CXO_MESSAGES;
	lcMine.lcPktData = PACKETDATA;
	lcMine.lcPktMode = PACKETMODE;
	lcMine.lcMoveMask = PACKETDATA;
	lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;

    /* Set the entire tablet as active */
	gpWTInfoA(WTI_DEVICES,DVC_X,&TabletX);
	gpWTInfoA(WTI_DEVICES,DVC_Y,&TabletY);
	lcMine.lcInOrgX = 0;
	lcMine.lcInOrgY = 0;
	lcMine.lcInExtX = TabletX.axMax;
	lcMine.lcInExtY = TabletY.axMax;

    /* output the data in screen coords */
	lcMine.lcOutOrgX = lcMine.lcOutOrgY = 0;
	lcMine.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
    /* move origin to upper left */
	lcMine.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);

	/* open the region */
	return gpWTOpenA(hWnd, &lcMine, TRUE);

}
//計測用ウインドウのプロージャ
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam){
	BOOL            fHandled = TRUE; /* whether the message was handled or not */
	LRESULT         lResult = 0L;    /* the result of the message */

	switch(msg){
	case WM_CREATE:
		hTab=TabletInit(hWnd);
		break;
	case WM_DESTROY: /* The window was destroyed */
		if (hTab)
			gpWTClose(hTab);
		PostQuitMessage(0);
		break;
	case WT_PACKET: /* A packet is waiting from WINTAB */
			if (gpWTPacket((HCTX)lParam, wParam, &pkt)) {
				
				/* old co-ordinates used for comparisons */
				POINT 		ptOld = ptNew; 
				UINT  		prsOld = prsNew;
				UINT  		curOld = curNew;
				ORIENTATION ortOld = ortNew;
				
				/* save new co-ordinates */
				ptNew.x = (UINT)pkt.pkX;
				ptNew.y = (UINT)pkt.pkY;
				curNew = pkt.pkCursor;
				prsNew = pkt.pkNormalPressure;
				ortNew = pkt.pkOrientation;
				
				/* If the visual changes update the main graphic */
				if (ptNew.x != ptOld.x ||
					ptNew.y != ptOld.y ||
					prsNew != prsOld ||
					ortNew.orAzimuth != ortOld.orAzimuth ||
					ortNew.orAltitude != ortOld.orAltitude ||
					ortNew.orTwist != ortOld.orTwist) {                                     
					InvalidateRect(hWnd, &rcDraw, TRUE);
				}
				/* if the displayed data changes update the text */
				if (ortNew.orAzimuth != ortOld.orAzimuth ||
					ortNew.orAltitude != ortOld.orAltitude ||
					ortNew.orTwist != ortOld.orTwist) {
					InvalidateRect(hWnd, &rcInfoTilt, TRUE);
				}
				/* if the cursor changes update the cursor name */
				if (curNew != curOld) {
					InvalidateRect(hWnd, &rcInfoName, TRUE);
				}
			}
		break;
			default:
			fHandled = FALSE;
			break;
	}
		if (fHandled)
		return (lResult);
	else
		return (DefWindowProc(hWnd, msg, wParam, lParam));
}

//計測用のウインドウを生成、表示
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow){
	
	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=hInstance;
	wc.hIcon=LoadIcon(NULL , IDI_APPLICATION);
	wc.hCursor=LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground=(HBRUSH)(COLOR_APPWORKSPACE+1);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=TEXT("WacomTablet");

	if (!RegisterClass(&wc)) return 0;
	
	if(!LoadAndCheckWintab()) return 0;

	//hWndCosoleはRTコンポーネントのコンソールウインドウのハンドラ
	hWnd=CreateWindow(
		TEXT("WacomTablet"),TEXT("WacomTablet"),WS_OVERLAPPEDWINDOW,
		100, 100, 300, 300, hWndConsole, NULL, hInstance, NULL
	);

	if (hWnd == NULL) return 0;


	if(!SetupHDC()) return 0;


	ShowWindow(hWnd , SW_SHOW);

	while(TRUE){
		GetMessage(&msg, NULL, 0, 0);
		DispatchMessage(&msg);
	}
	return 0;
}





// Module specification
// <rtc-template block="module_spec">
static const char* wacomtablet_spec[] =
{
	"implementation_id", "WacomTablet",
	"type_name",         "WacomTablet",
	"description",       "ModuleDescription",
	"version",           "0.0.1",
	"vendor",            "Ogata-lab",
	"category",          "InputDevic",
	"activity_type",     "PERIODIC",
	"kind",              "DataFlowComponent",
	"max_instance",      "1",
	"language",          "C++",
	"lang_type",         "compile",
	// Configuration variables
	"conf.default.debug", "1",
	// Widget
	"conf.__widget__.debug", "text",
	// Constraints
	""
};
// </rtc-template>

/*!
* @brief constructor
* @param manager Maneger Object
*/
WacomTablet::WacomTablet(RTC::Manager* manager)
	// <rtc-template block="initializer">
	: RTC::DataFlowComponentBase(manager),
	m_positionOut("position", m_position),
	m_pressureOut("pressure", m_pressure),
	m_orientationOut("orientation", m_orientation),
	m_sizeOut("size", m_size)

	// </rtc-template>
{
}

/*!
* @brief destructor
*/
WacomTablet::~WacomTablet()
{
}



RTC::ReturnCode_t WacomTablet::onInitialize()
{
	// Registration: InPort/OutPort/Service
	// <rtc-template block="registration">
	// Set InPort buffers

	// Set OutPort buffer
	addOutPort("position", m_positionOut);
	addOutPort("pressure", m_pressureOut);
	addOutPort("orientation", m_orientationOut);
	addOutPort("size", m_sizeOut);

	// Set service provider to Ports

	// Set service consumers to Ports

	// Set CORBA Service Ports

	// </rtc-template>

	// <rtc-template block="bind_config">
	// Bind variables and configuration variable
	bindParameter("debug", m_debug, "1");
	// </rtc-template>

	return RTC::RTC_OK;
}



/*
RTC::ReturnCode_t WacomTablet::onFinalize()
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onStartup(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onShutdown(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/


RTC::ReturnCode_t WacomTablet::onActivated(RTC::UniqueId ec_id)
{


	//***************************************
	//get handle and instance from console
	hWndConsole=GetConsoleHwnd();
	WinMain(hInstance,hPrevInstance, lpCmdLine, nCmdShow);

	return RTC::RTC_OK;
}



RTC::ReturnCode_t WacomTablet::onDeactivated(RTC::UniqueId ec_id)
{
	
	SendMessage(hWnd , WM_CLOSE , 0 , 0);


	gpWTClose(hTab);

	WACOM_TRACE( "Cleanup()\n" );

	UnloadWintab( );	

	return RTC::RTC_OK;
}




RTC::ReturnCode_t WacomTablet::onExecute(RTC::UniqueId ec_id)
{
	
	m_position.data.x=ptNew.x;
	m_position.data.y=ptNew.y;
	m_positionOut.write();
	return RTC::RTC_OK;
}

/*
RTC::ReturnCode_t WacomTablet::onAborting(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onError(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onReset(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onStateUpdate(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/

/*
RTC::ReturnCode_t WacomTablet::onRateChanged(RTC::UniqueId ec_id)
{
return RTC::RTC_OK;
}
*/



extern "C"
{

	void WacomTabletInit(RTC::Manager* manager)
	{
		coil::Properties profile(wacomtablet_spec);
		manager->registerFactory(profile,
			RTC::Create<WacomTablet>,
			RTC::Delete<WacomTablet>);
	}

};


