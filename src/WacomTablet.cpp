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

//from tilttest.h
#define IDM_ABOUT 100

HWND consolehWnd;

//プロトタイプ宣言
LRESULT  WINAPI  WndProc2(HWND, UINT, WPARAM, LPARAM);
int   WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);



   HWND GetConsoleHwnd(void)
   {
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




//Windows Main 関数
int  WINAPI  WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow ){
    HWND        hWnd;
    MSG         msg;
    
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
    
    ShowWindow( hWnd, nCmdShow );     //Window を表示
    UpdateWindow( hWnd );             //表示を初期化
    SetFocus( hWnd );                 //フォーカスを設定
    
    while( GetMessage( &msg, NULL, 0, 0 ) ){
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}

//Windws イベント用関数
LRESULT WINAPI WndProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    //渡された message から、イベントの種類を解析する
    switch(msg){
    //----終了処理----
    case WM_DESTROY:
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
	consolehWnd=GetConsoleHwnd();

	WinMain(::GetModuleHandleA(NULL),0,0,SW_SHOW);
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



	return RTC::RTC_OK;
}



RTC::ReturnCode_t WacomTablet::onDeactivated(RTC::UniqueId ec_id)
{
	


	return RTC::RTC_OK;
}




RTC::ReturnCode_t WacomTablet::onExecute(RTC::UniqueId ec_id)
{

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


