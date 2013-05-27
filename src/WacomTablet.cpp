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


//from tilttest.h
#define IDM_ABOUT 100


#define FIX_DOUBLE(x)   ((double)(INT(x))+((double)FRAC(x)/65536))
#define pi 3.14159265359


POINT           ptNew={0.0,0.0};               /* XY value storage */
UINT            prsNew=1;              /* Pressure value storage */
UINT            curNew=1;              /* Cursor number storage */
ORIENTATION     ortNew={1,1,1};              /* Tilt value storage */
RECT            rcDraw;              /* Size of draw area */
RECT            rcInfoTilt;          /* Size of tilt info box */
RECT            rcInfoName;          /* Size of cursor name box */

HWND hWnd;
HANDLE hInstance;
WPARAM wParam=1;
LPARAM lParam=1;

HCTX hTab;


MSG msg;

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
	//get function address from wintab32.dll
	if(!LoadWintab()){
		return RTC::RTC_ERROR;
	}


	BOOL            tilt_support = TRUE;
	double          tpvar;
	double          aziFactor = 1;       /* Azimuth factor */
	double          altFactor = 1;       /* Altitude factor */
	double          altAdjust = 1;       /* Altitude zero adjust */

	//get info about tilt
	struct          tagAXIS TpOri[3];
	tilt_support=gpWTInfoA(WTI_DEVICES,DVC_ORIENTATION, &TpOri);
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


	//get handle and instance from console


	hWnd=GetConsoleHwnd();
	hInstance=GetModuleHandle(NULL);

	/* Get Device Context and setup a rects  to write packet info */
	HDC hDC = GetDC(hWnd);
	if(!hDC) return RTC::RTC_ERROR;

	TEXTMETRIC      textmetric;       /* Structure for font info */
	int             nLineH;			  /* Holds the text height */
	int             Xinch, Yinch;     /* Holds the number of pixels per inch */
	int             Hres, Vres;       /* Holds the screen resolution */
	RECT            rcClient;            /* Size of current Client */
	RECT            rcInfoTilt;          /* Size of tilt info box */
	RECT            rcInfoName;          /* Size of cursor name box */
	RECT            rcInfoGen;           /* Size of testing box */
	RECT            rcDraw;              /* Size of draw area */

	GetTextMetrics(hDC,&textmetric);
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

	//tablet init
	LOGCONTEXT      lcMine;           /* The context of the tablet */
	AXIS            TabletX, TabletY; /* The maximum tablet size */

	/* get default region */
	gpWTInfoA(WTI_DEFCONTEXT, 0, &lcMine);

	/* modify the digitizing region */
	wsprintf(lcMine.lcName, "TiltTest Digitizing %x", hInstance);
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

	hTab = gpWTOpenA(hWnd, &lcMine, TRUE);




	///TODO: ここでウィンドウを作って，hWnd2という変数を上で宣言して，
	/// そいつにウィンドウハンドらを設定せよ
	return RTC::RTC_OK;
}



RTC::ReturnCode_t WacomTablet::onDeactivated(RTC::UniqueId ec_id)
{
	///TODO: 作ったhWin2を消す

	gpWTClose(hTab);

	WACOM_TRACE( "Cleanup()\n" );

	UnloadWintab( );	

	return RTC::RTC_OK;
}


RTC::ReturnCode_t WacomTablet::onExecute(RTC::UniqueId ec_id)
{
	PACKET          pkt;             /* the current packet */

	/* A packet is waiting from WINTAB */

	if (GetMessage(&msg, hWnd2, 0,  0) <= 0) { /// コンパイルエラーになる．hWnd2を上のほうでで宣言せよ
		return RTC::RTC_ERROR;
	}

	std::cout << msg.message << "/" << WT_PACKET << std::endl;
	TranslateMessage(&msg);
	DispatchMessage(&msg);

	//gpWTEnable(hTab, GET_WM_ACTIVATE_STATE(wParam, lParam));

	wParam=msg.wParam;
	//lParam=msg.lParam;

	if (gpWTPacket((HCTX)hTab, wParam, &pkt)) {

		// old co-ordinates used for comparisons
		POINT 		ptOld = ptNew; 
		UINT  		prsOld = prsNew;
		UINT  		curOld = curNew;
		ORIENTATION ortOld = ortNew;

		// save new co-ordinates/
		ptNew.x = (UINT)pkt.pkX;
		ptNew.y = (UINT)pkt.pkY;
		std::cout << "P(" << pkt.pkX << ", " << pkt.pkY << ")" << std::endl;
		curNew = pkt.pkCursor;
		prsNew = pkt.pkNormalPressure;
		ortNew = pkt.pkOrientation;
		/*
		// If the visual changes update the main graphic
		if (ptNew.x != ptOld.x ||
		ptNew.y != ptOld.y ||
		prsNew != prsOld ||
		ortNew.orAzimuth != ortOld.orAzimuth ||
		ortNew.orAltitude != ortOld.orAltitude ||
		ortNew.orTwist != ortOld.orTwist) {                                     
		InvalidateRect(hWnd, &rcDraw, TRUE);
		}
		// if the displayed data changes update the text
		if (ortNew.orAzimuth != ortOld.orAzimuth ||
		ortNew.orAltitude != ortOld.orAltitude ||
		ortNew.orTwist != ortOld.orTwist) {
		InvalidateRect(hWnd, &rcInfoTilt, TRUE);
		}
		// if the cursor changes update the cursor name
		if (curNew != curOld) {
		InvalidateRect(hWnd, &rcInfoName, TRUE);
		}
		*/

	}



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


