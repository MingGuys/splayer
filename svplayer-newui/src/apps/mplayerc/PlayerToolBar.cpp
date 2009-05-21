/* 
 *	Copyright (C) 2003-2006 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// PlayerToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "mplayerc.h"
#include <math.h>
#include <atlbase.h>
#include <afxpriv.h>
#include "PlayerToolBar.h"
#include "MainFrm.h"

typedef HRESULT (__stdcall * SetWindowThemeFunct)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);

// CPlayerToolBar

IMPLEMENT_DYNAMIC(CPlayerToolBar, CToolBar)
CPlayerToolBar::CPlayerToolBar() :
m_hovering(0),
iButtonWidth (30)
{

}

CPlayerToolBar::~CPlayerToolBar()
{
}

BOOL CPlayerToolBar::Create(CWnd* pParentWnd)
{
	int iToolBarID = IDB_PLAYERTOOLBAR;
	/*
	CRect rcDesktop;
		GetDesktopWindow()->GetWindowRect(&rcDesktop);
	
		if( rcDesktop.Width() < 1200){
			iToolBarID = IDB_PLAYERTOOLBAR_SMALL;
			iButtonWidth = 20;
		}*/
	

	if(!__super::CreateEx(pParentWnd,
		TBSTYLE_FLAT|TBSTYLE_TRANSPARENT|TBSTYLE_AUTOSIZE,
		WS_CHILD|WS_VISIBLE|CBRS_ALIGN_BOTTOM , CRect(0,0,0,0))  //CBRS_TOOLTIPS NEW UI
	) //|| !LoadToolBar(iToolBarID)
		return FALSE;

	GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	CSUIButton* btnPlay = new CSUIButton(L"BTN_PLAY.BMP" , ALIGN_TOPLEFT, CRect(-50 , 10, 3,3)  , 0, ID_PLAY_PLAY, FALSE, 0, 0 );
	//btnPlay->m_stat = 3; //disabled
	m_btnList.AddTail( btnPlay );

	CSUIButton* btnPause = new CSUIButton(L"BTN_PAUSE.BMP" , ALIGN_TOPLEFT, CRect(-50 , 10, 3,3)  , 0, ID_PLAY_PAUSE, TRUE, 0, 0 );
	//btnPlay->m_stat = 3; //disabled
	m_btnList.AddTail( btnPause );

	cursorHand = ::LoadCursor(NULL, IDC_HAND);
	/*
		CToolBarCtrl& tb = GetToolBarCtrl();
			tb.DeleteButton(tb.GetButtonCount()-1);
			tb.DeleteButton(tb.GetButtonCount()-1);
		
			SetMute(AfxGetAppSettings().fMute);
		
			UINT styles[] = 
			{
				TBBS_CHECKGROUP, TBBS_CHECKGROUP, TBBS_CHECKGROUP, 
				TBBS_SEPARATOR,
				TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, 
				//TBBS_SEPARATOR,
				//TBBS_BUTTON/ *|TBSTYLE_DROPDOWN* /, 
				TBBS_SEPARATOR,
				TBBS_BUTTON, TBBS_BUTTON, TBBS_BUTTON, 
				TBBS_SEPARATOR,
				TBBS_BUTTON, TBBS_BUTTON, TBBS_SEPARATOR, 
				TBBS_BUTTON,
				TBBS_BUTTON,
				TBBS_DISABLED,TBBS_DISABLED,
				TBBS_CHECKBOX, 
				/ *TBBS_SEPARATOR,* /
			};
		
			for(int i = 0; i < countof(styles); i++)
				SetButtonStyle(i, styles[i]|TBBS_DISABLED);*/
		

	/*
	SetButtonStyle(0, GetButtonStyle(0)|BS_ICON );
		CWnd* hWndBtn = GetToolBarCtrl().GetDlgItem( ID_PLAY_PLAY );
		if(hWndBtn){
			HICON hIcon = (HICON)LoadImage(AfxGetApp()->m_hInstance,  MAKEINTRESOURCE(IDI_PLAY), IMAGE_ICON, 32, 32, LR_SHARED);
			if(hIcon){
				::SendMessage( hWndBtn->m_hWnd,  BM_SETIMAGE,      IMAGE_ICON , (LPARAM)(hIcon) );  
				DestroyIcon(hIcon);
			}else{
				AfxMessageBox(_T("f"));
			}
		}else{
			AfxMessageBox(_T("s"));
		}*/
	/*
	CWinApp* aa = AfxGetApp();
		m_ToolBarImages.Create(32, 32, ILC_COLOR32, 4, 4);
		m_ToolBarImages.Add(aa->LoadIcon(IDI_PLAY));
	
		m_ToolBarDisabledImages.Create(32, 32, ILC_COLOR32, 4, 4);
		m_ToolBarDisabledImages.Add(aa->LoadIcon(IDI_PLAY));
	
		tb.SetImageList(&m_ToolBarImages);
		tb.SetDisabledImageList(&m_ToolBarDisabledImages);
	*/
	

	

	
		m_volctrl.Create(this);
		/*
		if(AfxGetAppSettings().fDisabeXPToolbars)
		{
			if(HMODULE h = LoadLibrary(_T("uxtheme.dll")))
			{
				SetWindowThemeFunct f = (SetWindowThemeFunct)GetProcAddress(h, "SetWindowTheme");
				if(f) f(m_hWnd, L" ", L" ");
				FreeLibrary(h);
			}
		}*/
	

	return TRUE;
}

BOOL CPlayerToolBar::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!__super::PreCreateWindow(cs))
		return FALSE;

	m_dwStyle &= ~CBRS_BORDER_TOP;
	m_dwStyle &= ~CBRS_BORDER_LEFT;
	m_dwStyle &= ~CBRS_BORDER_RIGHT;
	m_dwStyle &= ~CBRS_BORDER_BOTTOM;
//	m_dwStyle |= CBRS_SIZE_FIXED;

	return TRUE;
}

void CPlayerToolBar::ArrangeControls()
{
	if(!::IsWindow(m_volctrl.m_hWnd)) return;

	/*
	CRect r;
		GetClientRect(&r);
	
		CRect br = GetBorders();
	
		CRect r10;
		GetItemRect(18, &r10);
	
		CRect vr;
		m_volctrl.GetClientRect(&vr);
		CRect vr2(r.right+br.right-iButtonWidth*2, r.top-1, r.right+br.right+6, r.bottom);
		m_volctrl.MoveWindow(vr2);
	
		UINT nID;
		UINT nStyle;
		int iImage;
		GetButtonInfo(20, nID, nStyle, iImage);
		SetButtonInfo(19, GetItemID(19), TBBS_SEPARATOR, vr2.left - iImage - r10.right - 19);*/
	
}

void CPlayerToolBar::SetMute(bool fMute)
{
	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	bi.iImage = fMute?21:20;
	tb.SetButtonInfo(ID_VOLUME_MUTE, &bi);

	AfxGetAppSettings().fMute = fMute;
}

bool CPlayerToolBar::IsMuted()
{
	CToolBarCtrl& tb = GetToolBarCtrl();
	TBBUTTONINFO bi;
	bi.cbSize = sizeof(bi);
	bi.dwMask = TBIF_IMAGE;
	tb.GetButtonInfo(ID_VOLUME_MUTE, &bi);
	return(bi.iImage==21);
}

int CPlayerToolBar::GetVolume()
{
	int volume = m_volctrl.GetPos();
	volume = (int)((log10(1.0*volume)-2)*5000);
	volume = max(min(volume, 0), -10000);
	return(IsMuted() ? -10000 : volume);
}

void CPlayerToolBar::SetVolume(int volume)
{
/*
	volume = (int)pow(10, ((double)volume)/5000+2);
	volume = max(min(volume, 100), 1);
*/
	m_volctrl.SetPosInternal(volume);
}

BEGIN_MESSAGE_MAP(CPlayerToolBar, CToolBar)
	ON_MESSAGE_VOID(WM_INITIALUPDATE, OnInitialUpdate)
	ON_COMMAND_EX(ID_VOLUME_MUTE, OnVolumeMute)
	ON_UPDATE_COMMAND_UI(ID_VOLUME_MUTE, OnUpdateVolumeMute)
	ON_COMMAND_EX(ID_VOLUME_UP, OnVolumeUp)
	ON_COMMAND_EX(ID_VOLUME_DOWN, OnVolumeDown)
	ON_WM_NCPAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_NCCALCSIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

// CPlayerToolBar message handlers
#define NEWUI_COLOR_BG  RGB(214,214,214)
#define NEWUI_COLOR_TOOLBAR_UPPERBG  RGB(0x17,0x17,0x17)


BOOL CPlayerToolBar::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message){
	
	CPoint pt;
	::GetCursorPos (&pt);
	ScreenToClient (&pt);

	if(m_nItemToTrack){	
		SetCursor(cursorHand );
		return TRUE;
	}
	return CWnd::OnSetCursor(pWnd, 0, 0);
}

CSize CPlayerToolBar::CalcFixedLayout(BOOL bStretch,BOOL bHorz ){

	
	CSize size( 32767, 36 );

	if ( CWnd* pParent = AfxGetMainWnd() )
	{
		CRect rc;
		pParent->GetWindowRect( &rc );
		size.cx = rc.Width() - 2;
	}

	CRect rc;
	GetWindowRect(&rc);
	m_btnList.OnSize( rc);

	return size;



	//return __super::CalcFixedLayout(bStretch,bHorz);
}
void CPlayerToolBar::OnPaint()
{

	CPaintDC dc(this); // device context for painting
	CRect rcClient;
	GetClientRect(&rcClient);
	CMemoryDC hdc(&dc, rcClient);
	CRect rcBottomSqu = rcClient;
	rcBottomSqu.top = rcBottomSqu.bottom - 10;
	hdc.FillSolidRect(rcBottomSqu, NEWUI_COLOR_BG);

	CRect rcUpperSqu = rcClient;
	rcUpperSqu.bottom = rcUpperSqu.bottom - 10;
	hdc.FillSolidRect(rcUpperSqu, NEWUI_COLOR_TOOLBAR_UPPERBG);

 	CRect rc;
	GetWindowRect(&rc);
	UpdateButtonStat();
 	m_btnList.PaintAll(&hdc, rc);
}
void CPlayerToolBar::UpdateButtonStat(){
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	BOOL fShow = pFrame->GetUIStat( ID_PLAY_PAUSE );
	m_btnList.SetHideStat( ID_PLAY_PLAY , fShow );
	m_btnList.SetHideStat( ID_PLAY_PAUSE , !fShow );
}
void CPlayerToolBar::OnNcPaint() // when using XP styles the NC area isn't drawn for our toolbar...
{
	//New UI GetSysColor(COLOR_BTNFACE)

	//��䱳��-----------------------------------------   
	//dc.FillSolidRect(wr, RGB(214,219,239) );   

	// Do not call CToolBar::OnNcPaint() for painting messages
}
void CPlayerToolBar::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);

	ArrangeControls();

	CRect rc;
	GetWindowRect(&rc);
	m_btnList.OnSize( rc);
}

void CPlayerToolBar::OnInitialUpdate()
{
	ArrangeControls();
}

BOOL CPlayerToolBar::OnVolumeMute(UINT nID)
{
	SetMute(!IsMuted()); 
	return FALSE;
}

void CPlayerToolBar::OnUpdateVolumeMute(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(true);
	pCmdUI->SetCheck(IsMuted());
}

BOOL CPlayerToolBar::OnVolumeUp(UINT nID)
{
	m_volctrl.IncreaseVolume();
	return FALSE;
}

BOOL CPlayerToolBar::OnVolumeDown(UINT nID)
{
	m_volctrl.DecreaseVolume();
	return FALSE;
}

void CPlayerToolBar::OnMouseMove(UINT nFlags, CPoint point){
	CRect rc;
	GetWindowRect(&rc);

	point += rc.TopLeft() ;
	UINT ret = m_btnList.OnHitTest(point,rc);
	m_nItemToTrack = ret;
	if( m_btnList.HTRedrawRequired ){
		Invalidate();
	}
	
	return;
}

void CPlayerToolBar::OnLButtonDown(UINT nFlags, CPoint point)
{
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
	
	CRect rc;
	GetWindowRect(&rc);

	point += rc.TopLeft() ;
	UINT ret = m_btnList.OnHitTest(point,rc);
	if( m_btnList.HTRedrawRequired ){
		if(ret)
			SetCapture();
		Invalidate();
	}
	m_nItemToTrack = ret;
	
	return;
	//New UI End
	iBottonClicked = -1;
	KillTimer(TIMER_FASTFORWORD);

	for(int i = 0, j = GetToolBarCtrl().GetButtonCount(); i < j; i++)
	{
		if(GetButtonStyle(i)&(TBBS_SEPARATOR|TBBS_DISABLED))
			continue;

		CRect r;
		GetItemRect(i, r);
		if(r.PtInRect(point))
		{
			UINT iButtonID , iStyle ;
			int iImage ;
			CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
			GetButtonInfo(i,iButtonID,iStyle,iImage );
			if(iButtonID == ID_PLAY_BWD || iButtonID == ID_PLAY_FWD){
				//pFrame->PostMessage( WM_COMMAND, ID_PLAY_PAUSE);
				iBottonClicked = iButtonID;
				iFastFFWCount = 0;
				SetTimer(TIMER_FASTFORWORD, 350, NULL);
			}else if(iButtonID == ID_SUBDELAYDEC || iButtonID == ID_SUBDELAYINC){
				iBottonClicked = iButtonID;
				iFastFFWCount = 0;
				SetTimer(TIMER_FASTFORWORD, 350, NULL);
			}
			__super::OnLButtonDown(nFlags, point);
			
			return;
		}
	} 

	if(!pFrame->m_fFullScreen)
	{
		MapWindowPoints(pFrame, &point, 1);
		pFrame->PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
	}
}

void CPlayerToolBar::OnLButtonUp(UINT nFlags, CPoint point)
{
	CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());

	ReleaseCapture();

	CRect rc;
	GetWindowRect(&rc);

	CPoint xpoint = point + rc.TopLeft() ;
	UINT ret = m_btnList.OnHitTest(xpoint,rc);
	if( m_btnList.HTRedrawRequired ){
		if(ret)
			pFrame->PostMessage( WM_COMMAND, ret);
		Invalidate();
	}
	m_nItemToTrack = ret;
//	__super::OnLButtonUp(nFlags, point);
	return;//New UI End

	KillTimer(TIMER_FASTFORWORD);
	for(int i = 0, j = GetToolBarCtrl().GetButtonCount(); i < j; i++)
	{
		CRect r;
		GetItemRect(i, r);
		if(r.PtInRect(point))
		{
			UINT iButtonID, iStyle ;
			int iImage ;
			GetButtonInfo(i,iButtonID, iStyle , iImage );
			if(iButtonID == iBottonClicked ){
				if(iFastFFWCount == 0){
					int iMsg = 0;
					CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
					// not increase or decrease play rate
					if(iBottonClicked == ID_PLAY_BWD){
						iMsg = ID_PLAY_SEEKBACKWARDSMALL;
					}else if(iBottonClicked == ID_PLAY_FWD){
						iMsg = ID_PLAY_SEEKFORWARDSMALL;
					}
					if(iMsg)
						pFrame->PostMessage( WM_COMMAND, iMsg);
					// 					if( iBottonClicked == ID_PLAY_BWD || iBottonClicked == ID_PLAY_FWD) 
					// 						pFrame->PostMessage( WM_COMMAND, ID_PLAY_PLAY);
				}
			}
			break;
		}
	}
	iBottonClicked = -1;

	__super::OnLButtonUp(nFlags, point);
	return;
}
void CPlayerToolBar::OnTimer(UINT nIDEvent){
	switch(nIDEvent){
		case TIMER_STOPFASTFORWORD:
			iFastFFWCount = 0;
			KillTimer(TIMER_STOPFASTFORWORD);
			{
// 				CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
// 				pFrame->PostMessage( WM_COMMAND, ID_PLAY_PLAY);
			}
			break;
		case TIMER_FASTFORWORD:
			if(iBottonClicked < 0 ){
				KillTimer(TIMER_FASTFORWORD);
				break;
			}
			iFastFFWCount++;
			//fast forward or backword
			{
				CMainFrame* pFrame = ((CMainFrame*)GetParentFrame());
				int iMsg;
				if(iBottonClicked == ID_PLAY_BWD){
					iMsg = ID_PLAY_SEEKBACKWARDSMALL;
				}else if(iBottonClicked == ID_PLAY_FWD){
					iMsg = ID_PLAY_SEEKFORWARDSMALL;
				}else{
					iMsg = iBottonClicked;
				}
				if(iFastFFWCount > 5 && ( iBottonClicked == ID_PLAY_BWD || iBottonClicked == ID_PLAY_FWD) ){

					int iStepPow = (int)(iFastFFWCount / 5) * 2;
					iStepPow = min(8, iStepPow);
					iMsg += iStepPow;
				}
				pFrame->PostMessage( WM_COMMAND, iMsg);
			}
			break;
	}

	__super::OnTimer(nIDEvent);
}