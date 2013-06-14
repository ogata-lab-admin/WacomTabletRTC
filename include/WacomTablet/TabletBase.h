/**
 * @file TabletBase.h
 */

#pragma once

#include "Tablet.h"

#include <Windows.h>

#include "WINTAB.H"


/**
 * タブレット機能クラス
 */
class TabletBase  : public Tablet{
	friend LRESULT WINAPI WindowCallback(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
private:
	HWND m_hWnd;
	HINSTANCE m_hInstance;
	HCTX m_hTablet;

	BOOL m_TiltSupport;       /* Is tilt supported */

	double m_aziFactor;       /* Azimuth factor */
	double m_altFactor;       /* Altitude factor */
	double m_altAdjust;       /* Altitude zero adjust */

	RECT m_rcClient;            /* Size of current Client */
	RECT m_rcInfoTilt;          /* Size of tilt info box */
	RECT m_rcInfoName;          /* Size of cursor name box */
	RECT m_rcInfoGen;           /* Size of testing box */
	RECT m_rcDraw;              /* Size of draw area */

	//proc: WM_PACKET
	POINT           m_ptNew;               /* XY value storage */
	UINT            m_prsNew;              /* Pressure value storage */
	UINT            m_curNew;              /* Cursor number storage */
	ORIENTATION     m_ortNew;              /* Tilt value storage */

#ifdef WIN32                                
#define MoveTo(h,x,y)   MoveToEx(h,x,y,NULL)
#endif
	POINT m_Z1Angle;      /* Rect coords from polar coords */


public:

	/**
	 * コンストラクタ
	 */
	TabletBase(HWND hWnd, HINSTANCE hInstance);

	/**
	 * デストラクタ
	 */
	virtual ~TabletBase();


public:

	/**
	 * データの更新
	 */
	virtual bool updateData(void);

	virtual TabletPoint getPosition() {
		TabletPoint p;
		p.x = m_ptNew.x; p.y = m_ptNew.y;
		return p;
	}

	virtual uint32_t getPressure() {return m_prsNew;}

	virtual TabletPoint getZAngle() {
		TabletPoint p;
		p.x = m_Z1Angle.x; p.y = m_Z1Angle.y;
		return p;}

	virtual TabletOrientation getOrientation(){
		TabletOrientation o;
		o.orAltitude = m_ortNew.orAltitude;
		o.orAzimuth  = m_ortNew.orAzimuth;
		o.orTwist    = m_ortNew.orTwist;
		return o;}

private:
	BOOL LoadCheckWintab(void);
	BOOL GetDcWintab(void);
	BOOL TabletInit(void);
	BOOL ProcessMessage(UINT msg, WPARAM wParam, LPARAM lParam);
public:


};