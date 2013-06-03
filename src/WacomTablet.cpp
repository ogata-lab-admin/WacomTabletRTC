// -*- C++ -*-
/*!
* @file  WacomTablet.cpp
* @brief ModuleDescription
* @date $Date$
*
* $Id$
*/

#define     NAME    "Section1.2 window base"  //�^�C�g���o�[�ɕ\������e�L�X�g
#include <Windows.h>

#include "WacomTablet.h"
#include  <math.h>
#include <string.h>
#include <iostream>

//from tilttest.h
#define IDM_ABOUT 100

HWND consolehWnd;

//�v���g�^�C�v�錾
LRESULT  WINAPI  WndProc2(HWND, UINT, WPARAM, LPARAM);
int   WINAPI     WinMain(HINSTANCE, HINSTANCE, LPSTR, int);



   HWND GetConsoleHwnd(void)
   {
    #define MY_BUFSIZE 1024 // �R���\�[�� �E�B���h�E�̃^�C�g���p�̃o�b�t�@�T�C�Y
    HWND hwndFound;         // �Ăяo�����֕Ԃ����l
    char pszNewWindowTitle[MY_BUFSIZE];
                           // �쐬�����E�B���h�E�̃^�C�g��������܂�
    char pszOldWindowTitle[MY_BUFSIZE]; // ���̃E�B���h�E�^�C�g��������܂�

    // ���݂̃E�B���h�E�^�C�g�����擾���܂�

    GetConsoleTitle(pszOldWindowTitle, MY_BUFSIZE);

    // �Ǝ��ɁA�E�B���h�E�̐V�K�^�C�g�����t�H�[�}�b�g���܂�

    wsprintf(pszNewWindowTitle,"%d/%d",
             GetTickCount(),
             GetCurrentProcessId());

    // ���݂̃E�B���h�E�^�C�g����ύX���܂�

    SetConsoleTitle(pszNewWindowTitle);

    // �E�B���h�E�^�C�g���̃A�b�v�f�[�g���m���Ȃ��̂ɂ����܂�

    Sleep(40);

    // �E�B���h�E�̐V�K�^�C�g����T���ɂ����܂�

    hwndFound=FindWindow(NULL, pszNewWindowTitle);

    // ���̃E�B���h�E�^�C�g���֖߂��܂�

    SetConsoleTitle(pszOldWindowTitle);

    return(hwndFound);
   }




//Windows Main �֐�
int  WINAPI  WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, int nCmdShow ){
    HWND        hWnd;
    MSG         msg;
    
    // Set up and register window class
    WNDCLASS wc = { CS_CLASSDC,
                   WndProc2,                                //�C�x���gcallback�֐�
                    0,
                    0,
                    hInstance,
                    NULL,                                   //�A�C�R��
                    LoadCursor( NULL, IDC_ARROW ),          //�}�E�X�J�[�\��
                    (HBRUSH) GetStockObject( WHITE_BRUSH ), //�w�i�F
                    NULL,                                   //���j���[
                    NAME };                                 //�N���X��
    if( RegisterClass( &wc ) == 0 ) return FALSE;
    
    //�E�C���h�E����
    hWnd = CreateWindow( NAME,                  //�^�C�g���o�[�e�L�X�g
                         NAME,                  //�N���X��
                         WS_OVERLAPPEDWINDOW,   //�E�C���h�E�X�^�C��
                         CW_USEDEFAULT,         //�E�C���h�E����x���W
                         CW_USEDEFAULT,         //�E�C���h�E����y���W
                         CW_USEDEFAULT,         //�E�C���h�E��
                         CW_USEDEFAULT,         //�E�C���h�E����
                         consolehWnd,                  //�e�E�C���h�E�̃n���h��
                         NULL,
                         hInstance,
                         NULL );
    if( !hWnd ) return FALSE;
    
    ShowWindow( hWnd, nCmdShow );     //Window ��\��
    UpdateWindow( hWnd );             //�\����������
    SetFocus( hWnd );                 //�t�H�[�J�X��ݒ�
    
    while( GetMessage( &msg, NULL, 0, 0 ) ){
        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}

//Windws �C�x���g�p�֐�
LRESULT WINAPI WndProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam){
    
    //�n���ꂽ message ����A�C�x���g�̎�ނ���͂���
    switch(msg){
    //----�I������----
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    
    //----�f�t�H���g�̏���----
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


