// -*- C++ -*-
/*!
* @file  WacomTablet.cpp
* @brief ModuleDescription
* @date $Date$
*
* $Id$
*/

#define     NAME    "Section1.2 window base"  //タイトルバーに表示するテキスト
#include <Windows.h>

#include "WacomTablet.h"
#include  <math.h>
#include <string.h>
#include <iostream>



#include "Utils.h"

//loadcheckwintab
BOOL            tilt_support = TRUE; /* Is tilt supported */
#define FIX_DOUBLE(x)   ((double)(INT(x))+((double)FRAC(x)/65536))
double          aziFactor = 1;       /* Azimuth factor */
double          altFactor = 1;       /* Altitude factor */
double          altAdjust = 1;       /* Altitude zero adjust */
#define pi 3.14159265359


//getdcwintab
RECT            rcClient;            /* Size of current Client */
RECT            rcInfoTilt;          /* Size of tilt info box */
RECT            rcInfoName;          /* Size of cursor name box */
RECT            rcInfoGen;           /* Size of testing box */
RECT            rcDraw;              /* Size of draw area */

//tabletinit
HANDLE          hInst;               /* Handle for instance */
#define PACKETDATA      (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | \
						 PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE      0
#include "pktdef.h"			// NOTE: get from wactab header package
#include "msgpack.h"

//proc: WM_CREATE
HCTX            hTab = NULL;         /* Handle for Tablet Context */

//proc: WM_PACKET
POINT           ptNew;               /* XY value storage */
UINT            prsNew;              /* Pressure value storage */
UINT            curNew;              /* Cursor number storage */
ORIENTATION     ortNew;              /* Tilt value storage */

//proc: WM_PAINT
#ifdef WIN32                                
#define MoveTo(h,x,y)   MoveToEx(h,x,y,NULL)
#endif
POINT Z1Angle;      /* Rect coords from polar coords */


//from tilttest.h
#define IDM_ABOUT 100

HWND consolehWnd;

//プロトタイプ宣言
LRESULT  WINAPI  WndProc2(HWND, UINT, WPARAM, LPARAM);
int   WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL LoadCheckWintab(void);
BOOL GetDcWintab(HWND        hWnd);
BOOL InitWintab(void);
HWND GetConsoleHwnd(void);
HCTX static NEAR TabletInit(HWND hWnd);


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


HWND GetConsoleHwnd(void){

    #define MY_BUFSIZE 1024 // コンソール ウィンドウのタイトル用のバッファサイズ
    HWND hwndFound;         // 呼び出し側へ返される値
    char pszNewWindowTitle[MY_BUFSIZE];
                           // 作成されるウィンドウのタイトルが入ります
    char pszOldWindowTitle[MY_BUFSIZE]; // 元のウィンドウタイトルが入ります

    // 現在のウィンドウタイトルを取得します

    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    // 独自に、ウィンドウの新規タイトルをフォーマットします

    wsprintf(pszNewWindowTitle,"%d/%d",
             GetTickCount(),
             GetCurrentProcessId());

    // 現在のウィンドウタイトルを変更します

    SetConsoleTitle(pszNewWindowTitle);

    // ウィンドウタイトルのアップデートを確実なものにさせます

    Sleep(40);

    // ウィンドウの新規タイトルを探しにいきます

    hwndFound=FindWindow(NULL, pszNewWindowTitle);

    // 元のウィンドウタイトルへ戻します

    SetConsoleTitle(pszOldWindowTitle);

    return(hwndFound);
   }


BOOL LoadCheckWintab(void){

	char            WName[50];        /* String to hold window name */
	struct          tagAXIS TpOri[3]; /* The capabilities of tilt */
	double          tpvar;            /* A temp for converting fix to double */


	if(!LoadWintab()) return FALSE;
	if (!gpWTInfoA(0, 0, NULL)) return FALSE;
	
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

BOOL GetDcWintab( HWND        hWnd){
	HDC             hDC;              /* Handle for Device Context */
	TEXTMETRIC      textmetric;       /* Structure for font info */
	int             nLineH;           /* Holds the text height */
	int             Xinch, Yinch;     /* Holds the number of pixels per inch */
	int             Hres, Vres;       /* Holds the screen resolution */


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
	
return TRUE;
}


BOOL InitWintab(void){return TRUE;}


 
//winmain message
 MSG         msg;


//Windows Main 関数
int  WINAPI  WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow ){
     HWND        hWnd;
   
	if(!::LoadCheckWintab()) return 0;
    
    // Set up and register window class
    WNDCLASS wc = { CS_CLASSDC,
                   WndProc2,                                //イベントcallback関数
                    0,
                    0,
                    hInstance,
                    NULL,                                   //アイコン
                    LoadCursor( NULL, IDC_ARROW ),          //マウスカーソル
                    (HBRUSH) GetStockObject( WHITE_BRUSH ), //背景色
                    NULL,                                   //メニュー
                    NAME };                                 //クラス名
    if( RegisterClass( &wc ) == 0 ) return FALSE;
    
    //ウインドウ生成
    hWnd = CreateWindow( NAME,                  //タイトルバーテキスト
                         NAME,                  //クラス名
                         WS_OVERLAPPEDWINDOW,   //ウインドウスタイル
                         CW_USEDEFAULT,         //ウインドウ左上x座標
                         CW_USEDEFAULT,         //ウインドウ左上y座標
                         CW_USEDEFAULT,         //ウインドウ幅
                         CW_USEDEFAULT,         //ウインドウ高さ
                         consolehWnd,                  //親ウインドウのハンドル
                         NULL,
                         hInstance,
                         NULL );
    if( !hWnd ) return FALSE;
    //getdcwintab
	if(!GetDcWintab(hWnd)) return FALSE;


    ShowWindow( hWnd, nCmdShow );     //Window を表示
    UpdateWindow( hWnd );             //表示を初期化
    SetFocus( hWnd );                 //フォーカスを設定
    
	if(GetMessage( &msg, NULL, 0, 0 )){
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

    return msg.wParam;
}

//Windws イベント用関数
LRESULT WINAPI WndProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
	PACKET          pkt;             /* the current packet */
	HDC             hDC;             /* handle for Device Context */
	PAINTSTRUCT     psPaint;         /* the paint structure */
    
    //渡された message から、イベントの種類を解析する
    switch(msg){

	case WM_CREATE:
		hTab = TabletInit(hWnd);
		break;
		case WM_PAINT: { /* Paint the window */
			int ZAngle;         /* Raw Altitude */
			UINT Thata;         /* Raw Azimuth */
			double ZAngle2;     /* Adjusted Altitude */
			double Thata2;      /* Adjusted Azimuth */
			
			char szOutput[128]; /* String for outputs */

			if (tilt_support) {                             
				/* 
				   wintab.h defines .orAltitude 
				   as a UINT but documents .orAltitude 
				   as positive for upward angles 
				   and negative for downward angles.
				   WACOM uses negative altitude values to 
				   show that the pen is inverted; 
				   therefore we cast .orAltitude as an 
				   (int) and then use the absolute value. 
				*/
				ZAngle  = (int)ortNew.orAltitude;
				ZAngle2 = altAdjust - (double)abs(ZAngle)/altFactor;
				/* adjust azimuth */
				Thata  = ortNew.orAzimuth;
				Thata2 = (double)Thata/aziFactor;
				/* get the length of the diagnal to draw */  
				Z1Angle.x = (int)(ZAngle2*sin(Thata2));
				Z1Angle.y = (int)(ZAngle2*cos(Thata2));
			}
			else {
				Z1Angle.x = 0;
				Z1Angle.y = 0;
			}

			if (hDC = BeginPaint(hWnd, &psPaint)) {
				
				/* write raw tilt info */ 
				if (tilt_support) {                             
					wsprintf((LPSTR)szOutput,"Tilt: %03i, Thata: %04u\0",
							 ZAngle,Thata);
				}
				else {
				    strcpy(szOutput,"Tilt not supported.");
				}
				DrawText(hDC,szOutput,strlen(szOutput),&rcInfoTilt,DT_LEFT);

				/* write current cursor name */ 
				gpWTInfoA(WTI_CURSORS + curNew, CSR_NAME, szOutput);
				DrawText(hDC,szOutput,strlen(szOutput),&rcInfoName,DT_LEFT);
				
				/* write tablet name */
				gpWTInfoA(WTI_DEVICES, DVC_NAME, szOutput);
				DrawText(hDC,szOutput,strlen(szOutput),&rcInfoGen,DT_LEFT);
				
				/* draw circle based on tablet pressure */
				Ellipse(hDC, ptNew.x - prsNew, ptNew.y - prsNew,
						ptNew.x + prsNew, ptNew.y + prsNew);

				/* draw a line based on tablet tilt */
				MoveTo(hDC,ptNew.x,ptNew.y);
				LineTo(hDC,ptNew.x + Z1Angle.x,ptNew.y - Z1Angle.y);
				
				/* draw CROSS based on tablet position */ 
				MoveTo(hDC,ptNew.x - 20,ptNew.y     );
				LineTo(hDC,ptNew.x + 20,ptNew.y     );
				MoveTo(hDC,ptNew.x     ,ptNew.y - 20);
				LineTo(hDC,ptNew.x     ,ptNew.y + 20);
				EndPaint(hWnd, &psPaint);
			}
			break;
		}

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

    //----終了処理----
    case WM_DESTROY:
		UnloadWintab( );
        PostQuitMessage(0);
        break;
    
    //----デフォルトの処理----
    default :
        return DefWindowProc(hWnd,msg,wParam,lParam);
    }
    
    return 0L;
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

		//***************************************
	//get handle and instance from console
	//consolehWnd=GetConsoleHwnd();

	consolehWnd=::GetConsoleHwnd();
	
	std::cout << "hello" <<std::endl;



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

	WinMain(::GetModuleHandleA(NULL),0,0,SW_SHOW);

	return RTC::RTC_OK;

}



RTC::ReturnCode_t WacomTablet::onDeactivated(RTC::UniqueId ec_id)
{



	return RTC::RTC_OK;
}




RTC::ReturnCode_t WacomTablet::onExecute(RTC::UniqueId ec_id)
{
//	while( GetMessage( &msg, NULL, 0, 0 ) ){
//        TranslateMessage( &msg );
//        DispatchMessage( &msg );
//    }
	GetMessage( &msg, NULL, 0, 0 );
	TranslateMessage( &msg );
	DispatchMessage( &msg );

	std::cout << "now pen is at" << ptNew.x << ", " << ptNew.y << std::endl;
	std::cout << "pressure is" << prsNew << std::endl;
	std::cout << "orientation is" << Z1Angle.x << ", " << Z1Angle.y << std::endl;

	m_position.data.x=ptNew.x;
	m_position.data.y=ptNew.y;
	m_pressure.data=prsNew;
	m_orientation.data.x=Z1Angle.x;
	m_orientation.data.y=Z1Angle.y;

	m_pressureOut.write();
	m_orientationOut.write();
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


