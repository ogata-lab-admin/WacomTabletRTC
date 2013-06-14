/**
 * @file TabletBase.cpp
 */


#include "TabletBase.h"
#include <Windows.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

#include "Utils.h"

#define     NAME    "WacomTabletRTC"  //タイトルバーに表示するテキスト
#define FIX_DOUBLE(x)   ((double)(INT(x))+((double)FRAC(x)/65536))
#define PACKETDATA      (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE | \
	PK_ORIENTATION | PK_CURSOR)
#define PACKETMODE      0
#include "pktdef.h"			// NOTE: get from wactab header package
#include "msgpack.h"

static LRESULT  WINAPI  WindowCallback(HWND, UINT, WPARAM, LPARAM);


/**
 * コンストラクタ
 */
TabletBase::TabletBase(HWND hWnd, HINSTANCE hInstance) :
m_TiltSupport(TRUE), m_aziFactor(1), m_altFactor(1), m_altAdjust(1)
{
	m_hInstance = hInstance;
	if(!LoadCheckWintab()) throw TabletException("Can not load WINTAB");

	// Set up and register window class
	WNDCLASS wc = { CS_CLASSDC,
		WindowCallback,                                //イベントcallback関数
		0,
		0,
		hInstance,
		NULL,                                   //アイコン
		LoadCursor( NULL, IDC_ARROW ),          //マウスカーソル
		(HBRUSH) GetStockObject( WHITE_BRUSH ), //背景色
		NULL,                                   //メニュー
		NAME };                                 //クラス名
	if( RegisterClass( &wc ) == 0 ) throw TabletException("Can not register WindClass.");

	//ウインドウ生成
	m_hWnd = ::CreateWindow( NAME,                  //タイトルバーテキスト
		NAME,                  //クラス名
		WS_OVERLAPPEDWINDOW,   //ウインドウスタイル
		CW_USEDEFAULT,         //ウインドウ左上x座標
		CW_USEDEFAULT,         //ウインドウ左上y座標
		CW_USEDEFAULT,         //ウインドウ幅
		CW_USEDEFAULT,         //ウインドウ高さ
		hWnd,                  //親ウインドウのハンドル
		NULL,
		hInstance,
		this );                 // WParam
	if( !m_hWnd ) throw TabletException("Can not create window.");
	//getdcwintab
	if(!GetDcWintab()) throw TabletException("Can not get DC for Tablet.");


	ShowWindow( m_hWnd, SW_SHOW );     //Window を表示
	UpdateWindow( m_hWnd );             //表示を初期化
	SetFocus( m_hWnd );                 //フォーカスを設定

	/*
	if(GetMessage( &msg, NULL, 0, 0 )){
	TranslateMessage( &msg );
	DispatchMessage( &msg );
	}
	*/
}


/**
 */
TabletBase::~TabletBase() {

}


bool TabletBase::updateData(void)
{
	MSG msg;
	GetMessage( &msg, NULL, 0, 0 );
	TranslateMessage( &msg );
	DispatchMessage( &msg );
	return TRUE;
}


BOOL TabletBase::TabletInit()
{
	LOGCONTEXT      lcMine;           /* The context of the tablet */
	AXIS            TabletX, TabletY; /* The maximum tablet size */

	/* get default region */
	gpWTInfoA(WTI_DEFCONTEXT, 0, &lcMine);

	/* modify the digitizing region */
	wsprintf(lcMine.lcName, "TiltTest Digitizing %x", m_hInstance);
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
	m_hTablet = gpWTOpenA(m_hWnd, &lcMine, TRUE);


	return TRUE;
}


BOOL TabletBase::LoadCheckWintab(void){
	//	char            WName[50];        /* String to hold window name */
	struct          tagAXIS TpOri[3]; /* The capabilities of tilt */
	double          tpvar;            /* A temp for converting fix to double */

	if(!LoadWintab()) return FALSE;
	if (!gpWTInfoA(0, 0, NULL)) return FALSE;

	/* get info about tilt */
	m_TiltSupport = gpWTInfoA(WTI_DEVICES,DVC_ORIENTATION,&TpOri);
	if (m_TiltSupport) {
		/* does the tablet support azimuth and altitude */
		if (TpOri[0].axResolution && TpOri[1].axResolution) {

			/* convert azimuth resulution to double */
			tpvar = FIX_DOUBLE(TpOri[0].axResolution);
			/* convert from resolution to radians */
			m_aziFactor = tpvar/(2*M_PI);  

			/* convert altitude resolution to double */
			tpvar = FIX_DOUBLE(TpOri[1].axResolution);
			/* scale to arbitrary value to get decent line length */ 
			m_altFactor = tpvar/1000; 
			/* adjust for maximum value at vertical */
			m_altAdjust = (double)TpOri[1].axMax/m_altFactor;
		}
		else {  /* no so dont do tilt stuff */
			m_TiltSupport = FALSE;
		}
	}
	return TRUE;
}

BOOL TabletBase::GetDcWintab(){
	HDC             hDC;              /* Handle for Device Context */
	TEXTMETRIC      textmetric;       /* Structure for font info */
	int             nLineH;           /* Holds the text height */
	int             Xinch, Yinch;     /* Holds the number of pixels per inch */
	int             Hres, Vres;       /* Holds the screen resolution */


	hDC = GetDC(m_hWnd);
	if (!hDC)
		return FALSE;
	GetTextMetrics(hDC, &textmetric);
	nLineH = textmetric.tmExternalLeading + textmetric.tmHeight;
	Xinch = GetDeviceCaps(hDC, LOGPIXELSX);
	Yinch = GetDeviceCaps(hDC, LOGPIXELSY);
	Hres = GetDeviceCaps(hDC, HORZRES);
	Vres = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(m_hWnd, hDC);

	GetClientRect(m_hWnd, &m_rcClient);
	m_rcInfoTilt = m_rcClient;
	m_rcInfoTilt.left   = Xinch / 8;
	m_rcInfoTilt.top    = Yinch / 8;
	m_rcInfoTilt.bottom = m_rcInfoTilt.top + nLineH;
	m_rcInfoName = m_rcInfoTilt;
	m_rcInfoName.top    += nLineH;
	m_rcInfoName.bottom += nLineH;
	m_rcInfoGen = m_rcInfoName;
	m_rcInfoGen.top    += nLineH;
	m_rcInfoGen.bottom += nLineH;
	m_rcDraw = m_rcInfoGen;
	m_rcDraw.left   = 0;
	m_rcDraw.top   += nLineH;
	m_rcDraw.bottom = m_rcClient.bottom;

	return TRUE;
}

BOOL TabletBase::ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam) 
{
	PACKET          pkt;             /* the current packet */
	HDC             hDC;             /* handle for Device Context */
	PAINTSTRUCT     psPaint;         /* the paint structure */

	//渡された message から、イベントの種類を解析する
	switch(msg){

	case WM_PAINT: { /* Paint the window */
		int ZAngle;         /* Raw Altitude */
		UINT Thata;         /* Raw Azimuth */
		double ZAngle2;     /* Adjusted Altitude */
		double Thata2;      /* Adjusted Azimuth */

		char szOutput[128]; /* String for outputs */

		if (m_TiltSupport) {                             
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
			ZAngle  = (int)m_ortNew.orAltitude;
			ZAngle2 = m_altAdjust - (double)abs(ZAngle)/m_altFactor;
			/* adjust azimuth */
			Thata  = m_ortNew.orAzimuth;
			Thata2 = (double)Thata/m_aziFactor;
			/* get the length of the diagnal to draw */  
			m_Z1Angle.x = (int)(ZAngle2*sin(Thata2));
			m_Z1Angle.y = (int)(ZAngle2*cos(Thata2));
		}
		else {
			m_Z1Angle.x = 0;
			m_Z1Angle.y = 0;
		}

		if (hDC = BeginPaint(m_hWnd, &psPaint)) {

			/* write raw tilt info */ 
			if (m_TiltSupport) {                             
				wsprintf((LPSTR)szOutput,"Tilt: %03i, Thata: %04u\0",
					ZAngle,Thata);
			}
			else {
				strcpy(szOutput,"Tilt not supported.");
			}
			DrawText(hDC,szOutput,strlen(szOutput),&m_rcInfoTilt,DT_LEFT);

			/* write current cursor name */ 
			gpWTInfoA(WTI_CURSORS + m_curNew, CSR_NAME, szOutput);
			DrawText(hDC,szOutput,strlen(szOutput),&m_rcInfoName,DT_LEFT);

			/* write tablet name */
			gpWTInfoA(WTI_DEVICES, DVC_NAME, szOutput);
			DrawText(hDC,szOutput,strlen(szOutput),&m_rcInfoGen,DT_LEFT);

			/* draw circle based on tablet pressure */
			Ellipse(hDC, m_ptNew.x - m_prsNew, m_ptNew.y - m_prsNew,
				m_ptNew.x + m_prsNew, m_ptNew.y + m_prsNew);

			/* draw a line based on tablet tilt */
			MoveTo(hDC, m_ptNew.x, m_ptNew.y);
			LineTo(hDC, m_ptNew.x + m_Z1Angle.x, m_ptNew.y - m_Z1Angle.y);

			/* draw CROSS based on tablet position */ 
			MoveTo(hDC, m_ptNew.x - 20, m_ptNew.y     );
			LineTo(hDC, m_ptNew.x + 20, m_ptNew.y     );
			MoveTo(hDC, m_ptNew.x     , m_ptNew.y - 20);
			LineTo(hDC, m_ptNew.x     , m_ptNew.y + 20);
			EndPaint(m_hWnd, &psPaint);
		}
		break;
				   }

	case WT_PACKET: /* A packet is waiting from WINTAB */
		if (gpWTPacket((HCTX)lParam, wParam, &pkt)) {

			/* old co-ordinates used for comparisons */
			POINT 		ptOld = m_ptNew; 
			UINT  		prsOld = m_prsNew;
			UINT  		curOld = m_curNew;
			ORIENTATION ortOld = m_ortNew;

			/* save new co-ordinates */
			m_ptNew.x = (UINT)pkt.pkX;
			m_ptNew.y = (UINT)pkt.pkY;
			m_curNew = pkt.pkCursor;
			m_prsNew = pkt.pkNormalPressure;
			m_ortNew = pkt.pkOrientation;

			/* If the visual changes update the main graphic */
			if (m_ptNew.x != ptOld.x ||
				m_ptNew.y != ptOld.y ||
				m_prsNew != prsOld ||
				m_ortNew.orAzimuth != ortOld.orAzimuth ||
				m_ortNew.orAltitude != ortOld.orAltitude ||
				m_ortNew.orTwist != ortOld.orTwist) {                                     
					InvalidateRect(m_hWnd, &m_rcDraw, TRUE);
			}
			/* if the displayed data changes update the text */
			if (m_ortNew.orAzimuth != ortOld.orAzimuth ||
				m_ortNew.orAltitude != ortOld.orAltitude ||
				m_ortNew.orTwist != ortOld.orTwist) {
					InvalidateRect(m_hWnd, &m_rcInfoTilt, TRUE);
			}
			/* if the cursor changes update the cursor name */
			if (m_curNew != curOld) {
				InvalidateRect(m_hWnd, &m_rcInfoName, TRUE);
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
		return DefWindowProc(m_hWnd,msg,wParam,lParam);
	}

	return 0L;
}

//Windws イベント用関数
LRESULT WINAPI WindowCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	if (msg == WM_CREATE) {
		TabletBase *pTablet = reinterpret_cast<TabletBase*>(lParam);
		pTablet->TabletInit();
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pTablet);
	} else {
		LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		TabletBase *pTablet = reinterpret_cast<TabletBase*>(ptr);
		return pTablet->ProcessMessage(msg, wParam, lParam);
	}
	return 0L;
}
