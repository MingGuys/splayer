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

// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mplayerc.h"

#include "MainFrm.h"

#include <math.h>

#include <afxpriv.h>

#include <atlconv.h>
#include "..\..\..\lib\ATL Server\include\atlrx.h"
#include <atlsync.h>

#include "OpenFileDlg.h"
#include "OpenDlg.h"
#include "OpenURLDlg.h"
#include "SaveDlg.h"
#include "GoToDlg.h"
#include "PnSPresetsDlg.h"
#include "MediaTypesDlg.h"
#include "SaveTextFileDialog.h"
#include "SaveThumbnailsDialog.h"
#include "FavoriteAddDlg.h"
#include "FavoriteOrganizeDlg.h"
#include "ConvertDlg.h"
#include "ShaderCombineDlg.h"
#include "UESettingPanel.h"

#include <mtype.h>
#include <Mpconfig.h>
#include <ks.h>
#include <ksmedia.h>
#include <dvdevcod.h>
#include <dsound.h>

#include <initguid.h>
#include <uuids.h>
#include "..\..\..\include\moreuuids.h"
#include <Qnetwork.h>
//#include <qedit.h>

#include "..\..\DSUtil\DSUtil.h"
#include "FGManager.h"

#include "textpassthrufilter.h"
#include "..\..\filters\filters.h"
#include "..\..\filters\PinInfoWnd.h"

#include "DX7AllocatorPresenter.h"
#include "DX9AllocatorPresenter.h"
#include "EVRAllocatorPresenter.h"

#include "..\..\subtitles\SSF.h"
#include "SVPSubDownUpDialog.h"
#include "SVPSubUploadDlg.h"

#include "revision.h"
#include "ChkDefPlayer.h"
#include "DlgChkUpdater.h"
#include "InfoReport.h"

#define DEFCLIENTW 480
#define DEFCLIENTH 360

static UINT s_uTaskbarRestart = RegisterWindowMessage(TEXT("TaskbarCreated"));
static UINT WM_NOTIFYICON = RegisterWindowMessage(TEXT("MYWM_NOTIFYICON"));

#include "..\..\filters\transform\vsfilter\IDirectVobSub.h"

#include "..\..\svplib\SVPToolBox.h"

class CSubClock : public CUnknown, public ISubClock
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			QI(ISubClock)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	REFERENCE_TIME m_rt;

public:
	CSubClock() : CUnknown(NAME("CSubClock"), NULL) {m_rt = 0;}

	DECLARE_IUNKNOWN;

	// ISubClock
	STDMETHODIMP SetTime(REFERENCE_TIME rt) {m_rt = rt; return S_OK;}
	STDMETHODIMP_(REFERENCE_TIME) GetTime() {return(m_rt);}
};

//

#define SaveMediaState \
	OAFilterState __fs = GetMediaState(); \
 \
	REFERENCE_TIME __rt = 0; \
	if(m_iMediaLoadState == MLS_LOADED) __rt = GetPos(); \
 \
	if(__fs != State_Stopped) \
		SendMessage(WM_COMMAND, ID_PLAY_STOP); \


#define RestoreMediaState \
	if(m_iMediaLoadState == MLS_LOADED) \
	{ \
		SeekTo(__rt); \
 \
		if(__fs == State_Stopped) \
			SendMessage(WM_COMMAND, ID_PLAY_STOP); \
		else if(__fs == State_Paused) \
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE); \
		else if(__fs == State_Running) \
			SendMessage(WM_COMMAND, ID_PLAY_PLAY); \
	} \

/*
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
*/
/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
	
	ON_REGISTERED_MESSAGE(s_uTaskbarRestart, OnTaskBarRestart)
	ON_REGISTERED_MESSAGE(WM_NOTIFYICON, OnNotifyIcon)

	ON_WM_SETFOCUS()
	ON_WM_GETMINMAXINFO()
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_WM_SIZE()
	ON_WM_SIZING()
	/*NEW UI*/
	ON_MESSAGE(WM_NCPAINT, OnNcPaint)
	ON_MESSAGE(WM_NCACTIVATE, OnNcActivate)
	ON_MESSAGE(WM_NCLBUTTONDOWN, OnNcLButtonDown)
	ON_MESSAGE(WM_NCLBUTTONUP, OnNcLButtonUp)
	ON_MESSAGE(WM_NCHITTEST, OnNcHitTestNewUI)
	ON_WM_NCCALCSIZE()
	/*NEW UI END*/

	ON_MESSAGE_VOID(WM_DISPLAYCHANGE, OnDisplayChange)

	ON_WM_SYSCOMMAND()
	ON_WM_ACTIVATEAPP()
	ON_MESSAGE(WM_APPCOMMAND, OnAppCommand)

	ON_WM_TIMER()

	ON_MESSAGE(WM_GRAPHNOTIFY, OnGraphNotify)
	ON_MESSAGE(WM_REARRANGERENDERLESS, OnRepaintRenderLess)
	ON_MESSAGE(WM_RESUMEFROMSTATE, OnResumeFromState)

	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_MESSAGE(WM_XBUTTONDOWN, OnXButtonDown)
	ON_MESSAGE(WM_XBUTTONUP, OnXButtonUp)
	ON_MESSAGE(WM_XBUTTONDBLCLK, OnXButtonDblClk)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEMOVE()
	//ON_WM_NCHITTEST()

	ON_WM_HSCROLL()

	ON_WM_INITMENU()
	ON_WM_INITMENUPOPUP()

	ON_COMMAND(ID_MENU_PLAYER_SHORT, OnMenuPlayerShort)
	ON_COMMAND(ID_MENU_PLAYER_LONG, OnMenuPlayerLong)
	ON_COMMAND(ID_MENU_FILTERS, OnMenuFilters)
	ON_UPDATE_COMMAND_UI(IDC_PLAYERSTATUS, OnUpdatePlayerStatus)

	ON_COMMAND(ID_FILE_POST_OPENMEDIA, OnFilePostOpenmedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_POST_OPENMEDIA, OnUpdateFilePostOpenmedia)
	ON_COMMAND(ID_FILE_POST_CLOSEMEDIA, OnFilePostClosemedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_POST_CLOSEMEDIA, OnUpdateFilePostClosemedia)

	ON_COMMAND(ID_BOSS, OnBossKey)

	ON_COMMAND_RANGE(ID_STREAM_AUDIO_NEXT, ID_STREAM_AUDIO_PREV, OnStreamAudio)
	ON_COMMAND_RANGE(ID_STREAM_SUB_NEXT, ID_STREAM_SUB_PREV, OnStreamSub)
	ON_COMMAND(ID_STREAM_SUB_ONOFF, OnStreamSubOnOff)
	ON_COMMAND_RANGE(ID_OGM_AUDIO_NEXT, ID_OGM_AUDIO_PREV, OnOgmAudio)
	ON_COMMAND_RANGE(ID_OGM_SUB_NEXT, ID_OGM_SUB_PREV, OnOgmSub)
	ON_COMMAND_RANGE(ID_DVD_ANGLE_NEXT, ID_DVD_ANGLE_PREV, OnDvdAngle)
	ON_COMMAND_RANGE(ID_DVD_AUDIO_NEXT, ID_DVD_AUDIO_PREV, OnDvdAudio)
	ON_COMMAND_RANGE(ID_DVD_SUB_NEXT, ID_DVD_SUB_PREV, OnDvdSub)
	ON_COMMAND(ID_DVD_SUB_ONOFF, OnDvdSubOnOff)

	
	ON_COMMAND(ID_FILE_OPENQUICK, OnFileOpenQuick)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENMEDIA, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENMEDIA, OnFileOpenmedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENMEDIA, OnUpdateFileOpen)
	
	ON_COMMAND(ID_FILE_OPENFOLDER, OnFileOpenFolder)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENFOLDER, OnUpdateFileOpen)
	
	ON_COMMAND(ID_FILE_OPENURLSTREAM, OnFileOpenUrlStream)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENURLSTREAM, OnUpdateFileOpen)
	ON_WM_COPYDATA()
	ON_COMMAND(ID_FILE_OPENDVD, OnFileOpendvd)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDVD, OnUpdateFileOpen)
	ON_COMMAND(ID_FILE_OPENDEVICE, OnFileOpendevice)
	ON_UPDATE_COMMAND_UI(ID_FILE_OPENDEVICE, OnUpdateFileOpen)
	ON_COMMAND_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnFileOpenCD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILE_OPEN_CD_START, ID_FILE_OPEN_CD_END, OnUpdateFileOpen)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_FILE_SAVE_COPY, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_COPY, OnUpdateFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE_IMAGE, OnFileSaveImage)
	ON_COMMAND(ID_FILE_COPYTOCLIPBOARD, OnFileCopyImageToCLipBoard)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_IMAGE, OnUpdateFileSaveImage)
	ON_UPDATE_COMMAND_UI(ID_FILE_COPYTOCLIPBOARD, OnUpdateFileSaveImage)
	ON_COMMAND(ID_FILE_SAVE_IMAGE_AUTO, OnFileSaveImageAuto)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_IMAGE_AUTO, OnUpdateFileSaveImage)
	ON_COMMAND(ID_FILE_SAVE_THUMBNAILS, OnFileSaveThumbnails)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_THUMBNAILS, OnUpdateFileSaveThumbnails)
	ON_COMMAND(ID_FILE_CONVERT, OnFileConvert)
	ON_UPDATE_COMMAND_UI(ID_FILE_CONVERT, OnUpdateFileConvert)
	ON_COMMAND(ID_FILE_LOAD_SUBTITLE, OnFileLoadsubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_SUBTITLE, OnUpdateFileLoadsubtitle)
	ON_COMMAND(ID_FILE_LOAD_SUBTITLE2, OnFileLoadsubtitle2)
	ON_UPDATE_COMMAND_UI(ID_FILE_LOAD_SUBTITLE2, OnUpdateFileLoadsubtitle)
	ON_COMMAND(ID_FILE_SAVE_SUBTITLE, OnFileSavesubtitle)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_SUBTITLE, OnUpdateFileSavesubtitle)
	ON_COMMAND(ID_FILE_ISDB_SEARCH, OnFileISDBSearch)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_SEARCH, OnUpdateFileISDBSearch)
	ON_COMMAND(ID_FILE_ISDB_UPLOAD, OnFileISDBUpload)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_UPLOAD, OnUpdateFileISDBUpload)
	ON_COMMAND(ID_FILE_ISDB_DOWNLOAD, OnFileISDBDownload)
	ON_UPDATE_COMMAND_UI(ID_FILE_ISDB_DOWNLOAD, OnUpdateFileISDBDownload)
	ON_COMMAND(ID_FILE_PROPERTIES, OnFileProperties)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateFileProperties)
	ON_COMMAND(ID_FILE_CLOSEPLAYLIST, OnFileClosePlaylist)
	ON_COMMAND(ID_FILE_CLOSEMEDIA, OnFileCloseMedia)
	ON_UPDATE_COMMAND_UI(ID_FILE_CLOSEMEDIA, OnUpdateFileClose)

	ON_COMMAND(ID_VIEW_CAPTIONMENU, OnViewCaptionmenu)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTIONMENU, OnUpdateViewCaptionmenu)
	ON_COMMAND_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnViewControlBar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_SEEKER, ID_VIEW_STATUS, OnUpdateViewControlBar)
	ON_COMMAND(ID_VIEW_SUBRESYNC, OnViewSubresync)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SUBRESYNC, OnUpdateViewSubresync)
	ON_COMMAND(ID_VIEW_PLAYLIST, OnViewPlaylist)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PLAYLIST, OnUpdateViewPlaylist)
	ON_COMMAND(ID_VIEW_CAPTURE, OnViewCapture)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTURE, OnUpdateViewCapture)
	ON_COMMAND(ID_VIEW_SHADEREDITOR, OnViewShaderEditor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHADEREDITOR, OnUpdateViewShaderEditor)
	ON_COMMAND(ID_VIEW_PRESETS_MINIMAL, OnViewMinimal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_MINIMAL, OnUpdateViewMinimal)
	ON_COMMAND(ID_VIEW_PRESETS_COMPACT, OnViewCompact)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_COMPACT, OnUpdateViewCompact)
	ON_COMMAND(ID_VIEW_PRESETS_NORMAL, OnViewNormal)
	ON_UPDATE_COMMAND_UI(ID_VIEW_PRESETS_NORMAL, OnUpdateViewNormal)
	ON_COMMAND(ID_VIEW_FULLSCREEN, OnViewFullscreen)
	ON_COMMAND(ID_VIEW_FULLSCREEN_SECONDARY, OnViewFullscreenSecondary)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FULLSCREEN, OnUpdateViewFullscreen)
	ON_COMMAND_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnViewZoom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_ZOOM_50, ID_VIEW_ZOOM_200, OnUpdateViewZoom)
	ON_COMMAND(ID_VIEW_ZOOM_AUTOFIT, OnViewZoomAutoFit)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ZOOM_AUTOFIT, OnUpdateViewZoom)
	ON_COMMAND_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_FROMOUTSIDE, OnViewDefaultVideoFrame)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_VF_HALF, ID_VIEW_VF_FROMOUTSIDE, OnUpdateViewDefaultVideoFrame)
	ON_COMMAND(ID_VIEW_VF_KEEPASPECTRATIO, OnViewKeepaspectratio)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_KEEPASPECTRATIO, OnUpdateViewKeepaspectratio)
	ON_COMMAND(ID_VIEW_VF_COMPMONDESKARDIFF, OnViewCompMonDeskARDiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_VF_COMPMONDESKARDIFF, OnUpdateViewCompMonDeskARDiff)
	ON_COMMAND_RANGE(ID_VIEW_RESET, ID_PANSCAN_CENTER, OnViewPanNScan)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_RESET, ID_PANSCAN_CENTER, OnUpdateViewPanNScan)
	ON_COMMAND_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnViewPanNScanPresets)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANNSCAN_PRESETS_START, ID_PANNSCAN_PRESETS_END, OnUpdateViewPanNScanPresets)
	ON_COMMAND_RANGE(ID_PANSCAN_ROTATEXP, ID_PANSCAN_ROTATEZM, OnViewRotate)
	ON_COMMAND_RANGE(ID_ROTATE_H, ID_ROTATE_END, OnViewRotate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PANSCAN_ROTATEXP, ID_PANSCAN_ROTATEZM, OnUpdateViewRotate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ROTATE_H, ID_ROTATE_END, OnUpdateViewRotate)
	ON_COMMAND_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnViewAspectRatio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ASPECTRATIO_START, ID_ASPECTRATIO_END, OnUpdateViewAspectRatio)
	ON_COMMAND(ID_ASPECTRATIO_NEXT, OnViewAspectRatioNext)
	ON_COMMAND_RANGE(ID_ONTOP_NEVER, ID_ONTOP_WHILEPLAYING, OnViewOntop)
	ON_UPDATE_COMMAND_UI_RANGE(ID_ONTOP_NEVER, ID_ONTOP_WHILEPLAYING, OnUpdateViewOntop)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	
	ON_COMMAND_RANGE(ID_SUB_DELAY_DOWN, ID_SUB_DELAY_UP, OnSubtitleDelay)
	ON_COMMAND_RANGE(ID_SUB_DELAY_DOWN2, ID_SUB_DELAY_UP2, OnSubtitleDelay2)

	ON_COMMAND_RANGE(ID_SUBMOVEUP, ID_SUB2MOVERIGHT, OnSubtitleMove)
	ON_COMMAND_RANGE(ID_SUBFONTUPBOTH, ID_SUB2FONTDOWN, OnSubtitleFontChange)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBFONTUPBOTH, ID_SUB2FONTDOWN, OnUpdateSubtitleFontChange)

	ON_COMMAND(ID_PLAY_PLAY, OnPlayPlay)
	ON_COMMAND(ID_PLAY_PAUSE, OnPlayPause)
	ON_COMMAND(ID_PLAY_PLAYPAUSE, OnPlayPlaypause)
	ON_COMMAND(ID_PLAY_STOP, OnPlayStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAY, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_PLAYPAUSE, OnUpdatePlayPauseStop)
	ON_UPDATE_COMMAND_UI(ID_PLAY_STOP, OnUpdatePlayPauseStop)
	//Sub Delay Button
	ON_COMMAND_RANGE(ID_SUBDELAYDEC,ID_SUBDELAYINC, OnPlaySubDelay )
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBDELAYDEC, ID_SUBDELAYINC, OnUpdatePlaySubDelay )
	ON_COMMAND_RANGE(ID_SUB2DELAYDEC,ID_SUB2DELAYINC, OnPlaySub2Delay )
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUB2DELAYDEC, ID_SUB2DELAYINC, OnUpdatePlaySub2Delay )

	ON_COMMAND_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnPlayFramestep)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_FRAMESTEP, ID_PLAY_FRAMESTEPCANCEL, OnUpdatePlayFramestep)
	ON_COMMAND_RANGE(ID_PLAY_BWD, ID_PLAY_FWD, OnSmartSeek)
	ON_COMMAND_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE4, OnPlaySeek)
	ON_COMMAND_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnPlaySeekKey)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKBACKWARDSMALL, ID_PLAY_SEEKFORWARDLARGE4, OnUpdatePlaySeek)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_SEEKKEYBACKWARD, ID_PLAY_SEEKKEYFORWARD, OnUpdatePlaySeek)
	ON_COMMAND(ID_PLAY_GOTO, OnPlayGoto)
	ON_UPDATE_COMMAND_UI(ID_PLAY_GOTO, OnUpdateGoto)
	ON_COMMAND_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnPlayChangeRate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_DECRATE, ID_PLAY_INCRATE, OnUpdatePlayChangeRate)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_BWD, ID_PLAY_FWD, OnUpdatePlayChangeRate)
	ON_COMMAND(ID_PLAY_RESETRATE, OnPlayResetRate)	
	ON_UPDATE_COMMAND_UI(ID_PLAY_RESETRATE, OnUpdatePlayResetRate)	
	ON_COMMAND_RANGE(ID_PLAY_INCAUDDELAY, ID_PLAY_DECAUDDELAY, OnPlayChangeAudDelay)
	ON_UPDATE_COMMAND_UI_RANGE(ID_PLAY_INCAUDDELAY, ID_PLAY_DECAUDDELAY, OnUpdatePlayChangeAudDelay)
	ON_COMMAND_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnPlayFilters)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERS_SUBITEM_START, ID_FILTERS_SUBITEM_END, OnUpdatePlayFilters)
	ON_COMMAND_RANGE(ID_SHADERS_START, ID_SHADERS_END, OnPlayShaders)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SHADERS_START, ID_SHADERS_END, OnUpdatePlayShaders)
	ON_COMMAND_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnPlayAudio)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AUDIO_SUBITEM_START, ID_AUDIO_SUBITEM_END, OnUpdatePlayAudio)
	ON_COMMAND(ID_TOGGLE_SUBTITLE, OnToogleSubtitle )
	ON_COMMAND_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnPlaySubtitles)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBTITLES_SUBITEM_START, ID_SUBTITLES_SUBITEM_END, OnUpdatePlaySubtitles)
	ON_COMMAND_RANGE(ID_SUBTITLES_SUBITEM_START2, ID_SUBTITLES_SUBITEM_END2, OnPlaySubtitles)
	ON_UPDATE_COMMAND_UI_RANGE(ID_SUBTITLES_SUBITEM_START2, ID_SUBTITLES_SUBITEM_END2, OnUpdatePlaySubtitles)
	ON_COMMAND_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnPlayLanguage)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FILTERSTREAMS_SUBITEM_START, ID_FILTERSTREAMS_SUBITEM_END, OnUpdatePlayLanguage)
	ON_COMMAND_RANGE(ID_VOLUME_UP, ID_VOLUME_MUTE, OnPlayVolume)
	ON_COMMAND_RANGE(ID_VOLUME_BOOST_INC, ID_VOLUME_BOOST_MAX, OnPlayVolumeBoost)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VOLUME_BOOST_INC, ID_VOLUME_BOOST_MAX, OnUpdatePlayVolumeBoost)
	ON_COMMAND_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_DONOTHING, OnAfterplayback)
	ON_UPDATE_COMMAND_UI_RANGE(ID_AFTERPLAYBACK_CLOSE, ID_AFTERPLAYBACK_DONOTHING, OnUpdateAfterplayback)

	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnNavigateSkip)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACK, ID_NAVIGATE_SKIPFORWARD, OnUpdateNavigateSkip)
	ON_COMMAND_RANGE(ID_NAVIGATE_SKIPBACKPLITEM, ID_NAVIGATE_SKIPFORWARDPLITEM, OnNavigateSkipPlaylistItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_SKIPBACKPLITEM, ID_NAVIGATE_SKIPFORWARDPLITEM, OnUpdateNavigateSkipPlaylistItem)
	ON_COMMAND_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnNavigateMenu)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_TITLEMENU, ID_NAVIGATE_CHAPTERMENU, OnUpdateNavigateMenu)
	ON_COMMAND_RANGE(ID_NAVIGATE_AUDIO_SUBITEM_START, ID_NAVIGATE_AUDIO_SUBITEM_END, OnNavigateAudio)
	ON_COMMAND_RANGE(ID_NAVIGATE_SUBP_SUBITEM_START, ID_NAVIGATE_SUBP_SUBITEM_END, OnNavigateSubpic)
	ON_COMMAND_RANGE(ID_NAVIGATE_ANGLE_SUBITEM_START, ID_NAVIGATE_ANGLE_SUBITEM_END, OnNavigateAngle)
	ON_COMMAND_RANGE(ID_NAVIGATE_CHAP_SUBITEM_START, ID_NAVIGATE_CHAP_SUBITEM_END, OnNavigateChapters)
	ON_COMMAND_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnNavigateMenuItem)
	ON_UPDATE_COMMAND_UI_RANGE(ID_NAVIGATE_MENU_LEFT, ID_NAVIGATE_MENU_LEAVE, OnUpdateNavigateMenuItem)

	ON_COMMAND(ID_FAVORITES_ADD, OnFavoritesAdd)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ADD, OnUpdateFavoritesAdd)
	ON_COMMAND(ID_FAVORITES_ORGANIZE, OnFavoritesOrganize)
	ON_UPDATE_COMMAND_UI(ID_FAVORITES_ORGANIZE, OnUpdateFavoritesOrganize)
	ON_COMMAND_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnFavoritesFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_FILE_START, ID_FAVORITES_FILE_END, OnUpdateFavoritesFile)
	ON_COMMAND_RANGE(ID_RECENT_FILE_START, ID_RECENT_FILE_END, OnRecentFile)
	ON_UPDATE_COMMAND_UI_RANGE(ID_RECENT_FILE_START, ID_RECENT_FILE_END, OnUpdateRecentFile)
	ON_COMMAND_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnFavoritesDVD)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DVD_START, ID_FAVORITES_DVD_END, OnUpdateFavoritesDVD)
	ON_COMMAND_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnFavoritesDevice)
	ON_UPDATE_COMMAND_UI_RANGE(ID_FAVORITES_DEVICE_START, ID_FAVORITES_DEVICE_END, OnUpdateFavoritesDevice)

	ON_COMMAND(ID_HELP_HOMEPAGE, OnHelpHomepage)
	ON_COMMAND(ID_HELP_DOCUMENTATION, OnHelpDocumentation)

	ON_COMMAND(ID_ADV_OPTIONS, &CMainFrame::OnAdvOptions)
	ON_COMMAND(ID_MANUALCHECKUPDATE, &CMainFrame::OnManualcheckupdate)
	ON_COMMAND(ID_SVPSUB_MENUENABLE, &CMainFrame::OnSvpsubMenuenable)
	ON_UPDATE_COMMAND_UI(ID_SVPSUB_MENUENABLE, &CMainFrame::OnUpdateSvpsubMenuenable)
	ON_COMMAND_RANGE(IDS_AUDIOCHANNALMAPNORMAL,IDS_AUDIOCHANNALMAPEND , &CMainFrame::OnAudioChannalMapMenu)
	ON_UPDATE_COMMAND_UI_RANGE(IDS_AUDIOCHANNALMAPNORMAL, IDS_AUDIOCHANNALMAPEND, &CMainFrame::OnUpdateChannalMapMenu)
	ON_COMMAND(ID_VISITBBS, &CMainFrame::OnVisitbbs)
	ON_COMMAND(ID_SENDEMAIL, &CMainFrame::OnSendemail)
	ON_COMMAND(ID_CHECK_DEFAULT_PLAYER, OnCheckDefaultPlayer)
	ON_COMMAND(ID_VISITCONTACTINFO, &CMainFrame::OnVisitcontactinfo)
	ON_COMMAND(ID_DONATE, &CMainFrame::OnDonate)
	ON_COMMAND(ID_JOINTEAM, &CMainFrame::OnJointeam)


	ON_COMMAND_RANGE(ID_BRIGHTINC, ID_BRIGHTDEC, OnColorControl )

	ON_COMMAND_RANGE(IDS_CHANGE_AUDIO_DEVICE, IDS_CHANGE_AUDIO_DEVICE_END, OnAudioDeviceChange )
	ON_UPDATE_COMMAND_UI_RANGE(IDS_CHANGE_AUDIO_DEVICE, IDS_CHANGE_AUDIO_DEVICE_END, OnUpdateAudioDeviceChange )

	ON_BN_CLICKED(IDC_BUTTONRESETCOLORCONTROL,  OnColorControlButtonReset)
	ON_BN_CLICKED(IDC_BUTTONENABLECOLORCONTROL, OnColorControlButtonEnable)
	ON_UPDATE_COMMAND_UI( IDC_BUTTONRESETCOLORCONTROL,  OnColorControlUpdateButtonReset)
	ON_UPDATE_COMMAND_UI( IDC_BUTTONENABLECOLORCONTROL,  OnColorControlUpdateButtonEnable)


	ON_COMMAND(ID_SHADERS_SETDX9, OnEnableDX9)

	ON_COMMAND(ID_SHOWCOLORCONTROLBAR, &CMainFrame::OnShowColorControlBar)
	ON_UPDATE_COMMAND_UI(ID_SHOWCOLORCONTROLBAR, &CMainFrame::OnUpdateShowColorControlBar)
	ON_COMMAND(ID_SETSNAPSHOTPATH, &CMainFrame::OnSetsnapshotpath)

	ON_MESSAGE(WM_HOTKEY,OnHotKey)
	ON_WM_ENTERMENULOOP()
	ON_WM_EXITMENULOOP()

	ON_REGISTERED_MESSAGE(WM_MOUSEMOVEIN, OnMouseMoveIn)
	ON_REGISTERED_MESSAGE(WM_MOUSEMOVEOUT, OnMouseMoveOut)

	ON_COMMAND(ID_RESET_SETTING,  OnResetSetting)
	ON_COMMAND(ID_VIEW_SETHOTKEY, OnSetHotkey)

	ON_WM_KILLFOCUS()
	ON_COMMAND(ID_CHANGEBACKGROUND, &CMainFrame::OnChangebackground)
	ON_COMMAND(ID_SUBSETFONTBOTH, &CMainFrame::OnSubsetfontboth)

	ON_COMMAND( ID_RECENTFILE_CLEAR ,  OnRecentFileClear)
	ON_COMMAND( ID_RECENTFILE_ENABLE ,  OnRecentFileEnable)
	ON_COMMAND( ID_RECENTFILE_DISABLE ,  OnRecentFileDisable)
	ON_WM_NCRBUTTONDOWN()
	ON_COMMAND(ID_DELETECURFILE, OnDeletecurfile)
	ON_COMMAND(ID_DELCURFOLDER, OnDelcurfolder)
	
	ON_UPDATE_COMMAND_UI_RANGE(ID_DELETECURFILE, ID_DELCURFOLDER, OnUpdateDeleteCurs)

	ON_COMMAND(ID_USINGSPDIF, OnToggleSPDIF)
	ON_UPDATE_COMMAND_UI( ID_USINGSPDIF,  OnUpdateToggleSPDIF)

	ON_COMMAND(ID_SUBTOOLBARBUTTON, OnSubMenuToolbar )
	

	ON_COMMAND(ID_DEBUGREPORT, OnDebugreport)
	ON_UPDATE_COMMAND_UI(ID_DEBUGREPORT, OnUpdateDebugreport)

	ON_COMMAND(ID_CONFIG_AUTOLOADSUBTITLE2, OnSetAutoLoadSubtitle)
	ON_UPDATE_COMMAND_UI(ID_CONFIG_AUTOLOADSUBTITLE2, OnUpdateSetAutoLoadSubtitle)

	END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

#pragma warning(disable : 4355)

static bool s_fLDown = false;
static int s_mDragFuc = 0;
static bool s_mDragFucOn = false;
//static bool bRecentFocused = FALSE;
static bool bNotHideColorControlBar = FALSE;
#define  SINGLECLICK_INTERLEAVE_MS 200
#define HTMINTOTRAY 10002

CMainFrame::CMainFrame() : 
	m_dwRegister(0),
	m_iMediaLoadState(MLS_CLOSED),
	m_iPlaybackMode(PM_NONE),
	m_iSpeedLevel(0),
	m_rtDurationOverride(-1),
	m_fFullScreen(false),
	m_fHideCursor(false),
	m_lastMouseMove(-1, -1),
	m_pLastBar(NULL),
	m_nLoops(0),
	m_iSubtitleSel(-1),
	m_ZoomX(1), m_ZoomY(1), m_PosX(0.5), m_PosY(0.5),
	m_AngleX(0), m_AngleY(0), m_AngleZ(0),
	m_fCustomGraph(false),
	m_fRealMediaGraph(false), m_fShockwaveGraph(false), m_fQuicktimeGraph(false),
	m_fFrameSteppingActive(false),
	m_fEndOfStream(false),
	m_fCapturing(false),
	m_fLiveWM(false),
	m_fOpeningAborted(false),
	m_fBuffering(false),
	m_fileDropTarget(this),
	m_fTrayIcon(false),
	m_iSubtitleSel2(-1),
	m_bSubUploading(false),
	m_bSubDownloading(false),
	m_iAudioChannelMaping(0),
	m_bCheckingUpdater(false),
	m_WndSizeInited(false),
	m_iOSDAlign(0),
	m_bDxvaInUse(false) ,
	m_bEVRInUse(false),
	m_bHasDrawShadowText(false)
{
}

CMainFrame::~CMainFrame()
{
//	m_owner.DestroyWindow();
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(__super::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_popup.LoadMenu(IDR_POPUP);
	m_popupmain.LoadMenu(IDR_POPUPMAIN);

	//GetMenu()->ModifyMenu(ID_FAVORITES, MF_BYCOMMAND|MF_STRING, IDR_MAINFRAME, ResStr(IDS_FAVORITES_POPUP));
	m_mainMenu.LoadMenu(IDR_MAINFRAME);

	// create a view to occupy the client area of the frame
	if(!m_wndView.Create(NULL, NULL, AFX_WS_DEFAULT_VIEW,
		CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, NULL))
	{
		TRACE0("Failed to create view window\n");
		return -1;
	}

	// static bars

	if(!m_wndStatsBar.Create(this)
	|| !m_wndInfoBar.Create(this)
	|| !m_wndToolBar.Create(this)
	|| !m_wndSeekBar.Create(this)
	)
	{
		TRACE0("Failed to create all control bars\n");
		return -1;      // fail to create
	}


	m_bars.AddTail(&m_wndSeekBar);
	m_bars.AddTail(&m_wndToolBar);
	m_bars.AddTail(&m_wndInfoBar);
	m_bars.AddTail(&m_wndStatsBar);
	
	m_wndSeekBar.Enable(false);

	// dockable bars

	EnableDocking(CBRS_ALIGN_ANY);

	m_dockingbars.RemoveAll();

	m_wndSubresyncBar.Create(this, &m_csSubLock);
	m_wndSubresyncBar.SetBarStyle(m_wndSubresyncBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndSubresyncBar.EnableDocking(CBRS_ALIGN_ANY);
//	m_wndSubresyncBar.SetHeight(200);
	LoadControlBar(&m_wndSubresyncBar, AFX_IDW_DOCKBAR_TOP);

	m_wndPlaylistBar.Create(this);
	m_wndPlaylistBar.SetBarStyle(m_wndPlaylistBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC | CBRS_BORDER_3D);
	m_wndPlaylistBar.EnableDocking(CBRS_ALIGN_ANY);
	//m_wndPlaylistBar.SetHeight(100);
	LoadControlBar(&m_wndPlaylistBar, AFX_IDW_DOCKBAR_RIGHT);
	m_wndPlaylistBar.LoadPlaylist();

	m_wndCaptureBar.Create(this);
	m_wndCaptureBar.SetBarStyle(m_wndCaptureBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndCaptureBar.EnableDocking(CBRS_ALIGN_LEFT|CBRS_ALIGN_RIGHT);
	LoadControlBar(&m_wndCaptureBar, AFX_IDW_DOCKBAR_LEFT);

	m_wndShaderEditorBar.Create(this);
	m_wndShaderEditorBar.SetBarStyle(m_wndShaderEditorBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);
	m_wndShaderEditorBar.EnableDocking(CBRS_ALIGN_ANY);
	LoadControlBar(&m_wndShaderEditorBar, AFX_IDW_DOCKBAR_TOP);


	
	if(m_wndColorControlBar.Create(this)){
		m_wndColorControlBar.ShowWindow( SW_HIDE);
	}
	if(m_wndStatusBar.Create(this)){
		//m_wndStatusBar.EnableDocking(0);
		//FloatControlBar(&m_wndStatusBar, CPoint(10,10), CBRS_ALIGN_BOTTOM );

		m_bars.AddTail(&m_wndStatusBar);
	}
	m_bars.AddTail(&m_wndColorControlBar);

	
	m_fileDropTarget.Register(this);

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);

	AppSettings& s = AfxGetAppSettings();

	if ( time(NULL) > (s.tLastCheckUpdater + s.tCheckUpdaterInterleave)){
		s.tLastCheckUpdater = (UINT)time(NULL); 
		s.UpdateData(true);
		SetTimer(TIMER_START_CHECKUPDATER, 30000, NULL);
	}

	
	ShowControls( ( s.nCS | (s.bShowControlBar ? CS_COLORCONTROLBAR : 0) ) & ~CS_SEEKBAR , false );
	
	GetSystemFontWithScale(&m_hft, 14.0, 600);
/*
	
		HMODULE hCommCtrlDLL =  ::LoadLibrary(L"comctl32.dll");
		if (hCommCtrlDLL)
		{
			if( ::GetProcAddress(hCommCtrlDLL, "DrawShadowText") ){
				m_bHasDrawShadowText = true;
			}
		}*/
	

	SetAlwaysOnTop(s.iOnTop);

	ShowTrayIcon(s.fTrayIcon);

	SetFocus();


	//WS_EX_NOACTIVATE
	if(!m_wndNewOSD.CreateEx(WS_EX_NOACTIVATE|WS_EX_TOPMOST, _T("STATIC"), _T("OSD"), WS_POPUP, CRect( 20,20,60,35 ) , this,  0)){
		AfxMessageBox(_T("OSD 创建失败！"));
	}

	m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));

	if(m_pGraphThread)
		m_pGraphThread->SetMainFrame(this);

	if(s.fEnableWebServer)
		StartWebServer(s.nWebServerPort);
		
	// load shaders
	CString	strList = AfxGetAppSettings().strShaderList;
	CString	strRes;
	int	curPos= 0;
		
	strRes = strList.Tokenize (_T("|"), curPos);
	while (strRes != _T(""))
	{
		m_shaderlabels.AddTail (strRes);
		strRes = strList.Tokenize(_T("|"),curPos);
	};
	// end

	CString szBuild;
	if(!s.szOEMTitle.IsEmpty()){
		szBuild.Format(_T(" (%s Build %s)"), s.szOEMTitle , SVP_REV_STR);
	}else{
		szBuild.Format( _T(" (Build %s)"),SVP_REV_STR);
	}
	//SetWindowText(CString(ResStr(IDR_MAINFRAME)) + szBuild);
	m_szTitle = CString(ResStr(IDR_MAINFRAME)) + szBuild;
	

	SetTimer(TIMER_STATUSCHECKER , 10000, NULL);

	s.RegGlobalAccelKey(this->m_hWnd);
	/*NEW UI*/
	SetMenu(  NULL);

	//ZeroMemory(m_nBoxStatus, sizeof(m_nBoxStatus));
	m_bmpCaption.LoadBitmap(L"CAPTION.BMP");

	m_bmpBCorner.LoadBitmap(L"BORDER_CORNER.BMP");
	
	//////////////////////////////////////////////////////////////////////////
	// an alternative way of pre-multiplying bitmap data
	CRect btnMargin(3,7,15,3);
	CSize btnSize(21,17);//IDM_CLOSE_PNG
	CSUIButton* bClose = new CSUIButton( L"CLOSE.BMP",ALIGN_TOPRIGHT, btnMargin  , 0,  HTCLOSE);
	m_btnList.AddTail(bClose );
	btnMargin.right = 3;

	CSUIButton* btnMax = new CSUIButton(L"MAXIMIZE.BMP" , ALIGN_TOPRIGHT, btnMargin  , 0, HTMAXBUTTON, FALSE, ALIGN_RIGHT, bClose );
	CSUIButton* btnRestore = new CSUIButton(L"RESTORE.BMP" , ALIGN_TOPRIGHT, btnMargin  , 0, HTMAXBUTTON, TRUE, ALIGN_RIGHT, bClose );
	m_btnList.AddTail(btnMax );
	m_btnList.AddTail( btnRestore );

	CSUIButton* btnMin = new CSUIButton(L"MINIMIZE.BMP", ALIGN_TOPRIGHT, btnMargin  ,  0, HTMINBUTTON ,FALSE, ALIGN_RIGHT, btnRestore);
	btnMin->addAlignRelButton(ALIGN_RIGHT , btnMax );
	m_btnList.AddTail( btnMin);
	m_btnList.AddTail( new CSUIButton(L"BTN_MINTOTRAY.BMP", ALIGN_TOPRIGHT, btnMargin  ,  0, HTMINTOTRAY ,FALSE, ALIGN_RIGHT, m_btnList.GetTail()));
	CSUIButton* btnMenu = new CSUIButton(L"MENU.BMP", ALIGN_TOPRIGHT, btnMargin  ,  0, HTMENU, FALSE, ALIGN_RIGHT, m_btnList.GetTail(), CRect(3,3,12,3));
	btnMenu->addAlignRelButton( ALIGN_RIGHT , btnMin , CRect(3,3,12,3) );
	m_btnList.AddTail( btnMenu);


	/*
	m_bmpClose.Attach((HBITMAP)::LoadImage(GetModuleHandle(NULL) L"CLOSE.BMP", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION));
	m_bmpMaximize.Attach( (HBITMAP)::LoadImage(GetModuleHandle(NULL), L"MAXIMIZE.BMP", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION) );
	m_bmpMinimize.Attach((HBITMAP)::LoadImage(GetModuleHandle(NULL), L"MINIMIZE.BMP", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION));
	m_bmpRestore.Attach((HBITMAP)::LoadImage(GetModuleHandle(NULL), L"RESTORE.BMP", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION));
	m_bmpMenu.Attach((HBITMAP)::LoadImage(GetModuleHandle(NULL), L"MENU.BMP", IMAGE_BITMAP, 0, 0, LR_DEFAULTCOLOR|LR_CREATEDIBSECTION));
	PreMultiplyBitmap(m_bmpClose);
	PreMultiplyBitmap(m_bmpMaximize);
	PreMultiplyBitmap(m_bmpMinimize);
	PreMultiplyBitmap(m_bmpRestore);
	PreMultiplyBitmap(m_bmpMenu);*/
	m_btnList.SetHideStat( HTMINTOTRAY,   s.fTrayIcon);


	/*NEW UI END*/
	return 0;
}

/*NEW UI*/
static bool bNoMoreHideMouse = false;

void CMainFrame::OnMouseMove(UINT nFlags, CPoint point)
{
	if( m_iMediaLoadState == MLS_CLOSED )
		m_wndView.OnMouseMove(nFlags,point);


	CRect CVideoRect = m_wndView.GetVideoRect();
	if(m_iPlaybackMode == PM_DVD)
	{
		CPoint vp = point - CVideoRect.TopLeft();
		pDVDC->SelectAtPosition(vp);
	}

	CSize diff = m_lastMouseMove - point;
	BOOL bMouseMoved =  diff.cx || diff.cy ;
	if(bMouseMoved){
		m_fHideCursor = false;
		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		if(!bNoMoreHideMouse && m_iMediaLoadState == MLS_LOADED)
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
	}
	AppSettings& s = AfxGetAppSettings();
	int iDistance = sqrt( pow( (double)abs(point.x - m_pLastClickPoint.x) , 2)  + pow( (double)abs( point.y - m_pLastClickPoint.y ) , 2) );
	if( ( iDistance > 30 || s_mDragFucOn) && s_mDragFuc){
		if(!s_mDragFucOn){
			m_pDragFuncStartPoint = point;
			SetAlwaysOnTop(s.iOnTop , FALSE);
			s_mDragFucOn = true;
		}
		if(s_mDragFuc == 1){ //移动画面
			m_PosX += (double)(point.x - m_pDragFuncStartPoint.x) / CVideoRect.Width() ;
			m_PosY += (double)(point.y - m_pDragFuncStartPoint.y)/ CVideoRect.Height() ;
			MoveVideoWindow(true);
		}else if(s_mDragFuc == 2){//缩放画面
			m_ZoomX += (double)(point.x - m_pDragFuncStartPoint.x) / CVideoRect.Width() ;
			m_ZoomY += (double)(m_pDragFuncStartPoint.y - point.y ) / CVideoRect.Height() ;
			MoveVideoWindow(true);
		}else if(s_mDragFuc == 3){
			PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
		m_pDragFuncStartPoint = point;

		s_fLDown = false;

	}
	if( ( m_fFullScreen || s.fHideCaptionMenu) && bMouseMoved)
	{
		int nTimeOut = s.nShowBarsWhenFullScreenTimeOut; //Should Be 0

		if(nTimeOut < 0)
		{
			m_fHideCursor = false;
			if(s.fShowBarsWhenFullScreen){
				ShowControls(s.nCS | (s.bShowControlBar ? CS_COLORCONTROLBAR : 0), false);
				//m_wndColorControlBar.ShowWindow(SW_SHOW);
			}

			KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
			//			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
		}
		else if(nTimeOut == 0)
		{
			CRect r;
			GetClientRect(r);
			r.top = r.bottom;
			DWORD dnCS = s.nCS;
			if( s.fHideCaptionMenu){
				dnCS |= CS_TOOLBAR|CS_SEEKBAR;
			}
			POSITION pos = m_bars.GetHeadPosition();
			for(int i = 1; pos; i <<= 1)
			{
				CControlBar* pNext = m_bars.GetNext(pos);
				CSize sz = pNext->CalcFixedLayout(FALSE, TRUE);
				if(dnCS&i) r.top -= sz.cy;
			}
			
			// HACK: the controls would cover the menu too early hiding some buttons
			if(m_iPlaybackMode == PM_DVD
				&& (m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu
				|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu))
				r.top = r.bottom - 10;

			m_fHideCursor = false;

			if(r.PtInRect(point))
			{
				if(s.fShowBarsWhenFullScreen){
					ShowControls(dnCS | ( s.bShowControlBar ? CS_COLORCONTROLBAR : 0) , false);
					//m_wndColorControlBar.ShowWindow(SW_SHOW);
				}
			}
			else{
				if(s.fShowBarsWhenFullScreen)
					ShowControls(CS_NONE, false);

			}


			//SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL);
		}
		else
		{
			m_fHideCursor = false;
			if(s.fShowBarsWhenFullScreen){
				ShowControls(s.nCS |  ( s.bShowControlBar ? CS_COLORCONTROLBAR : 0), false );
				//m_wndColorControlBar.ShowWindow(SW_SHOW);
			}

			SetTimer(TIMER_FULLSCREENCONTROLBARHIDER, nTimeOut*1000, NULL);
			//SetTimer(TIMER_FULLSCREENMOUSEHIDER, max(nTimeOut*1000, 2000), NULL);
		}
	}

	
	


	m_lastMouseMove = point;
	
	__super::OnMouseMove(nFlags, point);

	

}


void CMainFrame::OnNcCalcSize( BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp){
	WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
	GetWindowPlacement(&wp);
	if(bCalcValidRects){
		//先把rect[1]拷贝到rect[2]，rect[0]拷贝到rect[1]
		//memcpy( &lpncsp->rgrc[2] ,  &lpncsp->rgrc[1] , sizeof(RECT));
		//memcpy( &lpncsp->rgrc[1] ,  &lpncsp->rgrc[0] , sizeof(RECT));

		CRect rc = lpncsp->rgrc[0];
		DWORD currentStyle = GetStyle();
		BOOL bCaptionOn = currentStyle&WS_CAPTION ;
		if(m_fFullScreen){

		}else if(wp.showCmd!=SW_MAXIMIZE ){
			rc.InflateRect( GetSystemMetrics(SM_CXFRAME) - 4,  GetSystemMetrics(SM_CXFRAME) - 8,   GetSystemMetrics(SM_CXFRAME) - 4, GetSystemMetrics(SM_CXFRAME) - 3  );
			if(!m_wndToolBar.IsVisible())	{
				//rc.bottom -= 2;
			}
			rc.bottom -= 4;
			rc.top -=1 ;
// 			CString szLog;
// 			szLog.Format(_T("%d %d"), GetSystemMetrics(SM_CXFRAME), GetSystemMetrics(SM_CYCAPTION));
// 			AfxMessageBox(szLog);
			if(!bCaptionOn){
				//rc.top -=3 ;
				rc.left += 1;
				rc.right -= 1;
			}
			if(m_wndToolBar.IsVisible()){
				//rc.bottom += 2;
				if(bCaptionOn){
					rc.bottom += 1;
				}
			}
		}else{
			//rc.InflateRect( GetSystemMetrics(SM_CXFRAME) - 3, 0,   GetSystemMetrics(SM_CXFRAME) - 3, GetSystemMetrics(SM_CXFRAME) - 2);

		}
		
		
		lpncsp->rgrc[0] = rc;
	}
	__super::OnNcCalcSize(bCalcValidRects, lpncsp);

	if(wp.showCmd==SW_MAXIMIZE ){
		CString szLog;
		szLog.Format(_T("Max Client Rect %d %d %d %d") , lpncsp->rgrc[0].left, lpncsp->rgrc[0].top, lpncsp->rgrc[0].right, lpncsp->rgrc[0].bottom);
		//SVP_LogMsg(szLog);
	}
}


void CMainFrame::OnMove(int x, int y)
{
	__super::OnMove(x, y);

	MoveVideoWindow();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if(!m_fFullScreen && wp.flags != WPF_RESTORETOMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED && m_WndSizeInited)
		GetWindowRect(AfxGetAppSettings().rcLastWindowPos);

	CRect rc;
	GetWindowRect(&rc);
	m_btnList.OnSize( rc);
	
	m_wndToolBar.ReCalcBtnPos();
	m_wndView.ReCalcBtn();
	rePosOSD();
	//m_wndView.SetMyRgn();
	
	RedrawNonClientArea();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	
	__super::OnSize(nType, cx, cy);
	
	//SVP_LogMsg3("Winsizing @ %I64d", AfxGetMyApp()->GetPerfCounter());
	//CAutoLock PaintLock(&m_PaintLock);
	AppSettings& s = AfxGetAppSettings();

	if(nType == SIZE_RESTORED && m_fTrayIcon)
	{
		//ShowWindow(SW_SHOW);
	}else if(nType == SIZE_MINIMIZED && m_fTrayIcon ){
		ShowWindow(SW_HIDE);
	}

	{//New UI

		if(m_fFullScreen || SIZE_MAXIMIZED == nType)
		{
			m_btnList.SetHideStat(L"MAXIMIZE.BMP", TRUE);
			m_btnList.SetHideStat(L"RESTORE.BMP", false);
		}else{
			m_btnList.SetHideStat(L"MAXIMIZE.BMP", false);
			m_btnList.SetHideStat(L"RESTORE.BMP", true);
		}
		CRect rc;
		GetWindowRect(&rc);
		m_btnList.OnSize( rc);
	}

	CRect r,cr;
	m_wndView.GetClientRect(r);
	m_wndView.GetWindowRect(cr);
	r.top += r.Height()/2;
	cr.top += cr.Height()/2;
	
	rePosOSD();
	
	//::SetWindowPos(m_wndStatusBar.m_hWnd, m_wndView.m_hWnd, 20, 20, 100, 30,0);

	if(!m_fFullScreen)
	{
		if(nType != SIZE_MAXIMIZED && nType != SIZE_MINIMIZED && m_WndSizeInited)
			GetWindowRect(s.rcLastWindowPos);
		s.lastWindowType = nType;

		{ //New UI
			CRect rc;
			WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
			GetWindowPlacement(&wp);
			GetWindowRect(&rc);
			rc-=rc.TopLeft();



			// destroy old region
			if((HRGN)m_rgn)
			{
				m_rgn.DeleteObject();
			}
			// create rounded rect region based on new window size
			if (wp.showCmd != SW_MAXIMIZE )
			{
				rc.InflateRect(GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));
				m_rgn.CreateRoundRectRgn(0,0,rc.Width()-1,rc.Height()-1, 3,3);                 // rounded rect w/50 pixel corners

				// set window region to make rounded window
			}
			else{
				m_rgn.CreateRectRgn( 0,0, rc.Width(), rc.Height() );

			}
			SetWindowRgn(m_rgn,TRUE);
			Invalidate();
		}
	}else{
		SetWindowRgn(NULL,TRUE);   
	}


	
	//SVP_LogMsg3("Winsized @ %I64d", AfxGetMyApp()->GetPerfCounter());

}

LRESULT CMainFrame::OnNcPaint(  WPARAM wParam, LPARAM lParam )
{
	//////////////////////////////////////////////////////////////////////////
	// Typically, we'll need the window placement information
	// and its rect to perform painting
	//CAutoLock PaintLock(&m_PaintLock);
	LONGLONG startTime = AfxGetMyApp()->GetPerfCounter();
	WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
	GetWindowPlacement(&wp);
	CRect rc, rcWnd;
	GetWindowRect(&rcWnd);
	rc = rcWnd - rcWnd.TopLeft();
	if(wp.showCmd!=SW_MAXIMIZE && !m_fFullScreen){
		rc.InflateRect(GetSystemMetrics(SM_CXBORDER), GetSystemMetrics(SM_CYBORDER));
		if(!m_wndToolBar.IsVisible()){
			//rc.bottom -=3;
		}
	}
	DWORD currentStyle = GetStyle();
	BOOL bCAPTIONon = currentStyle&WS_CAPTION ;

	//////////////////////////////////////////////////////////////////////////
	// This is an undocumented (or not very clearly documented)
	// trick. When wParam==1, documents says that the entire
	// window needs to be repainted, so in order to paint, we get
	// the DC by calling GetWindowDC. Otherwise, the wParam
	// parameter should contain the region, we make sure anything
	// outside the region is excluded from update because it's
	// actually possible to paint outside the region

	CDC* dc;
	if(wParam == 1)
		dc = GetWindowDC();
	else
	{
		CRgn* rgn = CRgn::FromHandle((HRGN)wParam);
		dc = GetDCEx(rgn, DCX_WINDOW|DCX_INTERSECTRGN|0x10000);
	}

	if ((HDC)dc)
	{
		// when updating the window frame, we exclude the client area
		// because we don't want to interfere with the client WM_PAINT
		// process and cause extra cost
		RECT rcClient = {0};
		GetClientRect(&rcClient);
		rcClient.top+=3;
		if(bCAPTIONon){
			rcClient.top+=GetSystemMetrics(SM_CYCAPTION) + 4;
			
			//rcClient.bottom+=GetSystemMetrics(SM_CYCAPTION);
		}else{
			rcClient.top+=3;//GetSystemMetrics(SM_CYFRAME);
		}
		rcClient.left+=4 ;
		rcClient.right+=4;
		rcClient.bottom+=6 ;
		if(m_wndToolBar.IsVisible()){
			
			rcClient.bottom+= 20;;//m_wndToolBar.CalcFixedLayout(TRUE, TRUE).cy;
		}else{
			//rcClient.bottom-=4;
		}

		int nTotalCaptionHeight = GetSystemMetrics(SM_CYCAPTION)+GetSystemMetrics(SM_CYFRAME)+( (8 - GetSystemMetrics(SM_CYFRAME) ) /2 );

		if(m_fFullScreen){
			nTotalCaptionHeight -= 4;
		}
		dc->ExcludeClipRect(&rcClient);

		// establish double buffered painting
		CMemoryDC hdc(dc, rc);
// 		CBrush brush;
// 		brush.CreateSolidBrush(RGB(0xff,0x00,0x00));
// 		HBRUSH holdbrush = (HBRUSH)hdc.SelectObject(brush);
		if (wp.showCmd != SW_MAXIMIZE && !m_fFullScreen){
			//hdc.RoundRect(rc.left+1, rc.top+1, rc.right-1, rc.bottom-1, 3, 3);
			int bSpace = 1;
			CPen pen;
			pen.CreatePen(PS_SOLID, 1, RGB(0x7f,0x7f,0x7f));
			HPEN holdpen = (HPEN)hdc.SelectObject(pen);
			hdc.MoveTo(rc.left+bSpace, rc.bottom-bSpace);
			hdc.LineTo(rc.left+bSpace, rc.top+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.bottom-bSpace-1);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-1);

			bSpace++;
			CPen pen2;
			pen2.CreatePen(PS_SOLID, 1, RGB(0xf0,0xf0,0xf0));
			holdpen = (HPEN)hdc.SelectObject(pen2);
			hdc.MoveTo(rc.left+bSpace, rc.bottom-bSpace);
			hdc.LineTo(rc.left+bSpace, rc.top+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.bottom-bSpace-1);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-1);

			bSpace++;
			CPen pen3;
			pen3.CreatePen(PS_SOLID, 1, RGB(0xe0,0xe0,0xe0));
			holdpen = (HPEN)hdc.SelectObject(pen3);
			hdc.MoveTo(rc.left+bSpace, rc.bottom-bSpace);
			hdc.LineTo(rc.left+bSpace, rc.top+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+bSpace);
			
			hdc.MoveTo(rc.left+bSpace, rc.top+1+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+1+bSpace);
			hdc.MoveTo(rc.left+bSpace, rc.top+2+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+2+bSpace);
			
			hdc.LineTo(rc.right-bSpace-1, rc.bottom-bSpace-1);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-1);
			hdc.MoveTo(rc.right-bSpace-1, rc.bottom-bSpace-2);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-2);
			hdc.MoveTo(rc.right-bSpace-1, rc.bottom-bSpace-3);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-3);

			bSpace++;
			CPen pen4;
			pen4.CreatePen(PS_SOLID, 1, RGB(0x73,0x73,0x73));
			holdpen = (HPEN)hdc.SelectObject(pen4);
			hdc.MoveTo(rc.left+bSpace, rc.bottom-bSpace);
			hdc.LineTo(rc.left+bSpace, rc.top+2+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.top+2+bSpace);
			hdc.LineTo(rc.right-bSpace-1, rc.bottom-bSpace-1-2);
			hdc.LineTo(rc.left+bSpace, rc.bottom-bSpace-1-2);

			BOOL bToolBarOn = m_wndToolBar.IsVisible();
			
			CDC dcBmp;
			dcBmp.CreateCompatibleDC(&hdc);
			HBITMAP hbmpold = (HBITMAP)dcBmp.SelectObject(m_bmpBCorner);
			hdc.SetStretchBltMode(HALFTONE);
			hdc.SetBrushOrg(0, 0);
			int tx = rc.Width() - 2;
			int ty = rc.Height() - 2;
			hdc.StretchBlt(0,0,5,7, &dcBmp, 0,0,5,7, SRCCOPY); // TopLeft
			hdc.StretchBlt(tx-5,0,5,7, &dcBmp, 5,0,5,7, SRCCOPY); // Top Right
			hdc.StretchBlt(0,ty-7,5,7, &dcBmp, 0,7,5,7, SRCCOPY);
			hdc.StretchBlt(tx-5,ty-7,5,7, &dcBmp, 5,7,5,7, SRCCOPY);
			
			if(bToolBarOn && 0){
				hdc.StretchBlt(0,ty-13,5,9, &dcBmp, 0,9,5,1, SRCCOPY);
				hdc.StretchBlt(tx-5,ty-13,5,9, &dcBmp, 5,9,5,1, SRCCOPY);
				hdc.StretchBlt(0,ty-14,5,1, &dcBmp, 0,8,5,1, SRCCOPY);
				hdc.StretchBlt(tx-5,ty-14,5,1, &dcBmp, 5,8,5,1, SRCCOPY);
				hdc.StretchBlt(0,ty-15,5,1, &dcBmp, 0,7,5,1, SRCCOPY);
				hdc.StretchBlt(tx-5,ty-15,5,1, &dcBmp, 5,7,5,1, SRCCOPY);
			}
			dcBmp.SelectObject(hbmpold);
		}else if(currentStyle&WS_CAPTION) {
			//hdc.FillRect(&rc, &brush);
		}


// 		CString szLog;
// 		szLog.Format(_T("%d") , nTotalCaptionHeight);
// 		AfxMessageBox(szLog);
		
		// some basic styles
		if(!m_fFullScreen){
// 			CPen pen, penBright, penDark;
// 			pen.CreatePen(PS_SOLID, 1, NEWUI_COLOR_PEN );
// 			penBright.CreatePen(PS_SOLID, 1, NEWUI_COLOR_PEN_BRIGHT);
// 			penDark.CreatePen(PS_SOLID, 1,NEWUI_COLOR_PEN_BRIGHT);
// 			HPEN holdpen = (HPEN)hdc.SelectObject(pen);
//			HBRUSH holdbrush = (HBRUSH)hdc.SelectObject(brush);
// 			hdc.SelectObject(penBright);
// 			hdc.MoveTo(rc.left+2, rc.bottom-3);
// 			hdc.LineTo(rc.left+2, rc.top+2);
// 			hdc.LineTo(rc.right-3, rc.top+2);
// 			hdc.SelectObject(penDark);
// 			hdc.MoveTo(rc.left+2, rc.bottom-3);
// 			hdc.LineTo(rc.right-3, rc.bottom-3);
// 			hdc.LineTo(rc.right-3, rc.top+2);
// 			hdc.SelectObject(holdpen);
// 			hdc.SelectObject(holdbrush);

			// 		hdc.SelectObject((HBRUSH) GetStockObject(NULL_BRUSH));
			// 		if (wp.showCmd != SW_MAXIMIZE)
			// 			hdc.RoundRect(rc.left+1, rc.top+1, rc.right-1, rc.bottom-1, 3, 3);

			
		}
		if(bCAPTIONon){
			// and the title
			
			// painting the caption bar
			CDC dcBmp;
			dcBmp.CreateCompatibleDC(&hdc);
			HBITMAP hbmpold = (HBITMAP)dcBmp.SelectObject(m_bmpCaption);
			hdc.SetStretchBltMode(HALFTONE);
			if(GetForegroundWindow() != this){
				COLORADJUSTMENT cadj;
				hdc.GetColorAdjustment(&cadj);
				cadj.caRedGamma = 15000;
				cadj.caGreenGamma = 15000;
				cadj.caBlueGamma = 15000;
				cadj.caSize = sizeof(cadj);
				hdc.SetColorAdjustment(&cadj);
			}
			hdc.SetBrushOrg(0, 0);
			hdc.StretchBlt(0, 0, 4, nTotalCaptionHeight, &dcBmp, 0, 0, 4, 25, SRCCOPY);
			hdc.StretchBlt(4, 0, rc.Width()-4*2, nTotalCaptionHeight, &dcBmp, 4, 0, 6, 25, SRCCOPY);
			hdc.StretchBlt(rc.Width()-2-4, 0, 4, nTotalCaptionHeight, &dcBmp, 10, 0, 4, 25, SRCCOPY);
			dcBmp.SelectObject(hbmpold);

			HFONT holdft = (HFONT)hdc.SelectObject(m_hft);

			RECT rcWindowText = {0};
			rcWindowText.top = rc.top+GetSystemMetrics(SM_CYFRAME)/2;
			rcWindowText.left = rc.left+7+GetSystemMetrics(SM_CXFRAME);
			rcWindowText.bottom = rc.top+nTotalCaptionHeight;
			rcWindowText.right = rc.right;


			CRect btnMenuRect = m_btnList.GetHTRect(HTMENU);
			rcWindowText.right = rcWindowText.right - ( rcWnd.right - btnMenuRect.left + 10 );
			
			CString szWindowText ;
			if(m_bDxvaInUse){
				CString szEVR;
				if(m_bEVRInUse){
					szEVR = _T("+EVR");
				}
				szWindowText.Format( _T("[硬件高清%s] "), szEVR);
			}else if(m_bEVRInUse){
				szWindowText = _T("[EVR] ");
			}
			szWindowText.Append(m_szTitle);
			
			//GetWindowText(szWindowText);
			//if(m_bHasDrawShadowText )
			//	::DrawShadowText(hdc, szWindowText, szWindowText.GetLength(), &rcWindowText, DT_LEFT|DT_SINGLELINE | DT_VCENTER, 0x00525d66, RGB(255,255,255), 1,1);
			//else{
				hdc.SetTextColor(0x00525d66);
				hdc.SetBkMode(TRANSPARENT);
				//hdc.SetBkColor( 0x00d6d7ce);
				DrawText(hdc, szWindowText, szWindowText.GetLength(), &rcWindowText, DT_LEFT|DT_SINGLELINE | DT_VCENTER );
			//}
			hdc.SelectObject(holdft);

			// min/max/close buttons
			//LONG lPaintParamArray[][5] = {{1,0},{1,1},{1,2},{1,3}};
			GetWindowRect(&rc);
			if(wp.showCmd==SW_MAXIMIZE || m_fFullScreen){
				//rc.top -= 8;
			}
			m_btnList.PaintAll(&hdc, rc);
		}
		/*

		int nVertPos = NEWUI_BTN_MARGIN_TOP;
		long nImagePositions[] = {0, NEWUI_BTN_HEIGTH, NEWUI_BTN_HEIGTH*2, NEWUI_BTN_HEIGTH*3};  // 正常 ; hove ; 按下 ; disabled
		dcBmp.SelectObject(m_bmpClose);
		BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};//
		hdc.AlphaBlend(rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH, nVertPos, NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, &dcBmp, 0, nImagePositions[m_nBoxStatus[0]], NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, bf);
		dcBmp.SelectObject( (wp.showCmd==SW_MAXIMIZE)?m_bmpRestore:m_bmpMaximize);
		hdc.AlphaBlend(rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*2, nVertPos, NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, &dcBmp, 0, nImagePositions[m_nBoxStatus[1]], NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, bf);
		dcBmp.SelectObject(m_bmpMinimize);
		hdc.AlphaBlend(rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*3, nVertPos, NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, &dcBmp, 0, nImagePositions[m_nBoxStatus[2]], NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, bf);
		dcBmp.SelectObject(m_bmpMenu);
		hdc.AlphaBlend(rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*4, nVertPos, NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, &dcBmp, 0, nImagePositions[m_nBoxStatus[3]], NEWUI_BTN_WIDTH, NEWUI_BTN_HEIGTH, bf);
		*/



		ReleaseDC(&hdc);

	}
	//LONGLONG costTime = AfxGetMyApp()->GetPerfCounter() - startTime;
	//SVP_LogMsg3("NcPaint @ %I64d cost %I64d" , startTime , costTime);
	return FALSE ;
}

LRESULT CMainFrame::OnNcActivate( WPARAM wParam, LPARAM lParam)
{
	//SVP_LogMsg(_T("OnNcActivate"));
	RedrawWindow();
	return TRUE;
}

LRESULT CMainFrame::OnNcLButtonDown( WPARAM wParam, LPARAM lParam )
{
	// custom processing of our min/max/close buttons
	if (wParam == HTCLOSE || wParam == HTMAXBUTTON || wParam == HTMINBUTTON || HTMINTOTRAY == wParam)
	{
		RedrawNonClientArea();
		return FALSE;
	}
	if(wParam == HTMENU){
		OnMenu(m_popup.GetSubMenu(0));
		m_btnList.ClearStat();
		RedrawNonClientArea();
		return FALSE;
	}
	return DefWindowProc(WM_NCLBUTTONDOWN, wParam, lParam);
}

LRESULT CMainFrame::OnNcLButtonUp( WPARAM wParam, LPARAM lParam )
{
	// custom processing of our min/max/close buttons
	if (wParam == HTCLOSE || wParam == HTMAXBUTTON || wParam == HTMINBUTTON || wParam == HTMENU || HTMINTOTRAY == wParam)
	{
		RedrawNonClientArea();
		WINDOWPLACEMENT wp = {sizeof(WINDOWPLACEMENT)};
		GetWindowPlacement(&wp);
		if (wParam == HTCLOSE)
			PostMessage(WM_CLOSE);
		else if(wParam == HTMAXBUTTON)
		{
			//m_nBoxStatus[0] = m_nBoxStatus[1] =m_nBoxStatus[2] = 0;
			m_btnList.ClearStat();
			if (wp.showCmd == SW_MAXIMIZE)
				ShowWindow(SW_NORMAL);
			else
				ShowWindow(SW_MAXIMIZE);
		}
		else if (wParam == HTMINBUTTON)
		{
			//m_nBoxStatus[0] = m_nBoxStatus[1] =m_nBoxStatus[2] = 0;
			m_btnList.ClearStat();
			ShowWindow(SW_MINIMIZE);
		}
		else if (wParam == HTMENU  )
		{
			//OnMenu(m_mainMenu.GetSubMenu(0));
		}else if(HTMINTOTRAY == wParam ){
			m_btnList.ClearStat();
			ShowWindow(SW_HIDE);
			ShowTrayIcon(true);
		}
		return FALSE;
	}
	return DefWindowProc(WM_NCLBUTTONUP, wParam, lParam);
}

void CMainFrame::OnNcRButtonDown(UINT nHitTest, CPoint point)
{
	// TODO: Add your message handler code here and/or call default
	OnMenu(m_popup.GetSubMenu(0));
	m_btnList.ClearStat();
	RedrawNonClientArea();
	
	//__super::OnNcRButtonDown(nHitTest, point);
}

LRESULT CMainFrame::OnNcHitTestNewUI(WPARAM wParam, LPARAM lParam )
{
	// custom processing of our min/max/close buttons
	CRect rc;
	GetWindowRect(&rc);

	/*
	long nBoxStatus[4];
	memcpy(nBoxStatus, m_nBoxStatus, sizeof(nBoxStatus));
	//SVP_LogMsg(_T("OnNcHitTestNewUI"));

	CRect rcClose (rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH, rc.top+NEWUI_BTN_MARGIN_TOP, rc.right-NEWUI_BTN_WIDTH, rc.top+NEWUI_BTN_MARGIN_TOP+NEWUI_BTN_HEIGTH);
	CRect rcMax (rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*2, rc.top+NEWUI_BTN_MARGIN_TOP, rc.right-NEWUI_BTN_WIDTH*2, rc.top+NEWUI_BTN_MARGIN_TOP+NEWUI_BTN_HEIGTH);
	CRect rcMin (rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*3, rc.top+NEWUI_BTN_MARGIN_TOP, rc.right-NEWUI_BTN_WIDTH*3, rc.top+NEWUI_BTN_MARGIN_TOP+NEWUI_BTN_HEIGTH);
	CRect rcMenu (rc.right-NEWUI_BTN_MARGIN_RIGHT-NEWUI_BTN_WIDTH*4, rc.top+NEWUI_BTN_MARGIN_TOP, rc.right-NEWUI_BTN_WIDTH*4, rc.top+NEWUI_BTN_MARGIN_TOP+NEWUI_BTN_HEIGTH);

	CPoint pt(lParam);
	SHORT bLBtnDown = GetAsyncKeyState(VK_LBUTTON);  
	if (rcClose.PtInRect(pt) && bLBtnDown) m_nBoxStatus[0] = 2; else if (rcClose.PtInRect(pt)) m_nBoxStatus[0] = 1; else m_nBoxStatus[0] = 0;
	if (rcMax.PtInRect(pt) && bLBtnDown) m_nBoxStatus[1] = 2; else if (rcMax.PtInRect(pt)) m_nBoxStatus[1] = 1; else m_nBoxStatus[1] = 0;
	if (rcMin.PtInRect(pt) && bLBtnDown) m_nBoxStatus[2] = 2; else if (rcMin.PtInRect(pt)) m_nBoxStatus[2] = 1; else m_nBoxStatus[2] = 0;
	if (rcMenu.PtInRect(pt) && bLBtnDown) m_nBoxStatus[3] = 2; else if (rcMenu.PtInRect(pt)) m_nBoxStatus[3] = 1; else m_nBoxStatus[3] = 0;

	if (m_nBoxStatus[0] != nBoxStatus[0] || m_nBoxStatus[1] != nBoxStatus[1] || m_nBoxStatus[2] != nBoxStatus[2] || m_nBoxStatus[3] != nBoxStatus[3]){
	//SVP_LogMsg(_T("RedrawNonClientArea"));
	RedrawNonClientArea();
	}
	if (m_nBoxStatus[0]!=0) return HTCLOSE;
	else if (m_nBoxStatus[1]!=0) return HTMAXBUTTON;
	else if (m_nBoxStatus[2]!=0) return HTMINBUTTON;
	else if (m_nBoxStatus[3]!=0) return HTMENU;*/

	CPoint pt(lParam);
	UINT ret = m_btnList.OnHitTest(pt,rc);
	if( m_btnList.HTRedrawRequired ){
		RedrawNonClientArea();
	}
	if(ret){
		return ret;
	}
	CRect rcMenu (rc.left + 3, rc.top + 3,rc.left + 23 , rc.top + 15 );
	if (rcMenu.PtInRect(pt) ){
		return HTMENU;
	}
	LRESULT lHTESTID = DefWindowProc(WM_NCHITTEST, wParam, lParam);
	if(lHTESTID == HTCLOSE || lHTESTID == HTMAXBUTTON || lHTESTID == HTMINBUTTON || lHTESTID == HTMENU || lHTESTID == HTMINTOTRAY ){
		return HTCAPTION;
	}
	if(HTSYSMENU == lHTESTID){
		return HTMENU;
	}
	
	return lHTESTID;
} 

void CMainFrame::RedrawNonClientArea()
{
	// We can't use InvalidateRect to repaint the frame, it's for client area.
	// To repaint the frame (or non-client area), we call SetWindowPos with
	// flag SWP_FRAMECHANGED.
	//
	// This function is written for convenience.
	//
	// Note that Windows doesn't always send WM_NCPAINT when the frame needs to be painted.
	// It's either a bug or problematic code or faulty design that certain API calls may attempt to draw
	// the window frame without WM_NCPAINT. An example of this is the SetWindowText API.
	// When SetWindowText API is called, it uses GetWindowDC, and DrawText to paint directly. Yes,
	// it doesn't send WM_NCPAINT like it should. This is important information for this custom window
	// painting sample because we certainly don't want Windows to paint our window ugly.
	// However, there are no great cure for this problem. We'll have to remember call this
	// RedrawNonClientArea function every time after SetWindowText, or other similar calls.
	// In certain situations this might cause a flicker because two painting processes are done.
	// So it's better to use less or avoid these types of calls.
	if(m_wndToolBar.IsVisible())
		m_wndToolBar.Invalidate();
	if(m_wndSeekBar.IsVisible())
		m_wndSeekBar.Invalidate();

	SetWindowPos(NULL,0,0,0,0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}

/*
void CMainFrame::PreMultiplyBitmap( CBitmap& bmp )
{
// this is only possible when the bitmap is loaded using the
// ::LoadImage API with flag LR_CREATEDIBSECTION

BITMAP bm;
bmp.GetBitmap(&bm);
for (int y=0; y<bm.bmHeight; y++)
{
BYTE * pPixel = (BYTE *) bm.bmBits + bm.bmWidth * 4 * y;
for (int x=0; x<bm.bmWidth; x++)
{
pPixel[0] = pPixel[0] * pPixel[3] / 255; 
pPixel[1] = pPixel[1] * pPixel[3] / 255; 
pPixel[2] = pPixel[2] * pPixel[3] / 255; 
pPixel += 4;
}
}
}*/
void CMainFrame::OnViewCompact()
{
	if(AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(CS_TOOLBAR|CS_SEEKBAR);
}

void CMainFrame::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	DWORD style = GetStyle();

	/*
	MENUBARINFO mbi;
	memset(&mbi, 0, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);
	*/

	lpMMI->ptMinTrackSize.x = 100;
	if(!IsCaptionMenuHidden())
	{
		lpMMI->ptMinTrackSize.x = 10;
		CRect r;
		//for(int i = 0; ::GetMenuItemRect(m_hWnd, mbi.hMenu, i, &r); i++)
		//lpMMI->ptMinTrackSize.x += r.Width();
		lpMMI->ptMinTrackSize.x = max(310, lpMMI->ptMinTrackSize.x);
	}
	if(style&WS_THICKFRAME) lpMMI->ptMinTrackSize.x += GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2;

	/*
	memset(&mbi, 0, sizeof(mbi));
	mbi.cbSize = sizeof(mbi);
	::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);*/


	lpMMI->ptMinTrackSize.y = 40;
	if(style&WS_CAPTION) lpMMI->ptMinTrackSize.y += GetSystemMetrics(SM_CYCAPTION);
	if(style&WS_THICKFRAME) lpMMI->ptMinTrackSize.y += GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2;
	//lpMMI->ptMinTrackSize.y += (mbi.rcBar.bottom - mbi.rcBar.top);
	
	if(!AfxGetAppSettings().fHideCaptionMenu) lpMMI->ptMinTrackSize.y += 3;
	
	POSITION pos = m_bars.GetHeadPosition();
	while(pos) 
	{
		CControlBar* pCB = m_bars.GetNext(pos);
		if(!IsWindow(pCB->m_hWnd) || !pCB->IsVisible()) continue;

		lpMMI->ptMinTrackSize.y += pCB->CalcFixedLayout(TRUE, TRUE).cy;
	}

	pos = m_dockingbars.GetHeadPosition();
	while(pos)
	{
		CSizingControlBar* pCB = m_dockingbars.GetNext(pos);
		if(IsWindow(pCB->m_hWnd) && pCB->IsWindowVisible() && !pCB->IsFloating())
			lpMMI->ptMinTrackSize.y += pCB->CalcFixedLayout(TRUE, TRUE).cy-2;
	}
	//CString szLog;
	//szLog.Format(_T("MaxInfo %d %d %d %d %d %d ") , lpMMI->ptMaxSize.x , lpMMI->ptMaxSize.y, lpMMI->ptMaxTrackSize.x , lpMMI->ptMaxTrackSize.y, lpMMI->ptMaxPosition.x , lpMMI->ptMaxPosition.y);
	//SVP_LogMsg(szLog);

	
	//lpMMI->ptMinTrackSize.y = max(lpMMI->ptMinTrackSize.y, DEFCLIENTH);

	if(IsCaptionMenuHidden()){
		lpMMI->ptMinTrackSize.x = 80;
		lpMMI->ptMinTrackSize.y = 45;
	}
	lpMMI->ptMaxPosition.x = 0;
	lpMMI->ptMaxPosition.y = 0;

	__super::OnGetMinMaxInfo(lpMMI);

}

/*NEW UI END*/
void CMainFrame::OnSetHotkey(){
	CAutoPtr<CPPageAccelTbl> page(new CPPageAccelTbl());
	CPropertySheet dlg(_T("快捷键设置..."), this);
	dlg.AddPage(page);
	dlg.DoModal() ;
}
void CMainFrame::OnResetSetting(){
	if(AfxMessageBox(_T("此操作将清除射手播放器的\n所有设置和播放记录，您确定么？"), MB_YESNO) == IDYES){

		CMPlayerCApp* mApp = (CMPlayerCApp*)AfxGetApp();
		mApp->RemoveAllSetting();
	}
}
void CMainFrame::OnDestroy()
{
	ShowTrayIcon(false);

	m_fileDropTarget.Revoke();
	
	CloseMedia();

	if(m_pGraphThread)
	{
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_EXIT, 0, (LPARAM)&e);
		if(!e.Wait(5000))
		{
			TRACE(_T("ERROR: Must call TerminateThread() on CMainFrame::m_pGraphThread->m_hThread\n")); 
			TerminateThread(m_pGraphThread->m_hThread, -1);
		}
	}

	__super::OnDestroy();
}

void CMainFrame::OnClose()
{
	// save shader list
	POSITION pos;
	CString	strList = "";
		
	pos = m_shaderlabels.GetHeadPosition();
	while(pos)
	{
		strList += m_shaderlabels.GetAt (pos) + "|";
		m_dockingbars.GetNext(pos);
	}
	AfxGetAppSettings().strShaderList = strList;
	// end
	
	m_wndPlaylistBar.SavePlaylist();

	SaveControlBars();
 
	ShowWindow(SW_HIDE);

	CloseMedia();

	__super::OnClose();
}

void CMainFrame::OnEnterMenuLoop( BOOL bIsTrackPopupMenu ){
	bNoMoreHideMouse = true;
	//SendStatusMessage(_T("Meni Enter"), 2000);
}
void CMainFrame::OnExitMenuLoop( BOOL bIsTrackPopupMenu ){
	bNoMoreHideMouse = false;
	//SendStatusMessage(_T("Meni Exit"), 2000);
}

DROPEFFECT CMainFrame::OnDragEnter(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}
DROPEFFECT CMainFrame::OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point)
{
	UINT CF_URL = RegisterClipboardFormat(_T("UniformResourceLocator"));
	return pDataObject->IsDataAvailable(CF_URL) ? DROPEFFECT_COPY : DROPEFFECT_NONE;
}
BOOL CMainFrame::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	UINT CF_URL = RegisterClipboardFormat(_T("UniformResourceLocator"));

	BOOL bResult = FALSE;

	if(pDataObject->IsDataAvailable(CF_URL)) 
	{
		FORMATETC fmt = {CF_URL, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
		if(HGLOBAL hGlobal = pDataObject->GetGlobalData(CF_URL, &fmt))
		{
			LPCSTR pText = (LPCSTR)GlobalLock(hGlobal);
			if(AfxIsValidString(pText))
			{
				CStringA url(pText);

				SetForegroundWindow();

				CAtlList<CString> sl;
				sl.AddTail(CString(url));

				if(m_wndPlaylistBar.IsWindowVisible())
				{
					m_wndPlaylistBar.Append(sl, true);
				}
				else
				{
					m_wndPlaylistBar.Open(sl, true);
					OpenCurPlaylistItem();
				}

				GlobalUnlock(hGlobal);
				bResult = TRUE;
			}
		}
	}

	return bResult;
}
DROPEFFECT CMainFrame::OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropDefault, DROPEFFECT dropList, CPoint point)
{
	return (DROPEFFECT)-1;
}
void CMainFrame::OnDragLeave()
{
}
DROPEFFECT CMainFrame::OnDragScroll(DWORD dwKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

void CMainFrame::LoadControlBar(CControlBar* pBar, UINT defDockBarID)
{
	if(!pBar) return;

	CString str;
	pBar->GetWindowText(str);
	if(str.IsEmpty()) return;
	CString section = _T("ToolBars\\") + str;

	CWinApp* pApp = AfxGetApp();

	UINT nID = pApp->GetProfileInt(section, _T("DockState"), defDockBarID);

	if(nID != AFX_IDW_DOCKBAR_FLOAT)
	{
		DockControlBar(pBar, nID);
	}

	pBar->ShowWindow(
		pApp->GetProfileInt(section, _T("Visible"), FALSE) 
		&& pBar != &m_wndSubresyncBar 
		&& pBar != &m_wndCaptureBar
		&& pBar != &m_wndShaderEditorBar
		? SW_SHOW
		: SW_HIDE);

	if(CSizingControlBar* pSCB = dynamic_cast<CSizingControlBar*>(pBar))
	{
		pSCB->LoadState(section + _T("\\State"));
		m_dockingbars.AddTail(pSCB);
	}
}

void CMainFrame::RestoreFloatingControlBars()
{
	CWinApp* pApp = AfxGetApp();

	CRect r;
	GetWindowRect(r);

	POSITION pos = m_dockingbars.GetHeadPosition();
	while(pos)
	{
		CSizingControlBar* pBar = m_dockingbars.GetNext(pos);

		CString str;
		pBar->GetWindowText(str);
		if(str.IsEmpty()) return;
		CString section = _T("ToolBars\\") + str;

		if(pApp->GetProfileInt(section, _T("DockState"), ~AFX_IDW_DOCKBAR_FLOAT) == AFX_IDW_DOCKBAR_FLOAT)
		{
			CPoint p;
			p.x = pApp->GetProfileInt(section, _T("DockPosX"), r.right);
			p.y = pApp->GetProfileInt(section, _T("DockPosY"), r.top);
			if(p.x < m_rcDesktop.left) p.x = m_rcDesktop.left;
			if(p.y < m_rcDesktop.top) p.y = m_rcDesktop.top;
			if(p.x >= m_rcDesktop.right) p.x = m_rcDesktop.right-1;
			if(p.y >= m_rcDesktop.bottom) p.y = m_rcDesktop.bottom-1;
			FloatControlBar(pBar, p);
		}
	}
}

void CMainFrame::SaveControlBars()
{
	CWinApp* pApp = AfxGetApp();

	POSITION pos = m_dockingbars.GetHeadPosition();
	while(pos)
	{
		CSizingControlBar* pBar = m_dockingbars.GetNext(pos);

		CString str;
		pBar->GetWindowText(str);
		if(str.IsEmpty()) return;
		CString section = _T("ToolBars\\") + str;

		pApp->WriteProfileInt(section, _T("Visible"), pBar->IsWindowVisible());

		if(CSizingControlBar* pSCB = dynamic_cast<CSizingControlBar*>(pBar))
		{
			pSCB->SaveState(section + _T("\\State"));
		}

		UINT nID = pBar->GetParent()->GetDlgCtrlID();
		
		if(nID == AFX_IDW_DOCKBAR_FLOAT)
		{
			CRect r;
			pBar->GetParent()->GetParent()->GetWindowRect(r);
			pApp->WriteProfileInt(section, _T("DockPosX"), r.left);
			pApp->WriteProfileInt(section, _T("DockPosY"), r.top);
		}

		pApp->WriteProfileInt(section, _T("DockState"), nID);
	}
}

LRESULT CMainFrame::OnTaskBarRestart(WPARAM, LPARAM)
{
	m_fTrayIcon = false;
	ShowTrayIcon(AfxGetAppSettings().fTrayIcon);

	return 0;
}

LRESULT CMainFrame::OnNotifyIcon(WPARAM wParam, LPARAM lParam)
{
    if((UINT)wParam != IDR_MAINFRAME)
		return -1;

	switch((UINT)lParam)
	{
		case WM_LBUTTONDOWN:
			ShowWindow(SW_RESTORE);
			//ShowWindow(SW_SHOW);
			MoveVideoWindow();
			SetForegroundWindow();
			break;

		case WM_LBUTTONDBLCLK:
			PostMessage(WM_COMMAND, ID_FILE_OPENMEDIA);
			break;

		case WM_RBUTTONDOWN:
		{
			POINT p;
			GetCursorPos(&p);
			SetForegroundWindow();
			m_popup.GetSubMenu(0)->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, p.x, p.y, this);
			PostMessage(WM_NULL);
			break; 
		}

		case WM_MOUSEMOVE:
		{
			CString str;
			//GetWindowText(str);
			SetTrayTip(m_szTitle);
			break;
		}

		default: 
			break; 
	}

	return 0;
}
	
void CMainFrame::ShowTrayIcon(bool fShow)
{
    //BOOL bWasVisible = ShowWindow(SW_HIDE);
	NOTIFYICONDATA tnid; 

	ZeroMemory(&tnid, sizeof(NOTIFYICONDATA));
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_hWnd; 
	tnid.uID = IDR_MAINFRAME; 

	if(fShow)
	{
		if(!m_fTrayIcon)
		{
			tnid.hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			tnid.uFlags = NIF_MESSAGE|NIF_ICON|NIF_TIP; 
			tnid.uCallbackMessage = WM_NOTIFYICON; 
			lstrcpyn(tnid.szTip, TEXT("射手播放器"), sizeof(tnid.szTip)); 
			Shell_NotifyIcon(NIM_ADD, &tnid);

			m_fTrayIcon = true;
		}
	}
	else
	{
		if(m_fTrayIcon)
		{
			Shell_NotifyIcon(NIM_DELETE, &tnid); 

			m_fTrayIcon = false;
		}
	}

	//if(bWasVisible)
	//	ShowWindow(SW_SHOW);
}

void CMainFrame::SetTrayTip(CString str)
{
	NOTIFYICONDATA tnid; 
	tnid.cbSize = sizeof(NOTIFYICONDATA); 
	tnid.hWnd = m_hWnd; 
	tnid.uID = IDR_MAINFRAME; 
	tnid.uFlags = NIF_TIP; 
	lstrcpyn(tnid.szTip, str, sizeof(tnid.szTip)); 
	Shell_NotifyIcon(NIM_MODIFY, &tnid);
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if(!__super::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = MPC_WND_CLASS_NAME; //AfxRegisterWndClass(0);

	return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
/*		if(m_fShockwaveGraph
		&& (pMsg->wParam == VK_LEFT || pMsg->wParam == VK_RIGHT
			|| pMsg->wParam == VK_UP || pMsg->wParam == VK_DOWN))
			return FALSE;
*/
		if(pMsg->wParam == VK_ESCAPE && m_iMediaLoadState == MLS_LOADED && m_fFullScreen)
		{
			OnViewFullscreen();
			//PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
			return TRUE;
		}
		else if(pMsg->wParam == VK_ESCAPE && (IsCaptionMenuHidden()))
		{
			PostMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
			return TRUE;
		}
		else if(pMsg->wParam == VK_LEFT && pAMTuner)
		{
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACK);
			return TRUE;
		}
		else if(pMsg->wParam == VK_RIGHT && pAMTuner)
		{
			PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	__super::RecalcLayout(bNotify);

	CRect r;
	GetWindowRect(&r);
	MINMAXINFO mmi;
	memset(&mmi, 0, sizeof(mmi));
	SendMessage(WM_GETMINMAXINFO, 0, (LPARAM)&mmi);
	r |= CRect(r.TopLeft(), CSize(r.Width(), mmi.ptMinTrackSize.y));
	MoveWindow(&r);

	rePosOSD();

}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	__super::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	__super::Dump(dc);
}

#endif //_DEBUG
/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers
void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

	// forward focus to the view window
	if(IsWindow(m_wndView.m_hWnd))
		m_wndView.SetFocus();

	RedrawNonClientArea();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the view have first crack at the command
	if(m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	POSITION pos = m_bars.GetHeadPosition();
	while(pos) 
	{
		if(m_bars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	pos = m_dockingbars.GetHeadPosition();
	while(pos) 
	{
		if(m_dockingbars.GetNext(pos)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
			return TRUE;
	}

	// otherwise, do default handling
	return __super::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}





void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	__super::OnMoving(fwSide, pRect);

	if(AfxGetAppSettings().fSnapToDesktopEdges)
	{
		const CPoint threshold(3, 3);

		CRect r0 = m_rcDesktop;
		CRect r1 = r0 + threshold;
		CRect r2 = r0 - threshold;

		RECT& wr = *pRect;
		CSize ws = CRect(wr).Size();

		if(wr.left < r1.left && wr.left > r2.left)
			wr.right = (wr.left = r0.left) + ws.cx;

		if(wr.top < r1.top && wr.top > r2.top)
			wr.bottom = (wr.top = r0.top) + ws.cy;

		if(wr.right < r1.right && wr.right > r2.right)
			wr.left = (wr.right = r0.right) - ws.cx;

		if(wr.bottom < r1.bottom && wr.bottom > r2.bottom)
			wr.top = (wr.bottom = r0.bottom) - ws.cy;
	}
}

void CMainFrame::OnSizing(UINT fwSide, LPRECT pRect)
{
	__super::OnSizing(fwSide, pRect);
	
	AppSettings& s = AfxGetAppSettings();
	
	bool fCtrl = !!(GetAsyncKeyState(VK_CONTROL)&0x80000000);

	if(m_iMediaLoadState != MLS_LOADED || m_fFullScreen
	|| s.iDefaultVideoSize == DVS_STRETCH
	|| (fCtrl ^ s.fFreeWindowResizing))
		return;

	CSize wsize(pRect->right - pRect->left, pRect->bottom - pRect->top);
	CSize vsize = GetVideoSize();
	CSize fsize(0, 0);

	if(!vsize.cx || !vsize.cy)
		return;

	// TODO
	{
		DWORD style = GetStyle();

		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		fsize.cx += GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2;

		if(style&WS_CAPTION) fsize.cy += GetSystemMetrics(SM_CYCAPTION);
		if(style&WS_THICKFRAME) fsize.cy += GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2;
		fsize.cy += mbi.rcBar.bottom - mbi.rcBar.top;
		if(!AfxGetAppSettings().fHideCaptionMenu) fsize.cy += 3;

		POSITION pos = m_bars.GetHeadPosition();
		while(pos) 
		{
			CControlBar* pCB = m_bars.GetNext(pos);
			if(IsWindow(pCB->m_hWnd) && pCB->IsVisible())
				fsize.cy += pCB->CalcFixedLayout(TRUE, TRUE).cy;
		}

		pos = m_dockingbars.GetHeadPosition();
		while(pos)
		{
			CSizingControlBar* pCB = m_dockingbars.GetNext(pos);
			
			if(IsWindow(pCB->m_hWnd) && pCB->IsWindowVisible())
			{
				if(pCB->IsHorzDocked()) fsize.cy += pCB->CalcFixedLayout(TRUE, TRUE).cy-2;
				else if(pCB->IsVertDocked()) fsize.cx += pCB->CalcFixedLayout(TRUE, FALSE).cx;
			}
		}
	}

	wsize -= fsize;

	bool fWider = wsize.cy < wsize.cx;

	wsize.SetSize(
		wsize.cy * vsize.cx / vsize.cy,
		wsize.cx * vsize.cy / vsize.cx);

	wsize += fsize;

	if(fwSide == WMSZ_TOP || fwSide == WMSZ_BOTTOM || !fWider && (fwSide == WMSZ_TOPRIGHT || fwSide == WMSZ_BOTTOMRIGHT))
	{
		pRect->right = pRect->left + wsize.cx;
	}
	else if(fwSide == WMSZ_LEFT || fwSide == WMSZ_RIGHT || fWider && (fwSide == WMSZ_BOTTOMLEFT || fwSide == WMSZ_BOTTOMRIGHT))
	{
		pRect->bottom = pRect->top + wsize.cy;
	}
	else if(!fWider && (fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_BOTTOMLEFT))
	{
		pRect->left = pRect->right - wsize.cx;
	}
	else if(fWider && (fwSide == WMSZ_TOPLEFT || fwSide == WMSZ_TOPRIGHT))
	{
		pRect->top = pRect->bottom - wsize.cy;
	}
}

void CMainFrame::OnDisplayChange() // untested, not sure if it's working...
{
	TRACE(_T("*** CMainFrame::OnDisplayChange()\n"));
/*
	if(m_iMediaLoadState == MLS_LOADED && m_pCAP) 
		m_pCAP->OnDisplayChange();
*/

	GetDesktopWindow()->GetWindowRect(&m_rcDesktop);
}

#include <psapi.h>

void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	if( (GetMediaState() == State_Running) && (((nID & 0xFFF0) == SC_SCREENSAVE) || ((nID & 0xFFF0) == SC_MONITORPOWER)) )
	{
		TRACE(_T("SC_SCREENSAVE, nID = %d, lParam = %d\n"), nID, lParam);
		return;
	}
	else if((nID & 0xFFF0) == SC_MINIMIZE && m_fTrayIcon)
	{
		ShowWindow(SW_HIDE);
		return;
	}

	__super::OnSysCommand(nID, lParam);
}

void CMainFrame::OnActivateApp(BOOL bActive, DWORD dwThreadID)
{
	if(bActive){
		this->PreFocused();
	}
	__super::OnActivateApp(bActive, dwThreadID);

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

	if(!bActive && (mi.dwFlags&MONITORINFOF_PRIMARY) && m_fFullScreen && m_iMediaLoadState == MLS_LOADED)
	{
		bool fExitFullscreen = true;

		if(CWnd* pWnd = GetForegroundWindow())
		{
			HMONITOR hMonitor1 = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
			HMONITOR hMonitor2 = MonitorFromWindow(pWnd->m_hWnd, MONITOR_DEFAULTTONEAREST);
			if(hMonitor1 && hMonitor2 && hMonitor1 != hMonitor2) fExitFullscreen = false;

			CString title;
			pWnd->GetWindowText(title);

			CString module;

			if(GetVersion()&0x80000000)
			{
				module.ReleaseBufferSetLength(GetWindowModuleFileName(pWnd->m_hWnd, module.GetBuffer(MAX_PATH), MAX_PATH));
			}
			else
			{
				DWORD pid; 
				GetWindowThreadProcessId(pWnd->m_hWnd, &pid); 

				if(HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid))
				{
					HMODULE hMod; 
					DWORD cbNeeded; 

					if(EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					{
						module.ReleaseBufferSetLength(GetModuleFileNameEx(hProcess, hMod, module.GetBuffer(MAX_PATH), MAX_PATH)); 
					} 

					CloseHandle(hProcess); 
				} 
			}

			CPath p(module);
			p.StripPath();
			module = (LPCTSTR)p;
			module.MakeLower();

			CString str;
			str.Format(_T("Focus lost to: %s - %s"), module, title);
			SendStatusMessage(str, 5000);
		}

		//if(fExitFullscreen) OnViewFullscreen();
	}
	if(!bActive){
		RedrawNonClientArea();
	}
}

LRESULT CMainFrame::OnAppCommand(WPARAM wParam, LPARAM lParam)
{
	UINT cmd  = GET_APPCOMMAND_LPARAM(lParam);
	UINT uDevice = GET_DEVICE_LPARAM(lParam);
	UINT dwKeys = GET_KEYSTATE_LPARAM(lParam);

	if(uDevice != FAPPCOMMAND_OEM)
	{
		AppSettings& s = AfxGetAppSettings();

		BOOL fRet = FALSE;

		POSITION pos = s.wmcmds.GetHeadPosition();
		while(pos)
		{
			wmcmd& wc = s.wmcmds.GetNext(pos);
			if(wc.appcmd == cmd && TRUE == SendMessage(WM_COMMAND, wc.cmd)) 
				fRet = TRUE;
		}

		if(fRet) return TRUE;
	}

	return Default();
}
//Play Toolbar Set Sub Delay
void CMainFrame::OnPlaySubDelay(UINT nID)
{
	int oldDelay = m_pCAP->GetSubtitleDelay();

	int newDelay = 0;
		
	switch(nID - ID_SUBDELAYDEC){
		case 0:
			newDelay = oldDelay-AfxGetAppSettings().nSubDelayInterval;
			break;
		case 2:
			newDelay = oldDelay+AfxGetAppSettings().nSubDelayInterval;
			break;
		default:
			//getCurPlayingSubfile(&newDelay);
			if(m_pSubStreams.GetCount() <= 0){
				SendMessage(WM_COMMAND , ID_FILE_LOAD_SUBTITLE, 0);
				return;
			}
			if(m_iSubtitleSel < 0){
				m_iSubtitleSel = 0;
			}else{
				POSITION pos = m_pSubStreams.GetHeadPosition();
				int iTotalSub = 0;
				while(pos )
				{
					iTotalSub += m_pSubStreams.GetNext(pos)->GetStreamCount();
				}
				m_iSubtitleSel++;
				if(m_iSubtitleSel >= iTotalSub ){
					m_iSubtitleSel ^= (1<<31);
				}
			}
			UpdateSubtitle();	
			return;
			break;
	}
	CString str;
	str.Format(_T("主字幕延时已经设为： %d ms"), newDelay);
	SendStatusMessage(str, 5000);
	this->SetSubtitleDelay(newDelay);
}
void CMainFrame::OnUpdateSubtitleFontChange(CCmdUI* pCmdUI)
{
	bool fEnable = false;
	if(m_pSubStreams.GetCount() > 0 && m_iSubtitleSel >= 0 ) 
		fEnable = true;
	pCmdUI->Enable(fEnable);
}
bool CMainFrame::IsSubLoaded(){
	if( (m_pSubStreams.GetCount() > 0 &&  ( m_iSubtitleSel >= 0 || m_iSubtitleSel2 >= 0 )  ) &&  IsSomethingLoaded()) 
		return true;

	return false;
}
void CMainFrame::OnUpdatePlaySubDelay(CCmdUI* pCmdUI)
{
	bool fEnable = false;
	if( (m_pSubStreams.GetCount() > 0 &&  m_iSubtitleSel >= 0  ) || ( pCmdUI->m_nID == ID_SUBLANGSWITCH && IsSomethingLoaded())) 
		fEnable = true;
	pCmdUI->Enable(fEnable);
}
void CMainFrame::OnPlaySub2Delay(UINT nID)
{
	int oldDelay = m_pCAP->GetSubtitleDelay2();

	int newDelay = 0;

	switch(nID - ID_SUB2DELAYDEC){
		case 0:
			newDelay = oldDelay-AfxGetAppSettings().nSubDelayInterval;
			break;
		case 2:
			newDelay = oldDelay+AfxGetAppSettings().nSubDelayInterval;
			break;
		default:
			//getCurPlayingSubfile(&newDelay, 2);
			break;
	}
	CString str;
	str.Format(_T("第二字幕延时已经设为： %d ms"), newDelay);
	SendStatusMessage(str, 5000);
	this->SetSubtitleDelay2(newDelay);
}
void CMainFrame::OnUpdatePlaySub2Delay(CCmdUI* pCmdUI)
{
	bool fEnable = false;
	if(m_pSubStreams2.GetCount() > 0 && m_iSubtitleSel2 >= 0 && m_iSubtitleSel2 != m_iSubtitleSel) {
		fEnable = true;
	}
	pCmdUI->Enable(fEnable);
}
CString CMainFrame::getCurPlayingSubfile(int * iSubDelayMS,int subid ){
	CString fnSubtitleFile = _T("");
	if(m_pCAP) {

		int i = m_iSubtitleSel;
		if(subid){ i = m_iSubtitleSel2;}
		CComPtr<ISubStream> pSubStream;
		POSITION pos = m_pSubStreams.GetHeadPosition();
		while(pos && i >= 0)
		{
			pSubStream = m_pSubStreams.GetNext(pos);

			if(i < pSubStream->GetStreamCount())
			{
				break;
			}

			i -= pSubStream->GetStreamCount();
		}
		if (pSubStream){
			int iSubDelay_ms;
			iSubDelay_ms = m_pCAP->GetSubtitleDelay();			
			if(subid){	iSubDelay_ms = m_pCAP->GetSubtitleDelay2();	}
			if (iSubDelayMS ){
				*iSubDelayMS = iSubDelay_ms;
			}
			CLSID clsid;
			pSubStream->GetClassID(&clsid);

			if(clsid == __uuidof(CVobSubFile))
			{
				CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;
				fnSubtitleFile = pVSF->m_title + _T(".idx");
			}
			else if(clsid == __uuidof(CRenderedTextSubtitle))
			{
				CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;
				fnSubtitleFile = pRTS->m_path;

			}
			if(  !pSubStream->notSaveDelay ){
				CSVPToolBox svpTool;
				CString szBuf;
				szBuf.Format(_T("%d"), iSubDelay_ms);
				svpTool.filePutContent(fnSubtitleFile+_T(".delay"),szBuf );
				pSubStream->sub_delay_ms = iSubDelay_ms;
				if(!iSubDelay_ms){
					_wunlink(fnSubtitleFile+_T(".delay"));
				}
			}
		}
	}
	return fnSubtitleFile;
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	if(TIMER_DELETE_CUR_FILE == nIDEvent){
		CSVPToolBox svpTool;
		if( svpTool.ifFileExist(fnDelPending, true) ){
			_wunlink(fnDelPending);
		}else{
			fnDelPending.Empty();
			KillTimer(TIMER_DELETE_CUR_FILE);
		}
		
		
	}else if(TIMER_DELETE_CUR_FOLDER == nIDEvent){
		CSVPToolBox svpTool;
		if( svpTool.ifDirExist(fnDelPending) ){
			svpTool.delDirRecursive(fnDelPending);
		}else{
			fnDelPending.Empty();
			KillTimer(TIMER_DELETE_CUR_FOLDER);
		}


	}else if(TIMER_START_CHECKUPDATER == nIDEvent){
		KillTimer(TIMER_START_CHECKUPDATER);
		SVP_CheckUpdaterExe( &m_bCheckingUpdater );
	}else if(TIMER_RECENTFOCUSED== nIDEvent){
		//bRecentFocused = FALSE;
		//KillTimer(TIMER_RECENTFOCUSED);
	}else if(TIMER_MOUSELWOWN == nIDEvent){
		if(s_fLDown){
			OnButton(wmcmd::LDOWN, NULL, NULL);
		}
		KillTimer(TIMER_MOUSELWOWN);
	}else if(nIDEvent == TIMER_STREAMPOSPOLLER && m_iMediaLoadState == MLS_LOADED)
	{
		REFERENCE_TIME rtNow = 0, rtDur = 0;

		if(m_iPlaybackMode == PM_FILE)
		{
			pMS->GetCurrentPosition(&rtNow);
			pMS->GetDuration(&rtDur);

			UINT iTotalLenSec = (UINT)( (INT64) rtDur / 20000000 );
			//如果视频长度大于1分钟， 而且是文件模式，而且正在播放中
			if ( iTotalLenSec >  180 && m_iPlaybackMode == PM_FILE && GetMediaState() == State_Running) {
				
				time_t time_now = time(NULL);
				
				UINT totalplayedtime =  time_now - m_tPlayStartTime;

				if( time_now > ( m_tLastLogTick + 180 )){ //如果和上次检查已经超n秒
					CString fnVideoFile , fnSubtitleFile; 
					int subDelayMS = 0;
					fnVideoFile = m_fnCurPlayingFile;
					fnSubtitleFile = getCurPlayingSubfile(&subDelayMS);
					
					
					if (!fnSubtitleFile.IsEmpty()){ //如果有字幕
						CString szLog;

						szLog.Format(_T(" %s ( with sub %s delay %d ) %d sec of %d sec ( 1/2 length video = %d ) ") , fnVideoFile, fnSubtitleFile,subDelayMS, totalplayedtime , iTotalLenSec, (UINT)(iTotalLenSec/2)  );
						SVP_LogMsg(szLog);
						//if time > 50%
						if (totalplayedtime > (UINT)(iTotalLenSec/2)){
							//是否已经上传过呢
							if(m_fnsAlreadyUploadedSubfile.Find( fnVideoFile+fnSubtitleFile ) < 0 ){
								//upload subtitle
								szLog.Format(_T("Uploading sub %s of %s width delay %d ms since user played %d sec of %d sec ( more than 1/2 length video ) ") , fnSubtitleFile, fnVideoFile ,subDelayMS, totalplayedtime , iTotalLenSec  );
								SVP_LogMsg(szLog);
								SVP_UploadSubFileByVideoAndSubFilePath(fnVideoFile , fnSubtitleFile, subDelayMS) ;
								m_fnsAlreadyUploadedSubfile.Append( fnVideoFile+fnSubtitleFile+_T(";") );
							}

						}
					}
					

					m_tLastLogTick = time_now;
				}
	
				
			}
			if(m_rtDurationOverride >= 0) rtDur = m_rtDurationOverride;

			m_wndSeekBar.Enable(rtDur > 0);
			m_wndSeekBar.SetRange(0, rtDur);
			m_wndSeekBar.SetPos(rtNow);
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			if(m_fCapturing && m_wndCaptureBar.m_capdlg.m_pMux)
			{
				CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
				if(!pMuxMS || FAILED(pMuxMS->GetCurrentPosition(&rtNow))) rtNow = 0;
			}

			if(m_rtDurationOverride >= 0) rtDur = m_rtDurationOverride;
            
			m_wndSeekBar.Enable(false);
			m_wndSeekBar.SetRange(0, rtDur);
			m_wndSeekBar.SetPos(rtNow);
/*
			if(m_fCapturing)
			{
				if(rtNow > 10000i64*1000*60*60*3)
				{
					m_wndCaptureBar.m_capdlg.OnRecord();
				}
			}
*/
		}

		if(m_pCAP && m_iPlaybackMode != PM_FILE) m_pCAP->SetTime(/*rtNow*/m_wndSeekBar.GetPos());
	}
	else if(nIDEvent == TIMER_STREAMPOSPOLLER2 && m_iMediaLoadState == MLS_LOADED)
	{
		__int64 start, stop, pos;
		m_wndSeekBar.GetRange(start, stop);
		pos = m_wndSeekBar.GetPosReal();

		GUID tf;
		pMS->GetTimeFormat(&tf);

		if(m_iPlaybackMode == PM_CAPTURE && !m_fCapturing)
		{
			CString str = _T("Live");

			long lChannel = 0, lVivSub = 0, lAudSub = 0;
			if(pAMTuner 
			&& m_wndCaptureBar.m_capdlg.IsTunerActive()
			&& SUCCEEDED(pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub)))
			{
				CString ch;
				ch.Format(_T(" (ch%d)"), lChannel);
				str += ch;
			}

			//m_wndStatusBar.SetStatusTimer(str);
			m_wndToolBar.SetStatusTimer(str);
		}
		else
		{
			double pRate;
			if(E_NOTIMPL == pMS->GetRate(&pRate)){
				pRate = 0;
			}
			m_wndToolBar.SetStatusTimer(pos, stop, !!m_wndSubresyncBar.IsWindowVisible(), &tf , pRate); 
			//m_wndToolBar.SetStatusTimer(str);
		}

		m_wndSubresyncBar.SetTime(pos);

		if(m_pCAP && GetMediaState() == State_Paused) m_pCAP->Paint(true);
	}else if(nIDEvent == TIMER_STATUSBARHIDER)
	{
		if (!( AfxGetAppSettings().nCS & CS_STATUSBAR)){
			ShowControlBar(&m_wndStatusBar, false, false);
		}
		bNotHideColorControlBar = false;
		KillTimer(TIMER_STATUSBARHIDER);
	}
	else if(nIDEvent == TIMER_FULLSCREENCONTROLBARHIDER)
	{
		CPoint p;
		GetCursorPos(&p);

		CRect r;
		GetWindowRect(r);
		bool fCursorOutside = !r.PtInRect(p);

		CWnd* pWnd = WindowFromPoint(p);
		if(pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside))
		{
			if(AfxGetAppSettings().nShowBarsWhenFullScreenTimeOut >= 0)
				ShowControls(CS_NONE, false);
		}
	}
	else if(nIDEvent == TIMER_FULLSCREENMOUSEHIDER)
	{
		if(!bNoMoreHideMouse){
			CPoint p;
			GetCursorPos(&p);

			CRect r;
			GetWindowRect(r);
			bool fCursorOutside = !r.PtInRect(p);

			CWnd* pWnd = WindowFromPoint(p);
			if(pWnd && (m_wndView == *pWnd || m_wndView.IsChild(pWnd) || fCursorOutside))
			{
				m_fHideCursor = true;
				SetCursor(NULL);
			}
		}
	}
	else if(nIDEvent == TIMER_STATS)
	{
		if(pQP)
		{
			CString rate;
			rate.Format(_T("(%0.1fx)"), 1.0 + (m_iSpeedLevel * 0.1) );
			
			/*
			if(m_iSpeedLevel >= -11 && m_iSpeedLevel <= 3 && m_iSpeedLevel != -4)
			{
				CString speeds[] = {_T("1/8"),_T("1/4"),_T("1/2"),_T("1"),_T("2"),_T("4"),_T("8")};
				rate = speeds[(m_iSpeedLevel >= -3 ? m_iSpeedLevel : (-m_iSpeedLevel - 8)) + 3];
				if(m_iSpeedLevel < -4) rate = _T("-") + rate;
				if(!rate.IsEmpty()) rate = _T("(") + rate + _T("X)");
			}
			*/

			CString info;
			int val;

			pQP->get_AvgFrameRate(&val);
			info.Format(_T("%d.%02d %s"), val/100, val%100, rate);
			m_wndStatsBar.SetLine(_T("Frame-rate"), info);

			int avg, dev;
			pQP->get_AvgSyncOffset(&avg);
			pQP->get_DevSyncOffset(&dev);
			info.Format(_T("avg: %d ms, dev: %d ms"), avg, dev);
			m_wndStatsBar.SetLine(_T("Sync Offset"), info);

			int drawn, dropped;
			pQP->get_FramesDrawn(&drawn);
			pQP->get_FramesDroppedInRenderer(&dropped);
			info.Format(_T("drawn: %d, dropped: %d"), drawn, dropped);
			m_wndStatsBar.SetLine(_T("Frames"), info);

			pQP->get_Jitter(&val);
			info.Format(_T("%d ms"), val);
			m_wndStatsBar.SetLine(_T("Jitter"), info);
		}

		if(pBI)
		{
			CAtlList<CString> sl;
			
			for(int i = 0, j = pBI->GetCount(); i < j; i++)
			{
				int samples, size;
				if(S_OK == pBI->GetStatus(i, samples, size))
				{
					CString str;
					str.Format(_T("[%d]: %03d/%d KB"), i, samples, size / 1024);
					sl.AddTail(str);
				}
			}

			if(!sl.IsEmpty())
			{
				CString str;
				str.Format(_T("%s (p%d)"), Implode(sl, ' '), pBI->GetPriority());
				
				m_wndStatsBar.SetLine(_T("Buffers"), str);
			}
		}

		CInterfaceList<IBitRateInfo> pBRIs;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			BeginEnumPins(pBF, pEP, pPin)
			{
				if(CComQIPtr<IBitRateInfo> pBRI = pPin)
				{
					pBRIs.AddTail(pBRI);
				}
			}
			EndEnumPins

			if(!pBRIs.IsEmpty())
			{
				CAtlList<CString> sl;

				POSITION pos = pBRIs.GetHeadPosition();
				for(int i = 0; pos; i++)
				{
					IBitRateInfo* pBRI = pBRIs.GetNext(pos);

					DWORD cur = pBRI->GetCurrentBitRate() / 1000;
					DWORD avg = pBRI->GetAverageBitRate() / 1000;

					if(avg == 0) continue;

					CString str;
					if(cur != avg) str.Format(_T("[%d]: %d/%d Kb/s"), i, avg, cur);
					else str.Format(_T("[%d]: %d Kb/s"), i, avg);
					sl.AddTail(str);
				}

				if(!sl.IsEmpty())
				{
					m_wndStatsBar.SetLine(_T("Bitrate"), Implode(sl, ' ') + _T(" (avg/cur)"));
				}

				break;
			}
		}
		EndEnumFilters

		if(m_iPlaybackMode == PM_FILE)
		{
			SetupChapters();
		}

		if(m_iPlaybackMode == PM_DVD) // we also use this timer to update the info panel for dvd playback
		{
			ULONG ulAvailable, ulCurrent;

			// Location

			CString Location('-');

			DVD_PLAYBACK_LOCATION2 loc;
			ULONG ulNumOfVolumes, ulVolume;
			DVD_DISC_SIDE Side;
			ULONG ulNumOfTitles;
			ULONG ulNumOfChapters;

			if(SUCCEEDED(pDVDI->GetCurrentLocation(&loc))
			&& SUCCEEDED(pDVDI->GetNumberOfChapters(loc.TitleNum, &ulNumOfChapters))
			&& SUCCEEDED(pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles)))
			{
				Location.Format(_T("Volume: %02d/%02d, Title: %02d/%02d, Chapter: %02d/%02d"), 
					ulVolume, ulNumOfVolumes, 
					loc.TitleNum, ulNumOfTitles, 
					loc.ChapterNum, ulNumOfChapters);
			}

			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_LOCATION), Location);

			// Video

			CString Video('-');

			DVD_VideoAttributes VATR;

			if(SUCCEEDED(pDVDI->GetCurrentAngle(&ulAvailable, &ulCurrent))
			&& SUCCEEDED(pDVDI->GetCurrentVideoAttributes(&VATR)))
			{
				Video.Format(_T("Angle: %02d/%02d, %dx%d %dHz %d:%d"), 
					ulAvailable, ulCurrent,
					VATR.ulSourceResolutionX, VATR.ulSourceResolutionY, VATR.ulFrameRate,
					VATR.ulAspectX, VATR.ulAspectY);
			}

			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_VIDEO), Video);

			// Audio

			CString Audio('-');

			DVD_AudioAttributes AATR;

			if(SUCCEEDED(pDVDI->GetCurrentAudio(&ulAvailable, &ulCurrent))
			&& SUCCEEDED(pDVDI->GetAudioAttributes(ulCurrent, &AATR)))
			{
				CString lang;
				int len = GetLocaleInfo(AATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
				lang.ReleaseBufferSetLength(max(len-1, 0));

				switch(AATR.LanguageExtension)
				{
				case DVD_AUD_EXT_NotSpecified:
				default: break;
				case DVD_AUD_EXT_Captions: lang += _T(" (Captions)"); break;
				case DVD_AUD_EXT_VisuallyImpaired: lang += _T(" (Visually Impaired)"); break;
				case DVD_AUD_EXT_DirectorComments1: lang += _T(" (Director Comments 1)"); break;
				case DVD_AUD_EXT_DirectorComments2: lang += _T(" (Director Comments 2)"); break;
				}

				CString format;
				switch(AATR.AudioFormat)
				{
				case DVD_AudioFormat_AC3: format = _T("AC3"); break;
				case DVD_AudioFormat_MPEG1: 
				case DVD_AudioFormat_MPEG1_DRC: format = _T("MPEG1"); break;
				case DVD_AudioFormat_MPEG2: 
				case DVD_AudioFormat_MPEG2_DRC: format = _T("MPEG2"); break;
				case DVD_AudioFormat_LPCM: format = _T("LPCM"); break;
				case DVD_AudioFormat_DTS: format = _T("DTS"); break;
				case DVD_AudioFormat_SDDS: format = _T("SDDS"); break;
				case DVD_AudioFormat_Other: 
				default: format = _T("Unknown format"); break;
				}

				Audio.Format(_T("%s, %s %dHz %dbits %d channel(s)"), 
					lang, 
					format,
					AATR.dwFrequency,
					AATR.bQuantization,
					AATR.bNumberOfChannels);

				m_wndStatusBar.SetStatusBitmap(
					AATR.bNumberOfChannels == 1 ? IDB_MONO 
					: AATR.bNumberOfChannels >= 2 ? IDB_STEREO 
					: IDB_NOAUDIO);
			}

			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUDIO), Audio);

			// Subtitles

			CString Subtitles('-');

			BOOL bIsDisabled;
			DVD_SubpictureAttributes SATR;

			if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulAvailable, &ulCurrent, &bIsDisabled))
			&& SUCCEEDED(pDVDI->GetSubpictureAttributes(ulCurrent, &SATR)))
			{
				CString lang;
				int len = GetLocaleInfo(SATR.Language, LOCALE_SENGLANGUAGE, lang.GetBuffer(64), 64);
				lang.ReleaseBufferSetLength(max(len-1, 0));

				switch(SATR.LanguageExtension)
				{
				case DVD_SP_EXT_NotSpecified:
				default: break;
				case DVD_SP_EXT_Caption_Normal: lang += _T(""); break;
				case DVD_SP_EXT_Caption_Big: lang += _T(" (Big)"); break;
				case DVD_SP_EXT_Caption_Children: lang += _T(" (Children)"); break;
				case DVD_SP_EXT_CC_Normal: lang += _T(" (CC)"); break;
				case DVD_SP_EXT_CC_Big: lang += _T(" (CC Big)"); break;
				case DVD_SP_EXT_CC_Children: lang += _T(" (CC Children)"); break;
				case DVD_SP_EXT_Forced: lang += _T(" (Forced)"); break;
				case DVD_SP_EXT_DirectorComments_Normal: lang += _T(" (Director Comments)"); break;
				case DVD_SP_EXT_DirectorComments_Big: lang += _T(" (Director Comments, Big)"); break;
				case DVD_SP_EXT_DirectorComments_Children: lang += _T(" (Director Comments, Children)"); break;
				}

				if(bIsDisabled) lang = _T("-");

				Subtitles.Format(_T("%s"), 
					lang);
			}

			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), Subtitles);
		}

		if(GetMediaState() == State_Running)
		{
			UINT fSaverActive = 0;
			if(SystemParametersInfo(SPI_GETSCREENSAVEACTIVE, 0, (PVOID)&fSaverActive, 0))
			{
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
				SystemParametersInfo(SPI_SETSCREENSAVEACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
			}

			fSaverActive = 0;
			if(SystemParametersInfo(SPI_GETPOWEROFFACTIVE, 0, (PVOID)&fSaverActive, 0))
			{
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE, 0, 0, SPIF_SENDWININICHANGE); // this might not be needed at all...
				SystemParametersInfo(SPI_SETPOWEROFFACTIVE, fSaverActive, 0, SPIF_SENDWININICHANGE);
			}

			SetThreadExecutionState(ES_DISPLAY_REQUIRED|ES_SYSTEM_REQUIRED); //this is the right way, only this work under vista . no ES_CONTINUOUS  so it can goes to sleep when not playing
		}
	}
	else if(nIDEvent == TIMER_STATUSERASER)
	{
		KillTimer(TIMER_STATUSERASER);
		m_playingmsg.Empty();
		
		if(m_wndStatusBar.IsVisible() && m_fFullScreen){
			SetTimer(TIMER_FULLSCREENMOUSEHIDER, 2000, NULL); 
		}
	}else if(TIMER_STATUSCHECKER == nIDEvent){
		KillTimer(TIMER_STATUSCHECKER);
		if(m_statusmsgs.GetCount() > 0 ){
			if ( m_playingmsg != m_statusmsgs.GetHead()){
				SendStatusMessage( m_statusmsgs.GetHead() , 10000);
				m_statusmsgs.RemoveHead();
			}
		}
		SetTimer(TIMER_STATUSCHECKER, 1000, NULL);
	}

	__super::OnTimer(nIDEvent);
}

static bool SetShutdownPrivilege()
{
   HANDLE hToken; 
   TOKEN_PRIVILEGES tkp; 
 
   // Get a token for this process. 
 
   if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
	   return(false);
 
   // Get the LUID for the shutdown privilege. 
 
   LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
 
   tkp.PrivilegeCount = 1;  // one privilege to set    
   tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
   // Get the shutdown privilege for this process. 
 
   AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
 
   if(GetLastError() != ERROR_SUCCESS)
	   return false;

   return true;
}

bool CMainFrame::DoAfterPlaybackEvent()
{
	AppSettings& s = AfxGetAppSettings();

	bool fExit = false;

	if(s.nCLSwitches&CLSW_CLOSE)
	{
		fExit = true;
	}
	
	if(s.nCLSwitches&CLSW_STANDBY)
	{
		SetShutdownPrivilege();
		SetSystemPowerState(TRUE, TRUE);
		fExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	}
	else if(s.nCLSwitches&CLSW_HIBERNATE)
	{
		SetShutdownPrivilege();
		SetSystemPowerState(FALSE, TRUE);
		fExit = true; // TODO: unless the app closes, it will call standby or hibernate once again forever, how to avoid that?
	}
	else if(s.nCLSwitches&CLSW_SHUTDOWN)
	{
		SetShutdownPrivilege();
		ExitWindowsEx(EWX_SHUTDOWN|EWX_POWEROFF|EWX_FORCEIFHUNG, 0);
		fExit = true;
	}
	else if(s.nCLSwitches&CLSW_LOGOFF)
	{
		SetShutdownPrivilege();
		ExitWindowsEx(EWX_LOGOFF|EWX_FORCEIFHUNG, 0);
		fExit = true;
	}
	
	if(!fExit) return false;

	SendMessage(WM_COMMAND, ID_FILE_EXIT);

	return true;
}

//
// our WM_GRAPHNOTIFY handler
//
#include <comdef.h>
LRESULT CMainFrame::OnGraphNotify(WPARAM wParam, LPARAM lParam)
{
    HRESULT hr = S_OK;

	LONG evCode, evParam1, evParam2;
    while(pME && SUCCEEDED(pME->GetEvent(&evCode, (LONG_PTR*)&evParam1, (LONG_PTR*)&evParam2, 0)))
    {
		CString str;

		if(m_fCustomGraph)
		{
			if(EC_BG_ERROR == evCode)
			{
				str = CString((char*)evParam1);
			}
		}

		hr = pME->FreeEventParams(evCode, evParam1, evParam2);

        if(EC_COMPLETE == evCode)
        {
			AppSettings& s = AfxGetAppSettings();

			if(m_wndPlaylistBar.GetCount() <= 1)
			{
				m_nLoops++;

				if(DoAfterPlaybackEvent()) return hr;

				if(s.fLoopForever || m_nLoops < s.nLoops)
				{
					if(GetMediaState() == State_Stopped)
					{
						SendMessage(WM_COMMAND, ID_PLAY_PLAY);
					}
					else
					{
						LONGLONG pos = 0;
						pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

						if(GetMediaState() == State_Paused)
						{
							SendMessage(WM_COMMAND, ID_PLAY_PLAY);
						}
					}
				}
				else 
				{
					if(s.fRewind) SendMessage(WM_COMMAND, ID_PLAY_STOP);
					else m_fEndOfStream = true;
					SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	
					if(m_fFullScreen && s.fExitFullScreenAtTheEnd) 
						OnViewFullscreen();
				}
			}
			else if(m_wndPlaylistBar.GetCount() > 1)
			{
				if(m_wndPlaylistBar.IsAtEnd())
				{
					if(DoAfterPlaybackEvent()) return hr;

					m_nLoops++;
				}

				if(s.fLoopForever || m_nLoops < s.nLoops)
				{
					int nLoops = m_nLoops;
					PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
					m_nLoops = nLoops;
				}
				else 
				{
					if(m_fFullScreen && s.fExitFullScreenAtTheEnd) 
						OnViewFullscreen();

					if(s.fRewind)
					{
						AfxGetAppSettings().nCLSwitches |= CLSW_OPEN; // HACK
						PostMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARD);
					}
					else
					{
						m_fEndOfStream = true;
						PostMessage(WM_COMMAND, ID_PLAY_PAUSE);
					}
				}
			}
        }
		else if(EC_ERRORABORT == evCode)
		{
			TRACE(_T("EC_ERRORABORT, hr = %08x\n"), (HRESULT)evParam1);
//			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
//			m_closingmsg = _com_error((HRESULT)evParam1).ErrorMessage();
		}
		else if(EC_REPAINT == evCode)
		{
			TRACE(_T("EC_REPAINT\n"));
		}
		else if(EC_BUFFERING_DATA == evCode)
		{
			TRACE(_T("EC_BUFFERING_DATA, %d, %d\n"), (HRESULT)evParam1, evParam2);

			m_fBuffering = ((HRESULT)evParam1 != S_OK);
		}
		else if(EC_STEP_COMPLETE == evCode)
		{
			if(m_fFrameSteppingActive)
			{
				m_fFrameSteppingActive = false;
				pBA->put_Volume(m_VolumeBeforeFrameStepping);
			}
		}
		else if(EC_DEVICE_LOST == evCode)
		{
			CComQIPtr<IBaseFilter> pBF;
			if(m_iPlaybackMode == PM_CAPTURE 
			&& (!pVidCap && pVidCap == (pBF = (IUnknown*)evParam1) 
				|| !pAudCap && pAudCap == (pBF = (IUnknown*)evParam1))
			&& evParam2 == 0)
			{
				SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			}
		}
		else if(EC_DVD_TITLE_CHANGE == evCode)
		{
			if(m_iPlaybackMode == PM_FILE)
			{
				SetupChapters();
			}
			else if(m_iPlaybackMode == PM_DVD)
			{
				m_iDVDTitle = (DWORD)evParam1;

				if(m_iDVDDomain == DVD_DOMAIN_Title)
				{
					CString Domain;
					Domain.Format(_T("Title %d"), m_iDVDTitle);
					m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), Domain);
				}
			}
		}
		else if(EC_DVD_DOMAIN_CHANGE == evCode)
		{
			m_iDVDDomain = (DVD_DOMAIN)evParam1;

			CString Domain('-');

			switch(m_iDVDDomain)
			{
			case DVD_DOMAIN_FirstPlay: Domain = _T("First Play"); break;
			case DVD_DOMAIN_VideoManagerMenu: Domain = _T("Video Manager Menu"); break;
			case DVD_DOMAIN_VideoTitleSetMenu: Domain = _T("Video Title Set Menu"); break;
			case DVD_DOMAIN_Title: Domain.Format(_T("Title %d"), m_iDVDTitle); break;
			case DVD_DOMAIN_Stop: Domain = _T("Stop"); break;
			default: Domain = _T("-"); break;
			}

			m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), Domain);

			MoveVideoWindow(); // AR might have changed
		}
		else if(EC_DVD_CURRENT_HMSF_TIME == evCode)
		{
			double fps = evParam2 == DVD_TC_FLAG_25fps ? 25.0
				: evParam2 == DVD_TC_FLAG_30fps ? 30.0
				: evParam2 == DVD_TC_FLAG_DropFrame ? 29.97
				: 25.0;

			REFERENCE_TIME rtDur = 0;

			DVD_HMSF_TIMECODE tcDur;
			ULONG ulFlags;
			if(SUCCEEDED(pDVDI->GetTotalTitleTime(&tcDur, &ulFlags)))
				rtDur = HMSF2RT(tcDur, fps);

			m_wndSeekBar.Enable(rtDur > 0);
			m_wndSeekBar.SetRange(0, rtDur);

			REFERENCE_TIME rtNow = HMSF2RT(*((DVD_HMSF_TIMECODE*)&evParam1), fps);

			m_wndSeekBar.SetPos(rtNow);

			if(m_pSubClock) m_pSubClock->SetTime(rtNow);
		}
		else if(EC_DVD_ERROR == evCode)
		{
			TRACE(_T("EC_DVD_ERROR %d %d\n"), evParam1, evParam2);

			CString err;

			switch(evParam1)
			{
			case DVD_ERROR_Unexpected: default: err = _T("DVD: Unexpected error"); break;
			case DVD_ERROR_CopyProtectFail: err = _T("DVD: Copy-Protect Fail"); break;
			case DVD_ERROR_InvalidDVD1_0Disc: err = _T("DVD: Invalid DVD 1.x Disc"); break;
			case DVD_ERROR_InvalidDiscRegion: err = _T("DVD: Invalid Disc Region"); break;
			case DVD_ERROR_LowParentalLevel: err = _T("DVD: Low Parental Level"); break;
			case DVD_ERROR_MacrovisionFail: err = _T("DVD: Macrovision Fail"); break;
			case DVD_ERROR_IncompatibleSystemAndDecoderRegions: err = _T("DVD: Incompatible System And Decoder Regions"); break;
			case DVD_ERROR_IncompatibleDiscAndDecoderRegions: err = _T("DVD: Incompatible Disc And Decoder Regions"); break;
			}

			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

			m_closingmsg = err;
		}
		else if(EC_DVD_WARNING == evCode)
		{
			TRACE(_T("EC_DVD_WARNING %d %d\n"), evParam1, evParam2);
		}
		else if(EC_VIDEO_SIZE_CHANGED == evCode)
		{
			TRACE(_T("EC_VIDEO_SIZE_CHANGED %dx%d\n"), CSize(evParam1));

			WINDOWPLACEMENT wp;
			wp.length = sizeof(wp);
			GetWindowPlacement(&wp);

			CSize size(evParam1);
			m_fAudioOnly = (size.cx <= 0 || size.cy <= 0);

			if(AfxGetAppSettings().fRememberZoomLevel
			&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED))
			{
				ZoomVideoWindow();
			}
			else
			{
				MoveVideoWindow();
			}

			if(m_iMediaLoadState == MLS_LOADED
			&& !m_fAudioOnly && (AfxGetAppSettings().nCLSwitches&CLSW_FULLSCREEN))
			{
				PostMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
				AfxGetAppSettings().nCLSwitches &= ~CLSW_FULLSCREEN;
			}
		}
		else if(EC_LENGTH_CHANGED == evCode)
		{
			__int64 rtDur = 0;
			pMS->GetDuration(&rtDur);
			m_wndPlaylistBar.SetCurTime(rtDur);
		}
		else if(!m_fCustomGraph)
		{
			TRACE(_T("evCode: %d\n"), evCode);
		}
		else if(EC_BG_AUDIO_CHANGED == evCode)
		{
			int nAudioChannels = evParam1;

			m_wndStatusBar.SetStatusBitmap(nAudioChannels == 1 ? IDB_MONO 
										: nAudioChannels >= 2 ? IDB_STEREO 
										: IDB_NOAUDIO);
		}
		else if(EC_BG_ERROR == evCode)
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			m_closingmsg = !str.IsEmpty() ? str : _T("Unspecified graph error");
			m_wndPlaylistBar.SetCurValid(false);
			break;
		}
	}

    return hr;
}

LRESULT CMainFrame::OnRepaintRenderLess(WPARAM wParam, LPARAM lParam)
{
	MoveVideoWindow();
	return TRUE;
}

LRESULT CMainFrame::OnResumeFromState(WPARAM wParam, LPARAM lParam)
{
	int iPlaybackMode = (int)wParam;

	if(iPlaybackMode == PM_FILE)
	{
		SeekTo(10000i64*int(lParam));
	}
	else if(iPlaybackMode == PM_DVD)
	{
		CComPtr<IDvdState> pDvdState;
		pDvdState.Attach((IDvdState*)lParam);
		if(pDVDC) pDVDC->SetState(pDvdState, DVD_CMD_FLAG_Block, NULL);
	}
	else if(iPlaybackMode == PM_CAPTURE)
	{
		// not implemented
	}
	else
	{
		ASSERT(0);
		return FALSE;
	}

	return TRUE;
}

BOOL CMainFrame::OnButton(UINT id, UINT nFlags, CPoint point)
{
	SetFocus();

	CRect r;
	m_wndView.GetClientRect(r);
	m_wndView.MapWindowPoints(this, &r);

	if(id != wmcmd::WDOWN && id != wmcmd::WUP && !r.PtInRect(point)) return FALSE;

	BOOL ret = FALSE;

	AppSettings& s = AfxGetAppSettings();
	POSITION pos = s.wmcmds.GetHeadPosition();
	while(pos)
	{
		wmcmd& wc = s.wmcmds.GetNext(pos);
		if(wc.mouse == id)
		{
			SendMessage(WM_COMMAND, wc.cmd);
			ret = true;
		}
	}

	return ret;
}
void CMainFrame::PreFocused(){
	//bRecentFocused = TRUE;
	//SetTimer(TIMER_RECENTFOCUSED, 100, NULL);
}
void CMainFrame::OnLButtonDown(UINT nFlags, CPoint point)
{
	if( m_iMediaLoadState == MLS_CLOSED )
		m_wndView.OnLButtonDown(nFlags ,point );

	SetFocus();
	

	m_pLastClickPoint = point;
	CRect rVideo = m_wndView.GetVideoRect();
	CPoint p = point - rVideo.TopLeft();

// 	if(p.y < 20){
// 		OnMenu(m_mainMenu.GetSubMenu(0));
// 		return;
// 	}

	int xPercent = 0, yPercent = 0;
	if(rVideo.Width()){
		xPercent = p.x * 100 / rVideo.Width();
	}
	if(rVideo.Height()){
		yPercent = p.y * 100 / rVideo.Height();
	}


	bool fClicked = false;

	if(m_iPlaybackMode == PM_DVD)
	{
		
		if(SUCCEEDED(pDVDC->ActivateAtPosition(p))
		|| m_iDVDDomain == DVD_DOMAIN_VideoManagerMenu 
		|| m_iDVDDomain == DVD_DOMAIN_VideoTitleSetMenu)
			fClicked = true;
	}

	if(!fClicked)
	{
		bool fLeftMouseBtnUnassigned = true;
		AppSettings& s = AfxGetAppSettings();
		POSITION pos = s.wmcmds.GetHeadPosition();
		while(pos && fLeftMouseBtnUnassigned)
			if(s.wmcmds.GetNext(pos).mouse == wmcmd::LDOWN)
				fLeftMouseBtnUnassigned = false;

		//if(!bRecentFocused || s.iOnTop != 0);  //&& 
		s_fLDown = true;
		
		if(!m_fFullScreen && ( ((xPercent < 25 && yPercent < 25) ) || !s.useSmartDrag )) //IsCaptionMenuHidden() || 
		{
			s_mDragFuc = 3; //PostMessage(WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(point.x, point.y));
		}
		else if( s.useSmartDrag )
		{
				if(xPercent > 40  && xPercent < 60 && yPercent > 40  && yPercent < 60 ){ //画面中心
					//移动画面
					s_mDragFuc = 1;
				}else if(xPercent > 65 && yPercent < 65){ // 画面右上角
					//缩放画面
					s_mDragFuc = 2;
				}
			
			
			//if(OnButton(wmcmd::LDOWN, nFlags, point))
			//SetTimer(TIMER_MOUSELWOWN, 300, NULL);
			return;
		}
	}

	__super::OnLButtonDown(nFlags, point);
}

void CMainFrame::OnLButtonUp(UINT nFlags, CPoint point)
{
	if( m_iMediaLoadState == MLS_CLOSED )
		m_wndView.OnLButtonUp(nFlags ,point );

	int iDistance = sqrt( pow( (double)abs(point.x - m_pLastClickPoint.x) , 2)  + pow( (double)abs( point.y - m_pLastClickPoint.y ) , 2) );
	//!bRecentFocused &&
	if(  s_fLDown && iDistance < 30){
		/*
		if( m_iMediaLoadState == MLS_CLOSED   ){
					OnFileOpenQuick();
					//OnFileOpenmedia();
					__super::OnLButtonUp(nFlags, point);
					return;
				}*/
		
		bool fDBLMouseClickUnassigned = true;
		AppSettings& s = AfxGetAppSettings();
		POSITION pos = s.wmcmds.GetHeadPosition();
		while(pos && fDBLMouseClickUnassigned)
			if(s.wmcmds.GetNext(pos).mouse == wmcmd::LDBLCLK)
				fDBLMouseClickUnassigned = false;
		if(fDBLMouseClickUnassigned){
			SetTimer(TIMER_MOUSELWOWN, 1, NULL);
		}else{
			SetTimer(TIMER_MOUSELWOWN, SINGLECLICK_INTERLEAVE_MS, NULL);
		}
	}
	s_mDragFuc = 0;
	s_mDragFucOn = false;

	if(!OnButton(wmcmd::LUP, nFlags, point))
		__super::OnLButtonUp(nFlags, point);
}
LRESULT CMainFrame::OnMouseMoveIn(WPARAM /*wparam*/, LPARAM /*lparam*/) {
	TRACE("OnMouseMoveIn\n");

	return 0;
}

LRESULT CMainFrame::OnMouseMoveOut(WPARAM /*wparam*/, LPARAM /*lparam*/) {
	TRACE("OnMouseMoveOut\n");
	m_btnList.ClearStat();

	AppSettings&s = AfxGetAppSettings();
	if (m_fFullScreen || !(s.nCS & CS_TOOLBAR) ) {
		ShowControls(CS_NONE, false);
	}

	if(m_fFullScreen || IsCaptionMenuHidden() )  
	{

		HMENU hMenu = NULL;
		
		::SetMenu(m_hWnd, hMenu);

	}
	RedrawNonClientArea();
	return 0;
}


void CMainFrame::OnShowColorControlBar()
{
	AppSettings &s = AfxGetAppSettings();
	s.bShowControlBar = !s.bShowControlBar;
	ShowControlBar(&m_wndColorControlBar, (s.bShowControlBar ? SW_SHOW : SW_HIDE) , false);
	if(s.bShowControlBar){
		bNotHideColorControlBar = TRUE;
		SetTimer(TIMER_STATUSBARHIDER, 3000 , NULL);
	}else{
		bNotHideColorControlBar = false;
	}
}

void CMainFrame::OnUpdateShowColorControlBar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!!AfxGetAppSettings().bShowControlBar);
}

void CMainFrame::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if(!IsSomethingLoaded()){
		SendMessage(WM_COMMAND, ID_FILE_OPENQUICK);
	}
	if(s_fLDown)
	{
		SendMessage(WM_LBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
		s_fLDown = false;
	}
	s_mDragFuc = 0;
	s_mDragFucOn = false;

	if(!OnButton(wmcmd::LDBLCLK, nFlags, point))
		__super::OnLButtonDblClk(nFlags, point);
}

void CMainFrame::OnMButtonDown(UINT nFlags, CPoint point)
{
	SendMessage(WM_CANCELMODE);
	if(!OnButton(wmcmd::MDOWN, nFlags, point))
		__super::OnMButtonDown(nFlags, point);
}

void CMainFrame::OnMButtonUp(UINT nFlags, CPoint point)
{
	if(!OnButton(wmcmd::MUP, nFlags, point))
		__super::OnMButtonUp(nFlags, point);
}

void CMainFrame::OnMButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessage(WM_MBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	if(!OnButton(wmcmd::MDBLCLK, nFlags, point))
		__super::OnMButtonDblClk(nFlags, point);
}

void CMainFrame::OnRButtonDown(UINT nFlags, CPoint point)
{
	if(!OnButton(wmcmd::RDOWN, nFlags, point))
		__super::OnRButtonDown(nFlags, point);
}

void CMainFrame::OnRButtonUp(UINT nFlags, CPoint point)
{
	if(!OnButton(wmcmd::RUP, nFlags, point))
		__super::OnRButtonUp(nFlags, point);
}

void CMainFrame::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	SendMessage(WM_RBUTTONDOWN, nFlags, MAKELPARAM(point.x, point.y));
	if(!OnButton(wmcmd::RDBLCLK, nFlags, point))
		__super::OnRButtonDblClk(nFlags, point);
}

LRESULT CMainFrame::OnXButtonDown(WPARAM wParam, LPARAM lParam)
{
	SendMessage(WM_CANCELMODE);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DOWN : fwButton == XBUTTON2 ? wmcmd::X2DOWN : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonUp(WPARAM wParam, LPARAM lParam)
{
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1UP : fwButton == XBUTTON2 ? wmcmd::X2UP : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

LRESULT CMainFrame::OnXButtonDblClk(WPARAM wParam, LPARAM lParam)
{
	SendMessage(WM_XBUTTONDOWN, wParam, lParam);
	UINT fwButton = GET_XBUTTON_WPARAM(wParam); 
	return OnButton(fwButton == XBUTTON1 ? wmcmd::X1DBLCLK : fwButton == XBUTTON2 ? wmcmd::X2DBLCLK : wmcmd::NONE,
		GET_KEYSTATE_WPARAM(wParam), CPoint(lParam));
}

BOOL CMainFrame::OnMouseWheel(UINT nFlags, short zDelta, CPoint point)
{
	ScreenToClient(&point);

	BOOL fRet = 
		zDelta > 0 ? OnButton(wmcmd::WUP, nFlags, point) :
		zDelta < 0 ? OnButton(wmcmd::WDOWN, nFlags, point) : 
		FALSE;

	return fRet;
}


LRESULT CMainFrame::OnNcHitTest(CPoint point)
{
	LRESULT nHitTest = __super::OnNcHitTest(point);
	return ((IsCaptionMenuHidden()) && nHitTest == HTCLIENT) ? HTCAPTION : nHitTest;
}

void CMainFrame::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if(pScrollBar->IsKindOf(RUNTIME_CLASS(CVolumeCtrl)))
	{
		OnPlayVolume(0);
	}
	else if(pScrollBar->IsKindOf(RUNTIME_CLASS(CPlayerSeekBar)) && m_iMediaLoadState == MLS_LOADED)
	{
		SeekTo(m_wndSeekBar.GetPos(), !!(::GetKeyState(VK_SHIFT)&0x8000));
	}

	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CMainFrame::OnInitMenu(CMenu* pMenu)
{
	__super::OnInitMenu(pMenu);

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for(UINT i = 0, j = pMenu->GetMenuItemCount(); i < j; i++)
	{
		CString str;
		pMenu->GetMenuString(i, str, MF_BYPOSITION);

		CMenu* pSubMenu = NULL;

		if(str == ResStr(IDS_FAVORITES_POPUP))
		{
			SetupFavoritesSubMenu();
			pSubMenu = &m_favorites;
		}

		if(pSubMenu)
		{
			mii.fMask = MIIM_STATE|MIIM_SUBMENU;
			mii.fType = MF_POPUP;
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED|MF_GRAYED));
			pMenu->SetMenuItemInfo(i, &mii, TRUE);
		}
	}
}
void MenuMerge(CMenu* Org, CMenu* New){
	if(!Org || !New){ return;};
	if(Org->GetMenuItemCount() > 0 && New->GetMenuItemCount() > 0 ){
		Org->AppendMenu(MF_SEPARATOR);
	}
	for(int j = 0; j < New->GetMenuItemCount(); j++){
		MENUITEMINFO mii;
		memset(&mii, 0, sizeof(MENUITEMINFO));
		mii.cbSize = sizeof(MENUITEMINFO);
		mii.fMask = MIIM_STATE | MIIM_TYPE;
		
		New->GetMenuItemInfo(j, &mii, TRUE);
		if(mii.fType & MFT_SEPARATOR){
			Org->AppendMenu(MF_SEPARATOR);
		}else{
			CString szMStr;
			New->GetMenuString(j, szMStr, MF_BYPOSITION); 
			UINT mflag = 0;
			if(MFS_CHECKED & mii.fState){
				mflag = MF_CHECKED;
			}

			Org->AppendMenu(MF_ENABLED | MF_STRING | mflag, New->GetMenuItemID(j), szMStr);
		}
	}
}

void CMainFrame::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	__super::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	static CAtlStringMap<UINT> transl;

	if(transl.IsEmpty())
	{
		transl[_T("Navigate")] = IDS_NAVIGATE_POPUP;
		transl[_T("Open Disc")] = IDS_OPENCDROM_POPUP;
		transl[_T("Filters")] = IDS_FILTERS_POPUP;
		transl[_T("Audio")] = IDS_AUDIO_POPUP;
		transl[_T("Subtitles")] = IDS_SUBTITLES_POPUP;
		transl[_T("Audio Language")] = IDS_AUDIOLANGUAGE_POPUP;
		transl[_T("Subtitle Language")] = IDS_SUBTITLELANGUAGE_POPUP;
		transl[_T("Video Angle")] = IDS_VIDEOANGLE_POPUP;
		transl[_T("Jump To...")] = IDS_JUMPTO_POPUP;
		transl[_T("Favorites")] = IDS_FAVORITES_POPUP;
		transl[_T("Shaders")] = IDS_SHADER_POPUP;
		transl[_T("Video Frame")] = IDS_VIDEOFRAME_POPUP;
		transl[_T("PanScan")] = IDS_PANSCAN_POPUP;
		transl[_T("Aspect Ratio")] = IDS_ASPECTRATIO_POPUP;
		transl[_T("Zoom")] = IDS_ZOOM_POPUP;
		transl[_T("DVD选单")] = IDS_NAVIGATE_POPUP;
		transl[_T("打开碟片")] = IDS_OPENCDROM_POPUP;
		transl[_T("滤镜(Filters)")] = IDS_FILTERS_POPUP;
		transl[_T("音频/声道切换")] = IDS_AUDIO_POPUP;
		transl[_T("字幕")] = IDS_SUBTITLES_POPUP;
		transl[_T("音轨选择")] = IDS_AUDIOLANGUAGE_POPUP;
		transl[_T("字幕语言")] = IDS_SUBTITLELANGUAGE_POPUP;
		transl[_T("视角切换")] = IDS_VIDEOANGLE_POPUP;
		transl[_T("跳至...")] = IDS_JUMPTO_POPUP;
		transl[_T("收藏夹")] = IDS_FAVORITES_POPUP;
		transl[_T("画面增益")] = IDS_SHADER_POPUP;
		transl[_T("视频尺寸")] = IDS_VIDEOFRAME_POPUP;
		transl[_T("画面微调")] = IDS_PANSCAN_POPUP;
		transl[_T("强制画面比例")] = IDS_ASPECTRATIO_POPUP;
		transl[_T("界面缩放")] = IDS_ZOOM_POPUP;
	}

	MENUITEMINFO mii;
	mii.cbSize = sizeof(mii);

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);

		CString lookupstr = str;
		lookupstr.Remove('&');

		CMenu* pSubMenu = NULL;
		

		UINT id;
		if(transl.Lookup(lookupstr, id))
		{
			str = ResStr(id);
			// pPopupMenu->ModifyMenu(i, MF_BYPOSITION|MF_STRING, 0, str);
			MENUITEMINFO mii;
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_STRING;
			mii.dwTypeData = (LPTSTR)(LPCTSTR)str;
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
		}

		if(str == ResStr(IDS_NAVIGATE_POPUP) )
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED 
				&& (1/*m_iPlaybackMode == PM_DVD *//*|| (m_iPlaybackMode == PM_FILE && m_PlayList.GetCount() > 0)*/)) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == ResStr(IDS_VIDEOFRAME_POPUP)
			|| str == ResStr(IDS_PANSCAN_POPUP)
			|| str == ResStr(IDS_ASPECTRATIO_POPUP)
			|| str == ResStr(IDS_ZOOM_POPUP))
		{
			UINT fState = (m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly) 
				? MF_ENABLED 
				: (MF_DISABLED|MF_GRAYED);

			pPopupMenu->EnableMenuItem(i, MF_BYPOSITION|fState);
		}
		else if(str == ResStr(IDS_OPENCDROM_POPUP))
		{
			SetupOpenCDSubMenu();
			pSubMenu = &m_opencds;
		}
		else if(str == ResStr(IDS_FILTERS_POPUP))
		{
			SetupFiltersSubMenu();
			pSubMenu = &m_filters;
		}
		else if(str == ResStr(IDS_AUDIO_POPUP)  )
		{
			SetupAudioSwitcherSubMenu();
			SetupNavAudioSubMenu();
			MenuMerge( &m_audios ,  &m_navaudio );
			pSubMenu = &m_audios;
		}
		else if(str == ResStr(IDS_SUBTITLES_POPUP))
		{
			SetupSubtitlesSubMenu();
			SetupNavSubtitleSubMenu();
			MenuMerge( &m_subtitles ,  &m_navsubtitle );
			pSubMenu = &m_subtitles;
		}
		else if(str == ResStr(IDS_SUBTITLES_POPUP2))
		{
			SetupSubtitlesSubMenu(2);
			pSubMenu = &m_subtitles2;
		}
		else if(str == ResStr(IDS_AUDIOLANGUAGE_POPUP) )
		{
			SetupNavAudioSubMenu();
			pSubMenu = &m_navaudio;
		}
		else if(str == ResStr(IDS_SUBTITLELANGUAGE_POPUP))
		{
			SetupNavSubtitleSubMenu();
			pSubMenu = &m_navsubtitle;
			
		}
		else if(str == ResStr(IDS_VIDEOANGLE_POPUP))
		{
			SetupNavAngleSubMenu();
			pSubMenu = &m_navangle;
		}
		else if(str == ResStr(IDS_JUMPTO_POPUP))
		{
			SetupNavChaptersSubMenu();
			pSubMenu = &m_navchapters;
		}
		else if(str == ResStr(IDS_FAVORITES_POPUP))
		{
			SetupFavoritesSubMenu();
			pSubMenu = &m_favorites;
		}
		else if(str == _T("最近播放"))
		{
			SetupRecentFileSubMenu();
			pSubMenu = &m_recentfiles;
		}
		else if(str == ResStr(IDS_SHADER_POPUP) )
		{
			SetupShadersSubMenu();
			pSubMenu = &m_shaders;
		}

		if(pSubMenu)
		{
			mii.fMask = MIIM_STATE|MIIM_SUBMENU;
			mii.fType = MF_POPUP;
			mii.hSubMenu = pSubMenu->m_hMenu;
			mii.fState = (pSubMenu->GetMenuItemCount() > 0 ? MF_ENABLED : (MF_DISABLED|MF_GRAYED));
			pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);
		}
	}

	//

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		UINT nID = pPopupMenu->GetMenuItemID(i);
		if(nID == ID_SEPARATOR || nID == -1
		|| nID >= ID_FAVORITES_FILE_START && nID <= ID_FAVORITES_FILE_END
		|| nID >= ID_NAVIGATE_CHAP_SUBITEM_START && nID <= ID_NAVIGATE_CHAP_SUBITEM_END)
			continue;

		CString str;
		pPopupMenu->GetMenuString(i, str, MF_BYPOSITION);
		int k = str.Find('\t');
		if(k > 0) str = str.Left(k);

		CString key = CPPageAccelTbl::MakeAccelShortcutLabel(nID);
		if(!key.IsEmpty()) str += _T("\t") + key;

		if(key.IsEmpty() && i < 0) continue;

		// BUG(?): this disables menu item update ui calls for some reason...
//		pPopupMenu->ModifyMenu(i, MF_BYPOSITION|MF_STRING, nID, str);

		// this works fine
		MENUITEMINFO mii;
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = (LPTSTR)(LPCTSTR)str;
		pPopupMenu->SetMenuItemInfo(i, &mii, TRUE);

	}

	//

	bool fPnSPresets = false;

	for(UINT i = 0, j = pPopupMenu->GetMenuItemCount(); i < j; i++)
	{
		UINT nID = pPopupMenu->GetMenuItemID(i);

		if(nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END)
		{
			do
			{
				nID = pPopupMenu->GetMenuItemID(i);
				pPopupMenu->DeleteMenu(i, MF_BYPOSITION);
				j--;
			}
			while(i < j && nID >= ID_PANNSCAN_PRESETS_START && nID < ID_PANNSCAN_PRESETS_END);

			nID = pPopupMenu->GetMenuItemID(i);
		}

		if(nID == ID_VIEW_RESET)
		{
			fPnSPresets = true;
		}
	}

	if(fPnSPresets)
	{
		AppSettings& s = AfxGetAppSettings();
		int i = 0, j = s.m_pnspresets.GetCount();
		for(; i < j; i++)
		{
			int k = 0;
			CString label = s.m_pnspresets[i].Tokenize(_T(","), k);
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, label);
		}
//		if(j > 0)
		{
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND, ID_PANNSCAN_PRESETS_START+i, ResStr(IDS_PANSCAN_EDIT));
			pPopupMenu->InsertMenu(ID_VIEW_RESET, MF_BYCOMMAND|MF_SEPARATOR);
		}
	}
}

BOOL CMainFrame::OnMenu(CMenu* pMenu)
{
	if(!pMenu) return FALSE;

	KillTimer(TIMER_FULLSCREENMOUSEHIDER);
	m_fHideCursor = false;

	CPoint point;
	GetCursorPos(&point);

	pMenu->TrackPopupMenu(TPM_RIGHTBUTTON|TPM_NOANIMATION, point.x+1, point.y+1, this);

	return TRUE;
}

void CMainFrame::OnMenuPlayerShort()
{
	/*
	if(IsCaptionMenuHidden())
		{
			OnMenu(m_popupmain.GetSubMenu(0));
		}
		else
		{*/
	
		OnMenu(m_popup.GetSubMenu(0));
	//}
}

void CMainFrame::OnMenuPlayerLong()
{
	OnMenu(m_popupmain.GetSubMenu(0));
}

void CMainFrame::OnMenuFilters()
{
	SetupFiltersSubMenu();
	OnMenu(&m_filters);
}

void CMainFrame::OnUpdatePlayerStatus(CCmdUI* pCmdUI)
{
	if(m_iMediaLoadState == MLS_LOADING)
	{
		pCmdUI->SetText(ResStr(IDS_CONTROLS_OPENING));
	}
	else if(m_iMediaLoadState == MLS_LOADED)
	{
		CString msg;

		if(!m_playingmsg.IsEmpty())
		{
			msg = m_playingmsg;
		}
		else if(m_fCapturing)
		{
			msg = ResStr(IDS_CONTROLS_CAPTURING);

			if(pAMDF)
			{
				long lDropped = 0;
				pAMDF->GetNumDropped(&lDropped);
				long lNotDropped = 0;
				pAMDF->GetNumNotDropped(&lNotDropped);

				if((lDropped + lNotDropped) > 0)
				{
					CString str;
					str.Format(_T(", Total: %d, Dropped: %d"), lDropped + lNotDropped, lDropped);
					msg += str;
				}
			}

			CComPtr<IPin> pPin;
			if(SUCCEEDED(pCGB->FindPin(m_wndCaptureBar.m_capdlg.m_pDst, PINDIR_INPUT, NULL, NULL, FALSE, 0, &pPin)))
			{
				LONGLONG size = 0;
				if(CComQIPtr<IStream> pStream = pPin)
				{
					pStream->Commit(STGC_DEFAULT);

					WIN32_FIND_DATA findFileData;
					HANDLE h = FindFirstFile(m_wndCaptureBar.m_capdlg.m_file, &findFileData);
					if(h != INVALID_HANDLE_VALUE)
					{
						size = ((LONGLONG)findFileData.nFileSizeHigh << 32) | findFileData.nFileSizeLow;

						CString str;
						if(size < 1024i64*1024)
							str.Format(_T(", Size: %I64dKB"), size/1024);
						else //if(size < 1024i64*1024*1024)
							str.Format(_T(", Size: %I64dMB"), size/1024/1024);
						msg += str;
						
						FindClose(h);
					}
				}

				ULARGE_INTEGER FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
				if(GetDiskFreeSpaceEx(
					m_wndCaptureBar.m_capdlg.m_file.Left(m_wndCaptureBar.m_capdlg.m_file.ReverseFind('\\')+1), 
					&FreeBytesAvailable, &TotalNumberOfBytes, &TotalNumberOfFreeBytes))
				{
					CString str;
					if(FreeBytesAvailable.QuadPart < 1024i64*1024)
						str.Format(_T(", Free: %I64dKB"), FreeBytesAvailable.QuadPart/1024);
					else //if(FreeBytesAvailable.QuadPart < 1024i64*1024*1024)
						str.Format(_T(", Free: %I64dMB"), FreeBytesAvailable.QuadPart/1024/1024);
					msg += str;
				}

				if(m_wndCaptureBar.m_capdlg.m_pMux)
				{
					__int64 pos = 0;
					CComQIPtr<IMediaSeeking> pMuxMS = m_wndCaptureBar.m_capdlg.m_pMux;
					if(pMuxMS && SUCCEEDED(pMuxMS->GetCurrentPosition(&pos)) && pos > 0)
					{
						double bytepersec = 10000000.0 * size / pos;
						if(bytepersec > 0)
							m_rtDurationOverride = __int64(10000000.0 * (FreeBytesAvailable.QuadPart+size) / bytepersec);
					}
				}

				if(m_wndCaptureBar.m_capdlg.m_pVidBuffer
				|| m_wndCaptureBar.m_capdlg.m_pAudBuffer)
				{
					int nFreeVidBuffers = 0, nFreeAudBuffers = 0;
					if(CComQIPtr<IBufferFilter> pVB = m_wndCaptureBar.m_capdlg.m_pVidBuffer)
						nFreeVidBuffers = pVB->GetFreeBuffers();
					if(CComQIPtr<IBufferFilter> pAB = m_wndCaptureBar.m_capdlg.m_pAudBuffer)
						nFreeAudBuffers = pAB->GetFreeBuffers();

					CString str;
					str.Format(_T(", Free V/A Buffers: %03d/%03d"), nFreeVidBuffers, nFreeAudBuffers);
					msg += str;
				}
			}
		}
		else if(m_fBuffering)
		{
			BeginEnumFilters(pGB, pEF, pBF)
			{
				if(CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF)
				{
					long BufferingProgress = 0;
					if(SUCCEEDED(pAMNS->get_BufferingProgress(&BufferingProgress)) && BufferingProgress > 0)
					{
						msg.Format(ResStr(IDS_CONTROLS_BUFFERING), BufferingProgress);

						__int64 start = 0, stop = 0;
						m_wndSeekBar.GetRange(start, stop);
						m_fLiveWM = (stop == start);
					}
					break;
				}
			}
			EndEnumFilters
		}
		else if(pAMOP)
		{
			__int64 t = 0, c = 0;
			if(SUCCEEDED(pAMOP->QueryProgress(&t, &c)) && t > 0 && c < t)
				msg.Format(ResStr(IDS_CONTROLS_BUFFERING), c*100/t);

			if(m_fUpdateInfoBar)
				OpenSetupInfoBar();
		}

		OAFilterState fs = GetMediaState();
		pCmdUI->SetText(
			!msg.IsEmpty() ? msg : 
			fs == State_Stopped ? ResStr(IDS_CONTROLS_STOPPED) :
			(fs == State_Paused || m_fFrameSteppingActive) ? ResStr(IDS_CONTROLS_PAUSED) :
			fs == State_Running ? ResStr(IDS_CONTROLS_PLAYING) :
			_T(""));
	}
	else if(m_iMediaLoadState == MLS_CLOSING)
	{
		pCmdUI->SetText(ResStr(IDS_CONTROLS_CLOSING));
	}
	else
	{
		pCmdUI->SetText(m_closingmsg);
	}
}

void CMainFrame::OnFilePostOpenmedia()
{
	OpenSetupInfoBar();

	OpenSetupStatsBar();

	OpenSetupStatusBar();

	// OpenSetupToolBar();

	OpenSetupCaptureBar();

	AppSettings& s = AfxGetAppSettings();
	m_wndColorControlBar.CheckAbility();

	if(m_wndToolBar.IsVisible())
		ShowControls(AfxGetAppSettings().nCS | CS_SEEKBAR, false);

	__int64 rtDur = 0;
	pMS->GetDuration(&rtDur);
	m_wndPlaylistBar.SetCurTime(rtDur);

	if(m_iPlaybackMode == PM_CAPTURE)
	{
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE);
//		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
//		ShowControlBar(&m_wndCaptureBar, TRUE, TRUE);
	}

	//if(m_pCAP) m_pCAP->SetSubtitleDelay(0); remove since we need set the delay while setSubTitle
	m_iMediaLoadState = MLS_LOADED;

	// IMPORTANT: must not call any windowing msgs before
	// this point, it will deadlock when OpenMediaPrivate is
	// still running and the renderer window was created on
	// the same worker-thread

	{
		WINDOWPLACEMENT wp;
		wp.length = sizeof(wp);
		GetWindowPlacement(&wp);

		// restore magnification
		if(IsWindowVisible() && AfxGetAppSettings().fRememberZoomLevel
		&& !(m_fFullScreen || wp.showCmd == SW_SHOWMAXIMIZED || wp.showCmd == SW_SHOWMINIMIZED))
		{
			ZoomVideoWindow();
		}
	}

	if(!m_fAudioOnly && (s.nCLSwitches&CLSW_FULLSCREEN))
	{
		SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
		s.nCLSwitches &= ~CLSW_FULLSCREEN;
	}

	m_bDxvaInUse = false;

	CComQIPtr<IMPCVideoDecFilter> pMDF  = FindFilter(__uuidof(CMPCVideoDecFilter), pGB);
	if(pMDF){
		GUID*	DxvaGui = NULL;
		DxvaGui = pMDF->GetDXVADecoderGuid();
		if (DxvaGui != NULL)
		{
			CString DXVAMode = GetDXVAMode (DxvaGui);
			m_bDxvaInUse = (DXVAMode != _T("Not using DXVA"));
			
		}
		
	}else if(FindFilter(L"{09571A4B-F1FE-4C60-9760-DE6D310C7C31}", pGB)) {
		if(s.useGPUCUDA){
			m_bDxvaInUse = true;
		}

	}
	
	if(FindFilter(L"{FA10746C-9B63-4B6C-BC49-FC300EA5F256}", pGB)){
		m_bEVRInUse = true;
	}
	RedrawNonClientArea();
	SendNowPlayingToMSN();
	SendNowPlayingTomIRC();
}

void CMainFrame::OnUpdateFilePostOpenmedia(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADING);
}

void CMainFrame::OnFilePostClosemedia()
{
	AppSettings& s = AfxGetAppSettings();
	m_bDxvaInUse = false;
	m_bEVRInUse = false;
	m_wndView.SetVideoRect();
	m_wndSeekBar.Enable(false);
	m_wndSeekBar.SetPos(0);
	m_wndInfoBar.RemoveAllLines();
	m_wndStatsBar.RemoveAllLines();		
	if(IsWindow(m_wndStatusBar.m_hWnd)){
		m_wndStatusBar.Clear();
		m_wndStatusBar.ShowTimer(false);
	}

	if(IsWindow(m_wndSubresyncBar.m_hWnd))
	{
		ShowControlBar(&m_wndSubresyncBar, FALSE, TRUE); 
		SetSubtitle(NULL);
	} 
//	AfxGetAppSettings().fEnableSubtitles2 = FALSE;
	m_iSubtitleSel2 = -1;
	
	if(m_wndToolBar.IsVisible())
	   ShowControls(s.nCS & ~CS_SEEKBAR , false);

	if(IsWindow(m_wndCaptureBar.m_hWnd))
	{
		ShowControlBar(&m_wndCaptureBar, FALSE, TRUE);
		m_wndCaptureBar.m_capdlg.SetupVideoControls(_T(""), NULL, NULL, NULL);
		m_wndCaptureBar.m_capdlg.SetupAudioControls(_T(""), NULL, CInterfaceArray<IAMAudioInputMixer>());
	}

	RecalcLayout();
	CString szBuild;
	if(!s.szOEMTitle.IsEmpty()){
		szBuild.Format(_T(" (%s Build %s)"), s.szOEMTitle , SVP_REV_STR);
	}else{
		szBuild.Format( _T(" (Build %s)"),SVP_REV_STR);
	}
	
	//SetWindowText(CString(ResStr(IDR_MAINFRAME)) + szBuild);
	m_szTitle = CString(ResStr(IDR_MAINFRAME)) + szBuild;
	RedrawNonClientArea();

	SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

	// this will prevent any further UI updates on the dynamically added menu items
	SetupFiltersSubMenu();
	SetupAudioSwitcherSubMenu();
	SetupSubtitlesSubMenu();
	SetupSubtitlesSubMenu(2);
	SetupNavAudioSubMenu();
	SetupNavSubtitleSubMenu();
	SetupNavAngleSubMenu();
	SetupRecentFileSubMenu();
	SetupNavChaptersSubMenu();
	SetupFavoritesSubMenu();

	SendNowPlayingToMSN();

	
}

void CMainFrame::OnUpdateFilePostClosemedia(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_hWnd && m_iMediaLoadState == MLS_CLOSING);
}

LRESULT CMainFrame::OnHotKey(WPARAM wParam, LPARAM lParam){
	SendMessage(WM_COMMAND, wParam);
	return S_OK;
}

void CMainFrame::OnBossKey()
{
	SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	if(m_fFullScreen) SendMessage(WM_COMMAND, ID_VIEW_FULLSCREEN);
	SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, -1);
}

void CMainFrame::OnStreamAudio(UINT nID)
{
	nID -= ID_STREAM_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB); // morgan's switcher

	DWORD cStreams = 0;
	if(pSS && SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;
			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
			
			if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
			{
				pSS->Enable((i+(nID==0?1:cStreams-1))%cStreams, AMSTREAMSELECTENABLE_ENABLE);
				break;
			}
		}
	}
	else if(m_iPlaybackMode == PM_FILE) SendMessage(WM_COMMAND, ID_OGM_AUDIO_NEXT+nID);
	else if(m_iPlaybackMode == PM_DVD) SendMessage(WM_COMMAND, ID_DVD_AUDIO_NEXT+nID);
}

void CMainFrame::OnStreamSub(UINT nID) //need work on 2nd sub
{
	nID -= ID_STREAM_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	int cnt = 0;
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos) cnt += m_pSubStreams.GetNext(pos)->GetStreamCount();

	if(cnt > 1)
	{
		int i = ((m_iSubtitleSel&0x7fffffff)+(nID==0?1:cnt-1))%cnt;
		m_iSubtitleSel = i | (m_iSubtitleSel&0x80000000);
		UpdateSubtitle();
		SetFocus();
	}
	else if(m_iPlaybackMode == PM_FILE) SendMessage(WM_COMMAND, ID_OGM_SUB_NEXT+nID);
	else if(m_iPlaybackMode == PM_DVD) SendMessage(WM_COMMAND, ID_DVD_SUB_NEXT+nID);
}

void CMainFrame::OnStreamSubOnOff()
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	int cnt = 0;
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos) cnt += m_pSubStreams.GetNext(pos)->GetStreamCount();

	if(cnt > 0)
	{
		m_iSubtitleSel ^= 0x80000000;
		UpdateSubtitle();
		SetFocus();
	}
	else if(m_iPlaybackMode == PM_DVD) SendMessage(WM_COMMAND, ID_DVD_SUB_ONOFF);
}

void CMainFrame::OnOgmAudio(UINT nID)
{
	nID -= ID_OGM_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) pSS = FindFilter(L"{55DA30FC-F16B-49fc-BAA5-AE59FC65F82D}", pGB);
	if(!pSS) return;

    CAtlArray<int> snds;
	int iSel = -1;

	DWORD cStreams = 0;
	if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;

			if(dwGroup == 1)
			{
				if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
					iSel = snds.GetCount();
				snds.Add(i);
			}

			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
		
		}

		int cnt = snds.GetCount();
		if(cnt > 1 && iSel >= 0)
			pSS->Enable(snds[(iSel+(nID==0?1:cnt-1))%cnt], AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnOgmSub(UINT nID)
{
	nID -= ID_OGM_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) pSS = FindFilter(L"{55DA30FC-F16B-49fc-BAA5-AE59FC65F82D}", pGB);
	if(!pSS) return;

    CAtlArray<int> subs;
	int iSel = -1;

	DWORD cStreams = 0;
	if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 1)
	{
		for(int i = 0; i < (int)cStreams; i++)
		{
			AM_MEDIA_TYPE* pmt = NULL;
			DWORD dwFlags = 0;
			LCID lcid = 0;
			DWORD dwGroup = 0;
			WCHAR* pszName = NULL;
			if(FAILED(pSS->Info(i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
				return;

			if(dwGroup == 2)
			{
				if(dwFlags&(AMSTREAMSELECTINFO_ENABLED|AMSTREAMSELECTINFO_EXCLUSIVE))
					iSel = subs.GetCount();
				subs.Add(i);
			}

			if(pmt) DeleteMediaType(pmt);
			if(pszName) CoTaskMemFree(pszName);
		
		}

		int cnt = subs.GetCount();
		if(cnt > 1 && iSel >= 0)
			pSS->Enable(subs[(iSel+(nID==0?1:cnt-1))%cnt], AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnDvdAngle(UINT nID)
{
	nID -= ID_DVD_ANGLE_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG ulAnglesAvailable, ulCurrentAngle;
		if(SUCCEEDED(pDVDI->GetCurrentAngle(&ulAnglesAvailable, &ulCurrentAngle)) && ulAnglesAvailable > 1)
		{
			ulCurrentAngle += nID==0 ? 1 : ulAnglesAvailable-1;
			if(ulCurrentAngle > ulAnglesAvailable) ulCurrentAngle = 1;
			else if(ulCurrentAngle < 1) ulCurrentAngle = ulAnglesAvailable;
			pDVDC->SelectAngle(ulCurrentAngle, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnDvdAudio(UINT nID)
{
	nID -= ID_DVD_AUDIO_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG nStreamsAvailable, nCurrentStream;
		if(SUCCEEDED(pDVDI->GetCurrentAudio(&nStreamsAvailable, &nCurrentStream)) && nStreamsAvailable > 1)
			pDVDC->SelectAudioStream((nCurrentStream+(nID==0?1:nStreamsAvailable-1))%nStreamsAvailable, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnDvdSub(UINT nID)
{
	nID -= ID_DVD_SUB_NEXT;

	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
		&& ulStreamsAvailable > 1)
		{
			pDVDC->SelectSubpictureStream(
				(ulCurrentStream+(nID==0?1:ulStreamsAvailable-1))%ulStreamsAvailable, 
				DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnDvdSubOnOff()
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	if(pDVDI && pDVDC)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled)))
		{
			pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

//
// menu item handlers
//

// file

void CMainFrame::OnFileOpenQuick()
{
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) return;

	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	CString szLastFile;
	MRU.ReadList();
	if(MRU.GetSize() > 0){
		 szLastFile = MRU[0];
	}

	CString filter;
	CAtlArray<CString> mask;
	AfxGetAppSettings().Formats.GetFilter(filter, mask);

	COpenFileDlg fd(mask, true, NULL, szLastFile, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT|OFN_ENABLEINCLUDENOTIFY, 
		filter, this);
	if(fd.DoModal() != IDOK) return;

	CAtlList<CString> fns;

	POSITION pos = fd.GetStartPosition();
	while(pos) fns.AddTail(fd.GetNextPathName(pos));

	bool fMultipleFiles = false;

	if(fns.GetCount() > 1 
	|| fns.GetCount() == 1 
		&& (fns.GetHead()[fns.GetHead().GetLength()-1] == '\\'
		|| fns.GetHead()[fns.GetHead().GetLength()-1] == '*'))
	{
		fMultipleFiles = true;
	}

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	m_wndPlaylistBar.Open(fns, fMultipleFiles);

	if(m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating())
	{
		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem();
}
void CMainFrame::OnFileOpenUrlStream(){
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) return;
	
	COpenURLDlg dlg;
	if(dlg.DoModal() != IDOK || dlg.m_fns.GetCount() == 0) return;

	bool fMultipleFiles = false;
	
	this->m_lastUrl = dlg.m_fns.GetTail();
	
	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	m_wndPlaylistBar.Open(dlg.m_fns, fMultipleFiles, NULL, 0);

	if(m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating())
	{
		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem(-1);
}

static int __stdcall BrowseCtrlCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if(uMsg == BFFM_INITIALIZED && lpData)
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
	return 0;
}
void CMainFrame::OnFileOpenFolder(){
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) return;

	CString szFolderPath;

	TCHAR buff[MAX_PATH];

	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = buff;
	bi.lpszTitle = _T("打开视频文件夹");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;
	bi.lpfn = BrowseCtrlCallback;
	bi.lParam = (LPARAM)(LPCTSTR)szFolderPath;
	bi.iImage = 0; 

	
	LPITEMIDLIST iil;
	if(iil = SHBrowseForFolder(&bi))
	{
		if( SHGetPathFromIDList(iil, buff) )
			szFolderPath = buff;
	}
	if(szFolderPath.IsEmpty()){
		return;
	}
	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	m_wndPlaylistBar.AddFolder(szFolderPath);

	if(m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating())
	{
		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem();
}
void CMainFrame::OnFileOpenmedia()
{
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar)) return;

	COpenDlg dlg;
	if(dlg.DoModal() != IDOK || dlg.m_fns.GetCount() == 0) return;

	if(dlg.m_fAppendPlaylist)
	{
		m_wndPlaylistBar.Append(dlg.m_fns, dlg.m_fMultipleFiles);
		return;
	}


	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

	ShowWindow(SW_SHOW);
	SetForegroundWindow();

	m_wndPlaylistBar.Open(dlg.m_fns, dlg.m_fMultipleFiles, NULL, dlg.m_fOpenDirAutomatic);
	//set current f

	if(m_wndPlaylistBar.GetCount() == 1 && m_wndPlaylistBar.IsWindowVisible() && !m_wndPlaylistBar.IsFloating())
	{
		ShowControlBar(&m_wndPlaylistBar, FALSE, TRUE);
	}

	OpenCurPlaylistItem();
}

void CMainFrame::OnUpdateFileOpen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState != MLS_LOADING);
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCDS)
{
	if(m_iMediaLoadState == MLS_LOADING || !IsWindow(m_wndPlaylistBar))
		return FALSE;

	if(pCDS->dwData != 0x6ABE51 || pCDS->cbData < sizeof(DWORD))
		return FALSE;

	DWORD len = *((DWORD*)pCDS->lpData);
	TCHAR* pBuff = (TCHAR*)((DWORD*)pCDS->lpData + 1);
	TCHAR* pBuffEnd = (TCHAR*)((BYTE*)pBuff + pCDS->cbData - sizeof(DWORD));

	CAtlList<CString> cmdln;

	while(len-- > 0)
	{
		CString str;
		while(pBuff < pBuffEnd && *pBuff) str += *pBuff++;
		pBuff++;
		cmdln.AddTail(str);
	}

	AppSettings& s = AfxGetAppSettings();

	s.ParseCommandLine(cmdln);

	POSITION pos = s.slFilters.GetHeadPosition();
	while(pos)
	{
		CString fullpath = MakeFullPath(s.slFilters.GetNext(pos));

		CPath tmp(fullpath);
		tmp.RemoveFileSpec();
		tmp.AddBackslash();
		CString path = tmp;

		WIN32_FIND_DATA fd = {0};
		HANDLE hFind = FindFirstFile(fullpath, &fd);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if(fd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) continue;

				CFilterMapper2 fm2(false);
				fm2.Register(path + fd.cFileName);
				while(!fm2.m_filters.IsEmpty())
				{
					if(FilterOverride* f = fm2.m_filters.RemoveTail())
					{
						f->fTemporary = true;

						bool fFound = false;

						POSITION pos2 = s.filters.GetHeadPosition();
						while(pos2)
						{
							FilterOverride* f2 = s.filters.GetNext(pos2);
							if(f2->type == FilterOverride::EXTERNAL && !f2->path.CompareNoCase(f->path))
							{
								fFound = true;
								break;
							}
						}

						if(!fFound)
						{
							CAutoPtr<FilterOverride> p(f);
							s.filters.AddHead(p);
						}
					}
				}
			}
			while(FindNextFile(hFind, &fd));
			
			FindClose(hFind);
		}
	}

	bool fSetForegroundWindow = false;

	if((s.nCLSwitches&CLSW_DVD) && !s.slFiles.IsEmpty())
	{
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		CAutoPtr<OpenDVDData> p(new OpenDVDData());
		if(p) {p->path = s.slFiles.GetHead(); p->subs.AddTailList(&s.slSubs);}
		OpenMedia(p);
	}
	else if(s.nCLSwitches&CLSW_CD)
	{
		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fSetForegroundWindow = true;

		CAtlList<CString> sl;

		if(!s.slFiles.IsEmpty())
		{
			GetCDROMType(s.slFiles.GetHead()[0], sl);
		}
		else
		{
			CString dir;
			dir.ReleaseBufferSetLength(GetCurrentDirectory(MAX_PATH, dir.GetBuffer(MAX_PATH)));

			GetCDROMType(dir[0], sl);			

			for(TCHAR drive = 'C'; sl.IsEmpty() && drive <= 'Z'; drive++)
			{
				GetCDROMType(drive, sl);
			}
		}

		m_wndPlaylistBar.Open(sl, true);
		OpenCurPlaylistItem();
	}
	else if(!s.slFiles.IsEmpty())
	{
		bool fMulti = s.slFiles.GetCount() > 1;

		CAtlList<CString> sl;
		sl.AddTailList(&s.slFiles);
		if(!fMulti) sl.AddTailList(&s.slDubs);

		if((s.nCLSwitches&CLSW_ADD) && m_wndPlaylistBar.GetCount() > 0)
		{
			m_wndPlaylistBar.Append(sl, fMulti, &s.slSubs);

 			if(s.nCLSwitches&(CLSW_OPEN|CLSW_PLAY))
			{
				m_wndPlaylistBar.SetLast();
				OpenCurPlaylistItem();
			}
		}
		else
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			fSetForegroundWindow = true;
			
			m_wndPlaylistBar.Open(sl, fMulti, &s.slSubs);
			OpenCurPlaylistItem((s.nCLSwitches&CLSW_STARTVALID) ? s.rtStart : 0);
			

			s.nCLSwitches &= ~CLSW_STARTVALID;
			s.rtStart = 0;
		}
	}
	else
	{
		s.nCLSwitches = CLSW_NONE;
	}

	if(fSetForegroundWindow && !(s.nCLSwitches&CLSW_NOFOCUS))
		SetForegroundWindow();

	s.nCLSwitches &= ~CLSW_NOFOCUS;

	return TRUE;
}

void CMainFrame::OnFileOpendvd()
{
	if(m_iMediaLoadState == MLS_LOADING) return;

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	SetForegroundWindow();

	ShowWindow(SW_SHOW);

	CAutoPtr<OpenDVDData> p(new OpenDVDData());
	if(p)
	{
        AppSettings& s = AfxGetAppSettings();
		if(s.fUseDVDPath && !s.sDVDPath.IsEmpty())
		{
			p->path = s.sDVDPath;
			p->path.Replace('/', '\\');
			if(p->path[p->path.GetLength()-1] != '\\') p->path += '\\';
		}
	}
	OpenMedia(p);
}

void CMainFrame::OnFileOpendevice()
{
	if(m_iMediaLoadState == MLS_LOADING) return;

	COpenCapDeviceDlg capdlg;
	if(capdlg.DoModal() != IDOK)
		return;

	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	SetForegroundWindow();

	ShowWindow(SW_SHOW);

	m_wndPlaylistBar.Empty();

	CAutoPtr<OpenDeviceData> p(new OpenDeviceData());
	if(p) {p->DisplayName[0] = capdlg.m_vidstr; p->DisplayName[1] = capdlg.m_audstr;}
	OpenMedia(p);
}

void CMainFrame::OnFileOpenCD(UINT nID)
{
	nID -= ID_FILE_OPEN_CD_START;

	nID++;
	for(TCHAR drive = 'C'; drive <= 'Z'; drive++)
	{
		CAtlList<CString> sl;

		switch(GetCDROMType(drive, sl))
		{
		case CDROM_Audio:
		case CDROM_VideoCD:
		case CDROM_DVDVideo:
		case CDROM_Unknown:
			nID--;
			break;
		default:
			break;
		}

		if(nID == 0)
		{
			SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
			SetForegroundWindow();

			ShowWindow(SW_SHOW);

			m_wndPlaylistBar.Open(sl, true, NULL, 0);
			OpenCurPlaylistItem();

			break;
		}
	}
}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	SetForegroundWindow();

// 	if(m_wndPlaylistBar.IsWindowVisible())
// 	{
// 		m_wndPlaylistBar.OnDropFiles(hDropInfo);
// 		return;
// 	}

	CAtlList<CString> sl;
	BOOL bHasSubAdded = false;
	CString lastSubFile;
	UINT nFiles = ::DragQueryFile(hDropInfo, (UINT)-1, NULL, 0);
	CSVPToolBox svpTool;
	BOOL bHasForlder = false;

	for(UINT iFile = 0; iFile < nFiles; iFile++)
	{
		CString fn;
		fn.ReleaseBuffer(::DragQueryFile(hDropInfo, iFile, fn.GetBuffer(MAX_PATH), MAX_PATH));
		bHasSubAdded = isSubtitleFile(fn);
		if(bHasSubAdded){
			if(LoadSubtitle(fn)){
				lastSubFile = fn;
			}else{
				bHasSubAdded = FALSE;
			}
		}

		if(!bHasSubAdded){
			if(PathIsDirectory(fn) && svpTool.ifDirExist(fn)){
				m_wndPlaylistBar.AddFolder(fn, true);
				bHasForlder = true;
			}else{
				sl.AddTail(fn);
			}
		}
			
		
	}

	::DragFinish(hDropInfo);

	

	if(!bHasForlder){
		if(sl.IsEmpty() ){

			if(bHasSubAdded && m_pSubStreams.GetCount() > 0);
			{
				SetSubtitle(m_pSubStreams.GetTail());
				CPath p(lastSubFile);
				p.StripPath();
				SendStatusMessage(CString((LPCTSTR)p) + _T(" loaded successfully"), 3000);

			}
			return;

		}

	
		m_wndPlaylistBar.Open(sl, true);
	}else{
		if(!sl.IsEmpty()){
			m_wndPlaylistBar.Append(sl, true);
		}
	}
	OpenCurPlaylistItem();
}

void CMainFrame::OnFileSaveAs()
{
	CString ext, in = m_wndPlaylistBar.GetCur(), out = in;

	if(out.Find(_T("://")) < 0)
	{
		ext = CString(CPath(out).GetExtension()).MakeLower();
		if(ext == _T(".cda")) out = out.Left(out.GetLength()-4) + _T(".wav");
		else if(ext == _T(".ifo")) out = out.Left(out.GetLength()-4) + _T(".vob");
	}
	else
	{
		out.Empty();
	}

	CFileDialog fd(FALSE, 0, out, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
		_T("All files (*.*)|*.*||"), this, 0); 
	if(fd.DoModal() != IDOK || !in.CompareNoCase(fd.GetPathName())) return;

	CPath p(fd.GetPathName());
	if(!ext.IsEmpty()) p.AddExtension(ext);

	OAFilterState fs = State_Stopped;
	pMC->GetState(0, &fs);
	if(fs == State_Running) pMC->Pause();

	CSaveDlg dlg(in, p);
	dlg.DoModal();

	if(fs == State_Running) pMC->Run();
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_FILE)
	{
		pCmdUI->Enable(FALSE);
		return;
	}

    CString fn = m_wndPlaylistBar.GetCur();
	CString ext = fn.Mid(fn.ReverseFind('.')+1).MakeLower();

	int i = fn.Find(_T("://"));
	if(i >= 0)
	{
		CString protocol = fn.Left(i).MakeLower();
		if(protocol != _T("http"))
		{
			pCmdUI->Enable(FALSE);
			return;
		}
	}

	if((GetVersion()&0x80000000) && (ext == _T("cda") || ext == _T("ifo")))
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(TRUE);
}

bool CMainFrame::GetDIB(BYTE** ppData, long& size, bool fSilent)
{
	if(!ppData) return false;

	*ppData = NULL;
	size = 0;

	bool fNeedsToPause = !m_pCAP;
	if(fNeedsToPause) fNeedsToPause = !IsVMR7InGraph(pGB);
	if(fNeedsToPause) fNeedsToPause = !IsVMR9InGraph(pGB);

	OAFilterState fs = GetMediaState();

	if(!(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && (fs == State_Paused || fs == State_Running)))
		return false;

	if(fs == State_Running && fNeedsToPause)
	{
		pMC->Pause();
		GetMediaState(); // wait for completion of the pause command
	}

	HRESULT hr = S_OK;
	CString errmsg;

	do
	{
		if(m_pCAP)
		{
			hr = m_pCAP->GetDIB(NULL, (DWORD*)&size);
			if(FAILED(hr))
			{
				OnPlayPause();GetMediaState(); // Pause and retry to support ffdshow queueing.
				int retry = 0;
				while(FAILED(hr) && retry < 20)
				{
					hr = m_pCAP->GetDIB(*ppData, (DWORD*)&size);
					if(SUCCEEDED(hr)) break;
					Sleep(1);
					retry++;
				}
				if(FAILED(hr))
				 {errmsg.Format(_T("GetDIB failed, hr = %08x"), hr); break;}
			}

			if(!(*ppData = new BYTE[size])) return false;

			hr = m_pCAP->GetDIB(*ppData, (DWORD*)&size);
			if(FAILED(hr)) {errmsg.Format(_T("GetDIB failed, hr = %08x"), hr); break;}
		}
		else
		{
			hr = pBV->GetCurrentImage(&size, NULL);
			if(FAILED(hr) || size == 0) {errmsg.Format(_T("GetCurrentImage failed, hr = %08x"), hr); break;}

			if(!(*ppData = new BYTE[size])) return false;

			hr = pBV->GetCurrentImage(&size, (long*)*ppData);
			if(FAILED(hr)) {errmsg.Format(_T("GetCurrentImage failed, hr = %08x"), hr); break;}
		}
	}
	while(0);

	if(!fSilent)
	{
		if(!errmsg.IsEmpty())
		{
			AfxMessageBox(errmsg, MB_OK);
		}
	}

	if(fs == State_Running && GetMediaState() != State_Running)
	{
		pMC->Run();
	}

	if(FAILED(hr))
	{
		if(*ppData) {ASSERT(0); delete [] *ppData; *ppData = NULL;} // huh?
		return false;
	}

	return true;
}

#include "jpeg.h"

void CMainFrame::SaveDIB(LPCTSTR fn, BYTE* pData, long size)
{
	CString ext = CString(CPath(fn).GetExtension()).MakeLower();

	if(ext == _T(".bmp"))
	{
		if(FILE* f = _tfopen(fn, _T("wb")))
		{
			BITMAPINFO* bi = (BITMAPINFO*)pData;

			BITMAPFILEHEADER bfh;
			bfh.bfType = 'MB';
			bfh.bfOffBits = sizeof(bfh) + sizeof(bi->bmiHeader);
			bfh.bfSize = sizeof(bfh) + size;
			bfh.bfReserved1 = bfh.bfReserved2 = 0;

			if(bi->bmiHeader.biBitCount <= 8)
			{
				if(bi->bmiHeader.biClrUsed) bfh.bfOffBits += bi->bmiHeader.biClrUsed * sizeof(bi->bmiColors[0]);
				else bfh.bfOffBits += (1 << bi->bmiHeader.biBitCount) * sizeof(bi->bmiColors[0]);
			}

			fwrite(&bfh, 1, sizeof(bfh), f);
			fwrite(pData, 1, size, f);

			fclose(f);
		}
		else
		{
			AfxMessageBox(_T("Cannot create file"), MB_OK);
		}
	}
	else if(ext == _T(".jpg"))
	{
		CJpegEncoderFile(fn).Encode(pData);
	}

	CPath p(fn);

	if(CDC* pDC = m_wndStatusBar.m_status.GetDC())
	{
		CRect r;
		m_wndStatusBar.m_status.GetClientRect(r);
		p.CompactPath(pDC->m_hDC, r.Width());
		m_wndStatusBar.m_status.ReleaseDC(pDC);
	}

	SendStatusMessage((LPCTSTR)p, 3000);
}
void CMainFrame::OnFileCopyImageToCLipBoard(){
	SaveImage();
}
void CMainFrame::SaveImage(LPCTSTR fn)
{
	BYTE* pData = NULL;
	long size = 0;

	if(GetDIB(&pData, size))
	{
		
		HANDLE hMem = GlobalAlloc (GMEM_DDESHARE | GMEM_MOVEABLE, size);
		if(hMem){
			void *pMem = GlobalLock (hMem);
			CopyMemory(pMem, pData, size);

			OpenClipboard();
			EmptyClipboard();

			HANDLE hHandle = SetClipboardData(CF_DIB, pMem);
			if(!hHandle){
				AfxMessageBox(_T("SetClipboardData Failed"));
			}
			CloseClipboard();
			GlobalUnlock (hMem);
		}
		if(fn)
			SaveDIB(fn, pData, size);

		delete [] pData;
	}
}

void CMainFrame::SaveThumbnails(LPCTSTR fn)
{
	if(!pMC || !pMS || m_iPlaybackMode != PM_FILE /*&& m_iPlaybackMode != PM_DVD*/) 
		return;

	REFERENCE_TIME rtPos = GetPos();
	REFERENCE_TIME rtDur = GetDur();

	if(rtDur <= 0)
	{
		AfxMessageBox(_T("Cannot create thumbnails for files with no duration"));
		return;
	}

	pMC->Pause();
	GetMediaState(); // wait for completion of the pause command

	//

	CSize video, wh(0, 0), arxy(0, 0);

	if (m_pMFVDC)
	{
		m_pMFVDC->GetNativeVideoSize(&wh, &arxy);
	}
	else if(m_pCAP)
	{
		wh = m_pCAP->GetVideoSize(false);
		arxy = m_pCAP->GetVideoSize(true);
	}
	else
	{
		pBV->GetVideoSize(&wh.cx, &wh.cy);

		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = pBV;
		if(pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0)
			arxy.SetSize(arx, ary);
	}

	if(wh.cx <= 0 || wh.cy <= 0)
	{
		AfxMessageBox(_T("Failed to get video frame size"));
		return;
	}

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if(m_iPlaybackMode == PM_DVD && SUCCEEDED(pDVDI->GetCurrentVideoAttributes(&VATR)))
		arxy.SetSize(VATR.ulAspectX, VATR.ulAspectY);

	video = (arxy.cx <= 0 || arxy.cy <= 0) ? wh : CSize(MulDiv(wh.cy, arxy.cx, arxy.cy), wh.cy);

	//

	AppSettings& s = AfxGetAppSettings();

	int cols = max(1, min(8, s.ThumbCols));
	int rows = max(1, min(8, s.ThumbRows));

	int margin = 5;
	int infoheight = 70;
	int width = max(256, min(2048, s.ThumbWidth));
	int height = width * video.cy / video.cx * rows / cols + infoheight;

	int dibsize = sizeof(BITMAPINFOHEADER) + width*height*4;

	CAutoVectorPtr<BYTE> dib;
	if(!dib.Allocate(dibsize))
	{
		AfxMessageBox(_T("Out of memory, go buy some more!"));
		return;
	}

	BITMAPINFOHEADER* bih = (BITMAPINFOHEADER*)(BYTE*)dib;
	memset(bih, 0, sizeof(BITMAPINFOHEADER));
	bih->biSize = sizeof(BITMAPINFOHEADER);
	bih->biWidth = width;
	bih->biHeight = height;
	bih->biPlanes = 1;
	bih->biBitCount = 32;
	bih->biCompression = BI_RGB;
	bih->biSizeImage = width*height*4;
	memsetd(bih + 1, 0xffffff, bih->biSizeImage);

	SubPicDesc spd;
	spd.w = width;
	spd.h = height;
	spd.bpp = 32;
	spd.pitch = -width*4;
	spd.bits = (BYTE*)(bih + 1) + (width*4)*(height-1);

	{
		BYTE* p = (BYTE*)spd.bits;
		for(int y = 0; y < spd.h; y++, p += spd.pitch)
		for(int x = 0; x < spd.w; x++)
			((DWORD*)p)[x] = 0x010101 * (0xe0 + 0x08*y/spd.h + 0x18*(spd.w-x)/spd.w);
	}

	CCritSec csSubLock;
	RECT bbox;

	for(int i = 1, pics = cols*rows; i <= pics; i++)
	{
		REFERENCE_TIME rt = rtDur * i / (pics+1);
		DVD_HMSF_TIMECODE hmsf = RT2HMSF(rt, 25);

		SeekTo(rt);

		m_VolumeBeforeFrameStepping = m_wndToolBar.Volume;
		pBA->put_Volume(-10000);

		HRESULT hr = pFS ? pFS->Step(1, NULL) : E_FAIL;

		if(FAILED(hr))
		{
			pBA->put_Volume(m_VolumeBeforeFrameStepping);
			AfxMessageBox(_T("Cannot frame step, try a different video renderer."));
			return;
		}

		HANDLE hGraphEvent = NULL;
		pME->GetEventHandle((OAEVENT*)&hGraphEvent);

		while(hGraphEvent && WaitForSingleObject(hGraphEvent, INFINITE) == WAIT_OBJECT_0)
		{
			LONG evCode = 0, evParam1, evParam2;
			while(SUCCEEDED(pME->GetEvent(&evCode, (LONG_PTR*)&evParam1, (LONG_PTR*)&evParam2, 0)))
			{
				pME->FreeEventParams(evCode, evParam1, evParam2);
				if(EC_STEP_COMPLETE == evCode) hGraphEvent = NULL;
			}
		}

		pBA->put_Volume(m_VolumeBeforeFrameStepping);

		int col = (i-1)%cols;
		int row = (i-1)/cols;

		CSize s((width-margin*2)/cols, (height-margin*2-infoheight)/rows);
		CPoint p(margin+col*s.cx, margin+row*s.cy+infoheight);
		CRect r(p, s);
		r.DeflateRect(margin, margin);

		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = new STSStyle();
		style->marginRect.SetRectEmpty();
		rts.AddStyle(_T("thumbs"), style);

		CStringW str;
		str.Format(L"{\\an7\\1c&Hffffff&\\4a&Hb0&\\bord1\\shad4\\be1}{\\p1}m %d %d l %d %d %d %d %d %d{\\p}", 
			r.left, r.top, r.right, r.top, r.right, r.bottom, r.left, r.bottom);
		rts.Add(str, true, 0, 1, _T("thumbs"));
		str.Format(L"{\\an3\\1c&Hffffff&\\3c&H000000&\\alpha&H80&\\fs16\\b1\\bord2\\shad0\\pos(%d,%d)}%02d:%02d:%02d", 
			r.right-5, r.bottom-3, hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
		rts.Add(str, true, 1, 2, _T("thumbs"));

		rts.Render(spd, 0, 25, bbox);

		BYTE* pData = NULL;
		long size = 0;
		if(!GetDIB(&pData, size)) return;

		BITMAPINFO* bi = (BITMAPINFO*)pData;

		if(bi->bmiHeader.biBitCount != 32)
		{
			delete [] pData;
			CString str;
			str.Format(_T("你选择的图片输出格式和视频不兼容, 未能创建缩略图 (%d bpp dibs)."), bi->bmiHeader.biBitCount);
			AfxMessageBox(str);
			return;
		}

		int sw = bi->bmiHeader.biWidth;
		int sh = abs(bi->bmiHeader.biHeight);
		int sp = sw*4;
		const BYTE* src = pData + sizeof(bi->bmiHeader);
		if(bi->bmiHeader.biHeight >= 0) {src += sp*(sh-1); sp = -sp;}

		int dw = spd.w;
		int dh = spd.h;
		int dp = spd.pitch;
		BYTE* dst = (BYTE*)spd.bits + spd.pitch*r.top + r.left*4;

		for(DWORD h = r.bottom - r.top, y = 0, yd = (sh<<8)/h; h > 0; y += yd, h--)
		{
			DWORD yf = y&0xff;
			DWORD yi = y>>8;

			DWORD* s0 = (DWORD*)(src + yi*sp);
			DWORD* s1 = (DWORD*)(src + yi*sp + sp);
			DWORD* d = (DWORD*)dst;

			for(DWORD w = r.right - r.left, x = 0, xd = (sw<<8)/w; w > 0; x += xd, w--)
			{
				DWORD xf = x&0xff;
				DWORD xi = x>>8;

				DWORD c0 = s0[xi];
				DWORD c1 = s0[xi+1];
				DWORD c2 = s1[xi];
				DWORD c3 = s1[xi+1];

				c0 = ((c0&0xff00ff) + ((((c1&0xff00ff) - (c0&0xff00ff)) * xf) >> 8)) & 0xff00ff
				  | ((c0&0x00ff00) + ((((c1&0x00ff00) - (c0&0x00ff00)) * xf) >> 8)) & 0x00ff00;

				c2 = ((c2&0xff00ff) + ((((c3&0xff00ff) - (c2&0xff00ff)) * xf) >> 8)) & 0xff00ff
				  | ((c2&0x00ff00) + ((((c3&0x00ff00) - (c2&0x00ff00)) * xf) >> 8)) & 0x00ff00;

				c0 = ((c0&0xff00ff) + ((((c2&0xff00ff) - (c0&0xff00ff)) * yf) >> 8)) & 0xff00ff
				  | ((c0&0x00ff00) + ((((c2&0x00ff00) - (c0&0x00ff00)) * yf) >> 8)) & 0x00ff00;

				*d++ = c0;
			}

			dst += dp;
		}

		rts.Render(spd, 10000, 25, bbox);

		delete [] pData;
	}

	{
		CRenderedTextSubtitle rts(&csSubLock);
		rts.CreateDefaultStyle(0);
		rts.m_dstScreenSize.SetSize(width, height);
		STSStyle* style = new STSStyle();
		style->marginRect.SetRect(margin*2, margin*2, margin*2, height-infoheight-margin);
		rts.AddStyle(_T("thumbs"), style);

		CStringW str;
		str.Format(L"{\\an9\\fs%d\\b1\\bord0\\shad0\\1c&Hffffff&}%s", infoheight-10, width >= 550 ? L"射手影音播放器" : L"射手播放器");

		rts.Add(str, true, 0, 1, _T("thumbs"), _T(""), _T(""), CRect(0,0,0,0), -1);

		DVD_HMSF_TIMECODE hmsf = RT2HMSF(rtDur, 25);

		CPath path(m_wndPlaylistBar.GetCur());
		path.StripPath();
		CStringW fn = (LPCTSTR)path;

		CStringW fs;
		WIN32_FIND_DATA wfd;
		HANDLE hFind = FindFirstFile(m_wndPlaylistBar.GetCur(), &wfd);
		if(hFind != INVALID_HANDLE_VALUE)
		{
			FindClose(hFind);

			__int64 size = (__int64(wfd.nFileSizeHigh)<<32)|wfd.nFileSizeLow;
			__int64 shortsize = size;
			CStringW measure = _T("B");
			if(shortsize > 10240) shortsize /= 1024, measure = L"KB";
			if(shortsize > 10240) shortsize /= 1024, measure = L"MB";
			if(shortsize > 10240) shortsize /= 1024, measure = L"GB";
			fs.Format(L"文件尺寸: %I64d%s (%I64d bytes)\\N", shortsize, measure, size);
		}

		CStringW ar;
		if(arxy.cx > 0 && arxy.cy > 0 && arxy.cx != wh.cx && arxy.cy != wh.cy)
			ar.Format(L"(%d:%d)", arxy.cx, arxy.cy);

		str.Format(L"{\\an7\\1c&H000000&\\fs16\\b0\\bord0\\shad0}文件名: %s\\N%s分辨率: %dx%d %s\\N总长度: %02d:%02d:%02d", 
			fn, fs, wh.cx, wh.cy, ar, hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
		rts.Add(str, true, 0, 1, _T("thumbs"));

		rts.Render(spd, 0, 25, bbox);
	}

	SaveDIB(fn, (BYTE*)dib, dibsize);

	SeekTo(rtPos);
}

static CString MakeSnapshotFileName(LPCTSTR prefix)
{
	CTime t = CTime::GetCurrentTime();
	CString fn;
	fn.Format(_T("%s%s%s"), prefix, t.Format(_T("%Y%m%d%H%M%S")), AfxGetAppSettings().SnapShotExt);
	return fn;
}

BOOL CMainFrame::IsRendererCompatibleWithSaveImage()
{
	BOOL result = TRUE;
	AppSettings& s = AfxGetAppSettings();

	if(m_fRealMediaGraph) {
		if(s.iRMVideoRendererType == VIDRNDT_RM_DEFAULT) {
			AfxMessageBox(_T("The 'Save Image' and 'Save Thumbnails' functions do not work with the default video renderer for RealMedia.\nSelect one of the DirectX renderers for RealMedia in MPC's output options and reopen the file."));
			result = FALSE;
		}
	} else {
		if(m_fQuicktimeGraph) {
			if(s.iQTVideoRendererType == VIDRNDT_QT_DEFAULT) {
				AfxMessageBox(_T("The 'Save Image and 'Save Thumbnails' functions do not work with the default video renderer for QuickTime.\nSelect one of the DirectX renderers for QuickTime in MPC's output options and reopen the file."));
				result = FALSE;
			}	
		} else {
			if(m_fShockwaveGraph) {
				AfxMessageBox(_T("The 'Save Image' and 'Save Thumbnails' functions do not work for Shockwave files."));
				result = FALSE;
			} else {
				if(s.iDSVideoRendererType == VIDRNDT_DS_OVERLAYMIXER) {
					AfxMessageBox(_T("The 'Save Image' and 'Save Thumbnails' functions do not work with the Overlay Mixer video renderer.\nChange the video renderer in MPC's output options and reopen the file."));
					result = FALSE;
				}
			}
		}
	}
	return result;
}

void CMainFrame::OnFileSaveImage()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if(!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CPath psrc(s.SnapShotPath);
	psrc.Combine(s.SnapShotPath, MakeSnapshotFileName(_T("snapshot")));

	CFileDialog fd(FALSE, 0, (LPCTSTR)psrc, 
		OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
		_T("Bitmaps (*.bmp)|*.bmp|Jpeg (*.jpg)|*.jpg||"), this, 0);

	if(s.SnapShotExt == _T(".bmp")) fd.m_pOFN->nFilterIndex = 1;
	else if(s.SnapShotExt == _T(".jpg")) fd.m_pOFN->nFilterIndex = 2;

	if(fd.DoModal() != IDOK) return;

	if(fd.m_pOFN->nFilterIndex == 1) s.SnapShotExt = _T(".bmp");
	else if(fd.m_pOFN->nFilterIndex = 2) s.SnapShotExt = _T(".jpg");

	CPath pdst(fd.GetPathName());
	if(pdst.GetExtension().MakeLower() != s.SnapShotExt) pdst = CPath((LPCTSTR)pdst + s.SnapShotExt);
	CString path = (LPCTSTR)pdst;
	pdst.RemoveFileSpec();
	s.SnapShotPath = (LPCTSTR)pdst;

	SaveImage(path);
}

void CMainFrame::OnFileSaveImageAuto()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if(!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CString fn;
	fn.Format(_T("%s\\%s"), AfxGetAppSettings().SnapShotPath, MakeSnapshotFileName(_T("snapshot")));
	SaveImage(fn);
}

void CMainFrame::OnUpdateFileSaveImage(CCmdUI* pCmdUI)
{
	OAFilterState fs = GetMediaState();
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && (fs == State_Paused || fs == State_Running));
}

void CMainFrame::OnFileSaveThumbnails()
{
	AppSettings& s = AfxGetAppSettings();

	/* Check if a compatible renderer is being used */
	if(!IsRendererCompatibleWithSaveImage()) {
		return;
	}

	CPath psrc(s.SnapShotPath);
	psrc.Combine(s.SnapShotPath, MakeSnapshotFileName(_T("thumbs")));

	CSaveThumbnailsDialog fd(
		s.ThumbRows, s.ThumbCols, s.ThumbWidth,
		0, (LPCTSTR)psrc, 
		_T("Bitmaps (*.bmp)|*.bmp|Jpeg (*.jpg)|*.jpg||"), this);

	if(s.SnapShotExt == _T(".bmp")) fd.m_pOFN->nFilterIndex = 1;
	else if(s.SnapShotExt == _T(".jpg")) fd.m_pOFN->nFilterIndex = 2;

	if(fd.DoModal() != IDOK) return;

	if(fd.m_pOFN->nFilterIndex == 1) s.SnapShotExt = _T(".bmp");
	else if(fd.m_pOFN->nFilterIndex = 2) s.SnapShotExt = _T(".jpg");

	s.ThumbRows = fd.m_rows;
	s.ThumbCols = fd.m_cols;
	s.ThumbWidth = fd.m_width;

	CPath pdst(fd.GetPathName());
	if(pdst.GetExtension().MakeLower() != s.SnapShotExt) pdst = CPath((LPCTSTR)pdst + s.SnapShotExt);
	CString path = (LPCTSTR)pdst;
	pdst.RemoveFileSpec();
	s.SnapShotPath = (LPCTSTR)pdst;

	SaveThumbnails(path);
}

void CMainFrame::OnUpdateFileSaveThumbnails(CCmdUI* pCmdUI)
{
	OAFilterState fs = GetMediaState();
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && (m_iPlaybackMode == PM_FILE /*|| m_iPlaybackMode == PM_DVD*/));
}

void CMainFrame::OnFileConvert()
{
	CConvertDlg().DoModal();
}

void CMainFrame::OnUpdateFileConvert(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
}
#define NOTSUPPORTSUB  _T("启用播放器内置字幕功能需要DX7或DX9中的一些高级功能\n如果您看到本提示，可能说明您需要升级显卡驱动\n或者您的显卡没有能力支持所需功能(VMR7或9)")
void CMainFrame::OnFileLoadsubtitle()
{
#ifndef DEBUG
	if(!m_pCAP)
	{
		AfxMessageBox(NOTSUPPORTSUB , MB_OK);
		return;
	}
#endif
	static TCHAR BASED_CODE szFilter[] = 
		_T(".srt .sub .ssa .ass .smi .psb .txt .idx .usf .xss .ssf|")
		_T("*.srt;*.sub;*.ssa;*.ass;*smi;*.psb;*.txt;*.idx;*.usf;*.xss;*.ssf|")
		_T("All files (*.*)|")
		_T("*.*||");

	CFileDialog fd(TRUE, NULL, NULL, 
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY, 
		szFilter, this, 0, 1);
	if(!m_fnCurPlayingFile.IsEmpty()){
		CSVPToolBox svptool;
		CPath* spInitialDir = new CPath(m_fnCurPlayingFile);
		svptool.GetDirectoryLeft(spInitialDir, 5);
		fd.m_pOFN->lpstrInitialDir = spInitialDir->m_strPath;
	}

	if(fd.DoModal() != IDOK) return;

	if(LoadSubtitle(fd.GetPathName()))
		SetSubtitle(m_pSubStreams.GetTail());
}
void CMainFrame::OnFileLoadsubtitle2()
{
#ifndef DEBUG
	if(!m_pCAP)
	{
		AfxMessageBox(NOTSUPPORTSUB, MB_OK);
		return;
	}
#endif
	static TCHAR BASED_CODE szFilter[] = 
		_T(".srt .sub .ssa .ass .smi .psb .txt .idx .usf .xss .ssf|")
		_T("*.srt;*.sub;*.ssa;*.ass;*smi;*.psb;*.txt;*.idx;*.usf;*.xss;*.ssf|")
		_T("All files (*.*)|")
		_T("*.*||");

	CFileDialog fd(TRUE, NULL, NULL, 
		OFN_EXPLORER | OFN_ENABLESIZING | OFN_HIDEREADONLY, 
		szFilter, this, 0);

	if(!m_fnCurPlayingFile.IsEmpty()){
		CSVPToolBox svptool;
		CPath* spInitialDir = new CPath(m_fnCurPlayingFile);
		svptool.GetDirectoryLeft(spInitialDir, 5);
		fd.m_pOFN->lpstrInitialDir = spInitialDir->m_strPath;
	}
	

	if(fd.DoModal() != IDOK) return;

	if(LoadSubtitle(fd.GetPathName()))
		SetSubtitle2(m_pSubStreams2.GetTail());
}

void CMainFrame::OnUpdateFileLoadsubtitle(CCmdUI *pCmdUI)
{
#ifdef DEBUG
	pCmdUI->Enable();
#else
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && /*m_pCAP &&*/ !m_fAudioOnly);
#endif
}

void CMainFrame::OnFileSavesubtitle()
{
	int i = m_iSubtitleSel;

	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos && i >= 0)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

		if(i < pSubStream->GetStreamCount())
		{
			CLSID clsid;
			if(FAILED(pSubStream->GetClassID(&clsid)))
				continue;

			if(clsid == __uuidof(CVobSubFile))
			{
				CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

				CFileDialog fd(FALSE, NULL, NULL, 
					OFN_EXPLORER|OFN_ENABLESIZING|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST, 
					_T("VobSub (*.idx, *.sub)|*.idx;*.sub||"), this, 0, 1);
		
				if(fd.DoModal() == IDOK)
				{
					CAutoLock cAutoLock(&m_csSubLock);
					pVSF->Save(fd.GetPathName());
				}
	            
				return;
			}
			else if(clsid == __uuidof(CRenderedTextSubtitle))
			{
				CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

				CString filter;
				filter += _T("Subripper (*.srt)|*.srt|");
				filter += _T("Microdvd (*.sub)|*.sub|");
				filter += _T("Sami (*.smi)|*.smi|");
				filter += _T("Psb (*.psb)|*.psb|");
				filter += _T("Sub Station Alpha (*.ssa)|*.ssa|");
				filter += _T("Advanced Sub Station Alpha (*.ass)|*.ass|");
				filter += _T("|");

				CSaveTextFileDialog fd(pRTS->m_encoding, NULL, NULL, filter, this);
		
				if(fd.DoModal() == IDOK)
				{
					CAutoLock cAutoLock(&m_csSubLock);
					pRTS->SaveAs(fd.GetPathName(), (exttype)(fd.m_ofn.nFilterIndex-1), m_pCAP->GetFPS(), fd.GetEncoding());
				}
	            
				return;
			}
		}

		i -= pSubStream->GetStreamCount();
	}
}

void CMainFrame::OnUpdateFileSavesubtitle(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iSubtitleSel >= 0);
}

///////////////

#include "SubtitleDlDlg.h"
#include "ISDb.h"


void CMainFrame::OnUpdateFileISDBSearch(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnFileISDBUpload()
{
	CString fnVideoFile , fnSubtitleFile; 
	fnVideoFile = m_fnCurPlayingFile;
	int subDelayMS = 0;
	fnSubtitleFile = getCurPlayingSubfile(&subDelayMS);

	if (!fnSubtitleFile.IsEmpty() ){ //如果有字幕
		CString szBuf;
		if(subDelayMS){
			szBuf.Format(_T("字幕延时：%d ms\r\n"),subDelayMS );
		}
		/*
		CString szUploadMsg;
				szUploadMsg.Format(_T("本操作将上传您正在播放中的字幕： \r\n %s \r\n%s\r\n是否继续？"), fnSubtitleFile,szBuf);
		
				if ( AfxMessageBox(szUploadMsg, MB_YESNO) == IDYES){
		
					CString szLog;
		
					szLog.Format(_T("Uploading sub %s of %s with delay %d ms because user demand to ") , fnSubtitleFile, fnVideoFile ,subDelayMS );
					SVP_LogMsg(szLog);
					SVP_UploadSubFileByVideoAndSubFilePath(fnVideoFile , fnSubtitleFile, subDelayMS) ;
					if( AfxMessageBox(_T("字幕已经上传到播放器系统。建议您通过上传网页丰富相关信息。"), MB_YESNO) != IDYES){
					return;
					}
				}else*/
		
		CSVPSubUploadDlg upDlg;
		upDlg.m_szVideoPath = fnVideoFile;
		upDlg.m_szSubPath = fnSubtitleFile;
		upDlg.m_iDelayMs = subDelayMS;
		upDlg.pFrame = this;
		upDlg.DoModal();

		return;
	}
	
		CStringA url = "http://shooter.cn/sub/upload.html?";
		ShellExecute(m_hWnd, _T("open"), CString(url), NULL, NULL, SW_SHOWDEFAULT);
	
}
void CMainFrame::OnManualcheckupdate()
{
	//AfxMessageBox(_T("自动升级程序正在启动，请稍后..") );
	if (AfxGetMyApp()->IsVista() && !IsUserAnAdmin())
	{
		if(IDYES == AfxMessageBox(_T("必须启动管理员权限\r\n才可以启动自动升级程序！\r\n现在进入管理员权限么？"), MB_YESNO)){
			AfxGetMyApp()->GainAdminPrivileges(2, FALSE);
		
		}
	}else{
		CDlgChkUpdater dlgChkUpdater(this);
		if(IDOK == dlgChkUpdater.DoModal() ){
		
		}
	}
	
}
void CMainFrame::OnUpdateFileISDBUpload(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_wndPlaylistBar.GetCount() > 0 && !m_bSubUploading);
}

void CMainFrame::OnFileISDBDownload()
{

	CString fnVideoFile = m_fnCurPlayingFile;
	

	
	
		//CString szUploadMsg;
		//szUploadMsg.Format(_T("本操作将尝试为您正在播放中的视频下载字幕： \r\n %s \r\n\r\n是否继续？"), fnVideoFile);

		CSVPSubDownUpDialog csdu;
		csdu.szVidFilePath = fnVideoFile;
		csdu.pFrame = this;

		csdu.DoModal();

		/*
			if ( AfxMessageBox(szUploadMsg, MB_YESNO) == IDYES){
			
						CString szLog;
			
						szLog.Format(_T("Downloading sub for %s  because user demand to ") , fnVideoFile );
						SVP_LogMsg(szLog);
						CStringArray szaSubarr;
						m_bSubDownloading = TRUE;
						SVP_FetchSubFileByVideoFilePath(fnVideoFile , &szaSubarr, &m_statusmsgs);
						m_bSubDownloading = FALSE;
						if( szaSubarr.GetCount() > 0){
							if( LoadSubtitle(szaSubarr.GetAt(0)) )
								SetSubtitle(m_pSubStreams.GetTail());
							szUploadMsg.Format(_T("%d 个字幕已经下载成功"),szaSubarr.GetCount());
							AfxMessageBox(szUploadMsg, MB_OK);
						}else{
							if ( AfxMessageBox(_T("没有下载到匹配的字幕。\n要不要试试看通过网页搜索？"), MB_YESNO) == IDYES){
								CStringA url = "http://shooter.cn/sub/?";
								ShellExecute(m_hWnd, _T("open"), CString(url), NULL, NULL, SW_SHOWDEFAULT);
							}
						}
						
					}else
						return;*/
			

	
/*
	filehash fh;
	if(!hash(m_wndPlaylistBar.GetCur(), fh))
	{
		MessageBeep(-1);
		return;
	}

	// TODO: put this on a worker thread

	CStringA url = "http://shooter.cn/index.php?";
	CStringA args;
	args.Format("player=mpc&searchword=%s&size[0]=%016I64x&hash[0]=%016I64x", 
		UrlEncode(CStringA(fh.name)), fh.size, fh.hash);

	try
	{
		CInternetSession is;

		CStringA str;
		if(!OpenUrl(is, CString(url+args), str))
		{
			MessageBeep(-1);
			return;
		}

		CStringA ticket;
		CList<isdb_movie> movies;
		isdb_movie m;
		isdb_subtitle s;

		CAtlList<CStringA> sl;
		Explode(str, sl, '\n');

		POSITION pos = sl.GetHeadPosition();
		while(pos)
		{
			str = sl.GetNext(pos);

			CStringA param = str.Left(max(0, str.Find('=')));
			CStringA value = str.Mid(str.Find('=')+1);

			if(param == "ticket") ticket = value;
			else if(param == "movie") {m.reset(); Explode(value, m.titles, '|');}
			else if(param == "subtitle") {s.reset(); s.id = atoi(value);}
			else if(param == "name") s.name = value;
			else if(param == "discs") s.discs = atoi(value);
			else if(param == "disc_no") s.disc_no = atoi(value);
			else if(param == "format") s.format = value;
			else if(param == "iso639_2") s.iso639_2 = value;
			else if(param == "language") s.language = value;
			else if(param == "nick") s.nick = value;
			else if(param == "email") s.email = value;
			else if(param == "" && value == "endsubtitle") {m.subs.AddTail(s);}
			else if(param == "" && value == "endmovie") {movies.AddTail(m);}
			else if(param == "" && value == "end") break;
		}

		CSubtitleDlDlg dlg(movies, this);
		if(IDOK == dlg.DoModal())
		{
			if(dlg.m_fReplaceSubs)
				m_pSubStreams.RemoveAll();

			CComPtr<ISubStream> pSubStreamToSet;

			POSITION pos = dlg.m_selsubs.GetHeadPosition();
			while(pos)
			{
				isdb_subtitle& s = dlg.m_selsubs.GetNext(pos);

				CStringA url = "http://" + AfxGetAppSettings().ISDb + "/dl.php?";
				CStringA args;
				args.Format("id=%d&ticket=%s", s.id, UrlEncode(ticket));

				if(OpenUrl(is, CString(url+args), str))
				{
					CAutoPtr<CRenderedTextSubtitle> pRTS(new CRenderedTextSubtitle(&m_csSubLock));
					if(pRTS && pRTS->Open((BYTE*)(LPCSTR)str, str.GetLength(), DEFAULT_CHARSET, CString(s.name)) && pRTS->GetStreamCount() > 0)
					{
						CComPtr<ISubStream> pSubStream = pRTS.Detach();
						m_pSubStreams.AddTail(pSubStream);
						if(!pSubStreamToSet) pSubStreamToSet = pSubStream;
					}
				}
			}

			if(pSubStreamToSet)
				SetSubtitle(pSubStreamToSet);
		}
	}
	catch(CInternetException* ie)
	{
		ie->Delete();
		return;
	}
	*/
}

void CMainFrame::OnUpdateFileISDBDownload(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_pCAP && !m_fAudioOnly && !m_bSubDownloading);
}

void CMainFrame::OnFileProperties()
{
	CPPageFileInfoSheet m_fileinfo(m_wndPlaylistBar.GetCur(), this);
	m_fileinfo.DoModal();
}

void CMainFrame::OnUpdateFileProperties(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_FILE);
}

void CMainFrame::OnFileCloseMedia()
{
	CloseMedia();
}

void CMainFrame::OnFileClosePlaylist()
{
	SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
	RestoreDefaultWindowRect();
}

void CMainFrame::OnUpdateFileClose(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED || m_iMediaLoadState == MLS_LOADING);
}

// view

void CMainFrame::OnViewCaptionmenu()
{
	bool fHideCaptionMenu = AfxGetAppSettings().fHideCaptionMenu;

	AfxGetAppSettings().fHideCaptionMenu = !fHideCaptionMenu;

	if(m_fFullScreen) return;

	DWORD dwRemove = 0, dwAdd = 0;
	HMENU hMenu;

	if(!fHideCaptionMenu)
	{
		dwRemove = WS_CAPTION;
		hMenu = NULL;
	}
	else
	{
		dwAdd = WS_CAPTION;
		hMenu = NULL;// m_hMenuDefault;
	}

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	::SetMenu(m_hWnd, hMenu);
	SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);

	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCaptionmenu(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(!AfxGetAppSettings().fHideCaptionMenu);
}

void CMainFrame::OnViewControlBar(UINT nID)
{
	nID -= ID_VIEW_SEEKER;
	ShowControls(AfxGetAppSettings().nCS ^ (1<<nID));
}

void CMainFrame::OnUpdateViewControlBar(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_VIEW_SEEKER;
	pCmdUI->SetCheck(!!(AfxGetAppSettings().nCS & (1<<nID)));
}

void CMainFrame::OnViewSubresync()
{
	ShowControlBar(&m_wndSubresyncBar, !m_wndSubresyncBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewSubresync(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndSubresyncBar.IsWindowVisible());
	pCmdUI->Enable(m_pCAP && m_iSubtitleSel >= 0);
}

void CMainFrame::OnViewPlaylist()
{
	ShowControlBar(&m_wndPlaylistBar, !m_wndPlaylistBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewPlaylist(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndPlaylistBar.IsWindowVisible());
	pCmdUI->Enable(m_iMediaLoadState == MLS_CLOSED && m_iMediaLoadState != MLS_LOADED
		|| m_iMediaLoadState == MLS_LOADED /*&& (m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE)*/);
}

void CMainFrame::OnViewCapture()
{
	ShowControlBar(&m_wndCaptureBar, !m_wndCaptureBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewCapture(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndCaptureBar.IsWindowVisible());
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_CAPTURE);
}

void CMainFrame::OnViewShaderEditor()
{
	ShowControlBar(&m_wndShaderEditorBar, !m_wndShaderEditorBar.IsWindowVisible(), TRUE);
}

void CMainFrame::OnUpdateViewShaderEditor(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndShaderEditorBar.IsWindowVisible());
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnViewMinimal()
{
	if(!AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(0);
}

void CMainFrame::OnUpdateViewMinimal(CCmdUI* pCmdUI)
{
}


void CMainFrame::OnUpdateViewCompact(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewNormal()
{
	if(AfxGetAppSettings().fHideCaptionMenu)
		SendMessage(WM_COMMAND, ID_VIEW_CAPTIONMENU);
	ShowControls(CS_SEEKBAR|CS_TOOLBAR|CS_STATUSBAR);//CS_INFOBAR
}

void CMainFrame::OnUpdateViewNormal(CCmdUI* pCmdUI)
{
}

void CMainFrame::OnViewFullscreen()
{
	ToggleFullscreen(true, true);
}

void CMainFrame::OnViewFullscreenSecondary()
{
	ToggleFullscreen(true, false);
}

void CMainFrame::OnUpdateViewFullscreen(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly || m_fFullScreen);
	pCmdUI->SetCheck(m_fFullScreen);
}

void CMainFrame::OnViewZoom(UINT nID)
{
	ZoomVideoWindow(nID == ID_VIEW_ZOOM_50 ? 0.5 : nID == ID_VIEW_ZOOM_200 ? 2.0 : 1.0);
}

void CMainFrame::OnUpdateViewZoom(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
}

void CMainFrame::OnViewZoomAutoFit()
{
	ZoomVideoWindow(GetZoomAutoFitScale());
}

void CMainFrame::OnViewDefaultVideoFrame(UINT nID)
{
	AfxGetAppSettings().iDefaultVideoSize = nID - ID_VIEW_VF_HALF;
	m_ZoomX = m_ZoomY = 1; 
	m_PosX = m_PosY = 0.5;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewDefaultVideoFrame(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	
	BOOL bCurrent = ( AfxGetAppSettings().iDefaultVideoSize == (pCmdUI->m_nID - ID_VIEW_VF_HALF) );
		if(m_ZoomX != 1 ||  m_ZoomY != 1 || m_PosX != 0.5 || m_PosY != 0.5){
			bCurrent = false;
		}
	

	pCmdUI->SetRadio(bCurrent);
}

void CMainFrame::OnViewKeepaspectratio()
{
	AfxGetAppSettings().fKeepAspectRatio = !AfxGetAppSettings().fKeepAspectRatio;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewKeepaspectratio(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fKeepAspectRatio);
}

void CMainFrame::OnViewCompMonDeskARDiff()
{
	AfxGetAppSettings().fCompMonDeskARDiff = !AfxGetAppSettings().fCompMonDeskARDiff;
	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewCompMonDeskARDiff(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
	pCmdUI->SetCheck(AfxGetAppSettings().fCompMonDeskARDiff);
}

void CMainFrame::OnViewPanNScan(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	int x = 0, y = 0;
	int dx = 0, dy = 0;

	switch(nID)
	{
	case ID_VIEW_RESET: m_ZoomX = m_ZoomY = 1.0; m_PosX = m_PosY = 0.5; m_AngleX = m_AngleY = m_AngleZ = 0; break;
	case ID_VIEW_INCSIZE: x = y = 1; break;
	case ID_VIEW_DECSIZE: x = y = -1; break;
	case ID_VIEW_INCWIDTH: x = 1; break;
	case ID_VIEW_DECWIDTH: x = -1; break;
	case ID_VIEW_INCHEIGHT: y = 1; break;
	case ID_VIEW_DECHEIGHT: y = -1; break;
	case ID_PANSCAN_CENTER: m_PosX = m_PosY = 0.5; break;
	case ID_PANSCAN_MOVELEFT: dx = -1; break;
	case ID_PANSCAN_MOVERIGHT: dx = 1; break;
	case ID_PANSCAN_MOVEUP: dy = -1; break;
	case ID_PANSCAN_MOVEDOWN: dy = 1; break;
	case ID_PANSCAN_MOVEUPLEFT: dx = dy = -1; break;
	case ID_PANSCAN_MOVEUPRIGHT: dx = 1; dy = -1; break;
	case ID_PANSCAN_MOVEDOWNLEFT: dx = -1; dy = 1; break;
	case ID_PANSCAN_MOVEDOWNRIGHT: dx = dy = 1; break;
	default: break;
	}

	if(x > 0 && m_ZoomX < 3) m_ZoomX *= 1.02;
	if(x < 0 && m_ZoomX > 0.2) m_ZoomX /= 1.02;
	if(y > 0 && m_ZoomY < 3) m_ZoomY *= 1.02;
	if(y < 0 && m_ZoomY > 0.2) m_ZoomY /= 1.02;

	if(dx < 0 && m_PosX > 0) m_PosX = max(m_PosX - 0.005*m_ZoomX, 0);
	if(dx > 0 && m_PosX < 1) m_PosX = min(m_PosX + 0.005*m_ZoomX, 1);
	if(dy < 0 && m_PosY > 0) m_PosY = max(m_PosY - 0.005*m_ZoomY, 0);
	if(dy > 0 && m_PosY < 1) m_PosY = min(m_PosY + 0.005*m_ZoomY, 1);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScan(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
}

void CMainFrame::OnViewPanNScanPresets(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED) return;

	AppSettings& s = AfxGetAppSettings();

	nID -= ID_PANNSCAN_PRESETS_START;

	if(nID == s.m_pnspresets.GetCount())
	{
		CPnSPresetsDlg dlg;
		dlg.m_pnspresets.Copy(s.m_pnspresets);
		if(dlg.DoModal() == IDOK)
		{
			s.m_pnspresets.Copy(dlg.m_pnspresets);
			s.UpdateData(true);
		}
		return;
	}

	m_PosX = 0.5;
	m_PosY = 0.5;
	m_ZoomX = 1.0;
	m_ZoomY = 1.0;
	
	CString str = s.m_pnspresets[nID];

	int i = 0, j = 0;
	for(CString token = str.Tokenize(_T(","), i); !token.IsEmpty(); token = str.Tokenize(_T(","), i), j++)
	{
		float f = 0;
		if(_stscanf(token, _T("%f"), &f) != 1) continue;

		switch(j)
		{
		case 0: break;
		case 1: m_PosX = f; break;
		case 2: m_PosY = f; break;
		case 3: m_ZoomX = f; break;
		case 4: m_ZoomY = f; break;
		default: break;
		}
	}

	if(j != 5) return;

	m_PosX = min(max(m_PosX, 0), 1);
	m_PosY = min(max(m_PosY, 0), 1);
	m_ZoomX = min(max(m_ZoomX, 0.2), 3);
	m_ZoomY = min(max(m_ZoomY, 0.2), 3);

	MoveVideoWindow(true);
}

void CMainFrame::OnUpdateViewPanNScanPresets(CCmdUI* pCmdUI)
{
	int nID = pCmdUI->m_nID - ID_PANNSCAN_PRESETS_START;
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && nID >= 0 && nID <= s.m_pnspresets.GetCount());
}

void CMainFrame::OnViewRotate(UINT nID)
{

	if(nID == ID_ENABLE_ROTATE){
		return OnEnableDX9();
	}

	if(!m_pCAP) return;

	switch(nID)
	{
	case ID_PANSCAN_ROTATEXP: m_AngleX += 2; break;
	case ID_PANSCAN_ROTATEXM: m_AngleX -= 2; break;
	case ID_PANSCAN_ROTATEYP: m_AngleY += 2; break;
	case ID_PANSCAN_ROTATEYM: m_AngleY -= 2; break;
	case ID_PANSCAN_ROTATEZP: m_AngleZ += 2; break;
	case ID_PANSCAN_ROTATEZM: m_AngleZ -= 2; break;

	case ID_ROTATE_H: 
		if(m_AngleY > 180 ) m_AngleY -= 180; else m_AngleY += 180;
		break;
	case ID_ROTATE_V: 
		if(m_AngleX > 180 ) m_AngleX -= 180; else m_AngleX += 180;
		break;
	case ID_ROTATE_90:
		if(m_AngleZ > 270 ) m_AngleZ -= 270; else m_AngleZ += 90;
		break;
	case ID_ROTATE_180: 
		if(m_AngleZ > 180 ) m_AngleZ -= 180; else m_AngleZ += 180;
		break;
	case ID_ROTATE_270: 
		if(m_AngleZ < 90 ) m_AngleZ += 270; else m_AngleZ -= 90;
		break;
	case ID_ROTATE_RESET:
		m_AngleX = m_AngleY = m_AngleZ = 0;
		break;
	default: return;
	}

	m_pCAP->SetVideoAngle(Vector(DegToRad(m_AngleX), DegToRad(m_AngleY), DegToRad(m_AngleZ)));

	CString info;
	info.Format(_T("x: %d, y: %d, z: %d"), m_AngleX, m_AngleY, m_AngleZ);
	SendStatusMessage(info, 3000);
}

void CMainFrame::OnUpdateViewRotate(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();
	if(s.iDSVideoRendererType != 6 || s.iRMVideoRendererType != 2 || s.iQTVideoRendererType != 2 || s.iAPSurfaceUsage != VIDRNDT_AP_TEXTURE3D){
		if(ID_ENABLE_ROTATE == pCmdUI->m_nID){
			pCmdUI->Enable(true);
		}else{
			pCmdUI->Enable(false);
		}
		return;
	}
	if(ID_ENABLE_ROTATE == pCmdUI->m_nID){
		//pCmdUI->SetText(_T("？？？"));
		pCmdUI->Enable(false);
	}else{
		pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && m_pCAP);
	}
}

// FIXME
const static SIZE s_ar[] = {{0,0}, {4,3}, {5,4}, {16,9}, {235, 100}};

void CMainFrame::OnViewAspectRatio(UINT nID)
{
	CSize& ar = AfxGetAppSettings().AspectRatio;

	ar = s_ar[nID - ID_ASPECTRATIO_START];

	CString info;
	if(ar.cx && ar.cy) info.Format(_T("Aspect Ratio: %d:%d"), ar.cx, ar.cy);
	else info.Format(_T("Aspect Ratio: Default"));
	SendStatusMessage(info, 3000);

	MoveVideoWindow();
}

void CMainFrame::OnUpdateViewAspectRatio(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(AfxGetAppSettings().AspectRatio == s_ar[pCmdUI->m_nID - ID_ASPECTRATIO_START]);
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly);
}

void CMainFrame::OnViewAspectRatioNext()
{
	CSize& ar = AfxGetAppSettings().AspectRatio;

	UINT nID = ID_ASPECTRATIO_START;

	for(int i = 0; i < countof(s_ar); i++)
	{
		if(ar == s_ar[i])
		{
			nID += (i + 1) % countof(s_ar);
			break;
		}
	}

	OnViewAspectRatio(nID);
}

void CMainFrame::OnViewOntop(UINT nID)
{
	nID -= ID_ONTOP_NEVER;
	if(AfxGetAppSettings().iOnTop == nID)
		nID = !nID;
	SetAlwaysOnTop(nID);
}

void CMainFrame::OnUpdateViewOntop(CCmdUI* pCmdUI)
{
	pCmdUI->SetRadio(AfxGetAppSettings().iOnTop == (pCmdUI->m_nID - ID_ONTOP_NEVER));
}

void CMainFrame::OnViewOptions()
{
	ShowOptions();
}

// play

void CMainFrame::OnPlayPlay()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		if(GetMediaState() == State_Stopped) {  m_iSpeedLevel = 0; time(&m_tPlayStartTime);}

		if(m_iPlaybackMode == PM_FILE)
		{
			time_t ttNow;
			time(&ttNow);
			if( m_tPlayPauseTime > m_tPlayStartTime){
				m_tPlayStartTime += (ttNow - m_tPlayPauseTime);
			}
			if(m_fEndOfStream) SendMessage(WM_COMMAND, ID_PLAY_STOP);
			pMC->Run();
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			double dRate = 1.0;
			//if(m_iSpeedLevel != -4 && m_iSpeedLevel != 0)
			//	dRate = pow(2.0, m_iSpeedLevel >= -3 ? m_iSpeedLevel : (-m_iSpeedLevel - 8));
			dRate = 1.0 + m_iSpeedLevel * 0.1;

			pDVDC->PlayForwards(dRate, DVD_CMD_FLAG_Block, NULL);
			pDVDC->Pause(FALSE);
			pMC->Run();
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{			
			pMC->Stop(); // audio preview won't be in sync if we run it from paused state
			pMC->Run();
		}

		SetTimer(TIMER_STREAMPOSPOLLER, 40, NULL);
		SetTimer(TIMER_STREAMPOSPOLLER2, 500, NULL);
		SetTimer(TIMER_STATS, 1000, NULL);

		if(m_fFrameSteppingActive) // FIXME
		{
			m_fFrameSteppingActive = false;
			pBA->put_Volume(m_VolumeBeforeFrameStepping);
		}

		SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

		m_wndColorControlBar.CheckAbility();

		MoveVideoWindow();
	}else{
		SendMessage(WM_COMMAND, ID_FILE_OPENMEDIA);
	}

	
}

void CMainFrame::OnPlayPauseI()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		
		if(m_iPlaybackMode == PM_FILE)
		{
			pMC->Pause();
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			pMC->Pause();
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			pMC->Pause();
		} 

		SetTimer(TIMER_STREAMPOSPOLLER, 40, NULL);
		SetTimer(TIMER_STREAMPOSPOLLER2, 500, NULL);
		SetTimer(TIMER_STATS, 1000, NULL);

		SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

	}

	MoveVideoWindow();
}

void CMainFrame::OnPlayPause()
{
	// Support ffdshow queueing.
	// To avoid black out on pause, we have to lock g_ffdshowReceive to synchronize with ReceiveMine.
	time(&m_tPlayPauseTime);
	if(queueu_ffdshow_support)
	{
		CAutoLock lck(&g_ffdshowReceive);
		return OnPlayPauseI();
	}
	OnPlayPauseI();
}

void CMainFrame::OnPlayPlaypause()
{
	OAFilterState fs = GetMediaState();
	if(fs == State_Running) SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	else if(fs == State_Stopped || fs == State_Paused) SendMessage(WM_COMMAND, ID_PLAY_PLAY);
}

void CMainFrame::OnPlayStop()
{
	if(m_iMediaLoadState == MLS_LOADED)
	{
		if(m_iPlaybackMode == PM_FILE)
		{
			LONGLONG pos = 0;
			pMS->SetPositions(&pos, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
			pMC->Stop();

			// BUG: after pause or stop the netshow url source filter won't continue 
			// on the next play command, unless we cheat it by setting the file name again.
			// 
			// Note: WMPx may be using some undocumented interface to restart streaming.

			BeginEnumFilters(pGB, pEF, pBF)
			{
				CComQIPtr<IAMNetworkStatus, &IID_IAMNetworkStatus> pAMNS = pBF;
				CComQIPtr<IFileSourceFilter> pFSF = pBF;
				if(pAMNS && pFSF)
				{
					WCHAR* pFN = NULL;
					AM_MEDIA_TYPE mt;
					if(SUCCEEDED(pFSF->GetCurFile(&pFN, &mt)) && pFN && *pFN)
					{
						pFSF->Load(pFN, NULL);
						CoTaskMemFree(pFN);
					}
					break;
				}
			}
			EndEnumFilters
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			pDVDC->SetOption(DVD_ResetOnStop, TRUE);
			pMC->Stop();
			pDVDC->SetOption(DVD_ResetOnStop, FALSE);
		}
		else if(m_iPlaybackMode == PM_CAPTURE)
		{
			pMC->Stop();
		}

		m_iSpeedLevel = 0;

		if(m_fFrameSteppingActive) // FIXME
		{
			m_fFrameSteppingActive = false;
			pBA->put_Volume(m_VolumeBeforeFrameStepping);
		}

		m_fEndOfStream = false;
	}

	m_nLoops = 0;

	if(m_hWnd) 
	{
		KillTimer(TIMER_STREAMPOSPOLLER2);
		KillTimer(TIMER_STREAMPOSPOLLER);
		KillTimer(TIMER_STATS);

		MoveVideoWindow();

		if(m_iMediaLoadState == MLS_LOADED)
		{
			__int64 start, stop;
			m_wndSeekBar.GetRange(start, stop);
			GUID tf;
			pMS->GetTimeFormat(&tf);
			m_wndToolBar.SetStatusTimer(m_wndSeekBar.GetPosReal(), stop, !!m_wndSubresyncBar.IsWindowVisible(), &tf);
			
			SetAlwaysOnTop(AfxGetAppSettings().iOnTop);
		}
	}
}

void CMainFrame::OnUpdatePlayPauseStop(CCmdUI* pCmdUI)
{
	OAFilterState fs = m_fFrameSteppingActive ? State_Paused : GetMediaState();

	pCmdUI->SetCheck(fs == State_Running && pCmdUI->m_nID == ID_PLAY_PLAY
		|| fs == State_Paused && pCmdUI->m_nID == ID_PLAY_PAUSE
		|| fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_STOP
		|| (fs == State_Paused || fs == State_Running) && pCmdUI->m_nID == ID_PLAY_PLAYPAUSE);

	bool fEnable = false;

	if(fs >= 0)
	{
		if(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE)
		{
			fEnable = true;

			if(fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_PAUSE && m_fRealMediaGraph) fEnable = false; // can't go into paused state from stopped with rm
			else if(m_fCapturing) fEnable = false;
			else if(m_fLiveWM && pCmdUI->m_nID == ID_PLAY_PAUSE) fEnable = false;
		}
		else if(m_iPlaybackMode == PM_DVD)
		{
			fEnable = m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu 
				&& m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu;

			if(fs == State_Stopped && pCmdUI->m_nID == ID_PLAY_PAUSE) fEnable = false;
		}
	}
	if(pCmdUI->m_nID == ID_PLAY_PLAY) fEnable = true;
	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayFramestep(UINT nID)
{
	REFERENCE_TIME rt;

	if(pFS && m_fQuicktimeGraph)
	{
		if(GetMediaState() != State_Paused && !queueu_ffdshow_support)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		pFS->Step(nID == ID_PLAY_FRAMESTEP ? 1 : -1, NULL);
	}
	else if(pFS && nID == ID_PLAY_FRAMESTEP)
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		m_fFrameSteppingActive = true;

		m_VolumeBeforeFrameStepping = m_wndToolBar.Volume;
		pBA->put_Volume(-10000);

		pFS->Step(1, NULL);
	}
	else if(S_OK == pMS->IsFormatSupported(&TIME_FORMAT_FRAME))
	{
		if(GetMediaState() != State_Paused)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		pMS->SetTimeFormat(&TIME_FORMAT_FRAME);
		pMS->GetCurrentPosition(&rt);
		if(nID == ID_PLAY_FRAMESTEP) rt++;
		else if(nID == ID_PLAY_FRAMESTEPCANCEL) rt--;
		pMS->SetPositions(&rt, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
		pMS->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME);
	}
}

void CMainFrame::OnUpdatePlayFramestep(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly
	&& (m_iPlaybackMode != PM_DVD || m_iDVDDomain == DVD_DOMAIN_Title)
	&& m_iPlaybackMode != PM_CAPTURE
	&& !m_fLiveWM)
	{
		REFTIME AvgTimePerFrame = 0;
        if(S_OK == pMS->IsFormatSupported(&TIME_FORMAT_FRAME)
		|| pCmdUI->m_nID == ID_PLAY_FRAMESTEP && pFS && pFS->CanStep(0, NULL) == S_OK
		|| m_fQuicktimeGraph && pFS)
		{
			fEnable = true;
		}
	}

	pCmdUI->Enable(fEnable);
}
void CMainFrame::OnSmartSeek(UINT nID){
	//PostMessage( WM_COMMAND, ID_PLAY_PAUSE);
	m_wndToolBar.iBottonClicked = nID;
	m_wndToolBar.KillTimer(TIMER_STOPFASTFORWORD);
	m_wndToolBar.iFastFFWCount++;
	//fast forward or backword
	{
		int iMsg;
		if(nID == ID_PLAY_BWD){
			iMsg = ID_PLAY_SEEKBACKWARDSMALL;
		}else if(nID == ID_PLAY_FWD){
			iMsg = ID_PLAY_SEEKFORWARDSMALL;
		}
		if(m_wndToolBar.iFastFFWCount > 5   ){
			int iStepPow = (int)(m_wndToolBar.iFastFFWCount / 5) * 2;
			iStepPow = min(6, iStepPow);
			iMsg += iStepPow;
		}
		PostMessage( WM_COMMAND, iMsg);
	}

	m_wndToolBar.SetTimer(TIMER_STOPFASTFORWORD, 150, NULL);
}
void CMainFrame::OnPlaySeek(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();
	int iDirect = 1;
	REFERENCE_TIME dt = 
		nID == ID_PLAY_SEEKBACKWARDSMALL ? -10000i64*s.nJumpDistS : 
		nID == ID_PLAY_SEEKFORWARDSMALL ? +10000i64*s.nJumpDistS : 
		nID == ID_PLAY_SEEKBACKWARDMED ? -10000i64*s.nJumpDistM : 
		nID == ID_PLAY_SEEKFORWARDMED ? +10000i64*s.nJumpDistM : 
		nID == ID_PLAY_SEEKBACKWARDLARGE ? -10000i64*s.nJumpDistL : 
		nID == ID_PLAY_SEEKFORWARDLARGE ? +10000i64*s.nJumpDistL : 
		0;
	if(nID > ID_PLAY_SEEKFORWARDLARGE){
		int jumpStep = (int) ( ( nID - ID_PLAY_SEEKFORWARDLARGE + 1) / 2 );
		dt = 10000i64*s.nJumpDistL*( pow( 2.0, jumpStep) );
		if ((nID - ID_PLAY_SEEKBACKWARDSMALL) % 2 == 0) { //Backward
			dt = 0 - dt;
		}

	}
	if ((nID - ID_PLAY_SEEKBACKWARDSMALL) % 2 == 0) { //Backward
		iDirect = -1;
	}


	if(!dt) return;

	// HACK: the custom graph should support frame based seeking instead
	if(m_fShockwaveGraph) dt /= 10000i64*100;

	SeekTo(m_wndSeekBar.GetPos() + dt, iDirect, dt);
}

static int rangebsearch(REFERENCE_TIME val, CAtlArray<REFERENCE_TIME>& rta, int iDirect = 1)
{
	int i = 0, j = rta.GetCount() - 1, ret = -1;

	if(j >= 0 && val >= rta[j]) return(j);

	while(i < j)
	{
		int mid = (i + j) >> 1;
		REFERENCE_TIME midt = rta[mid];
		if(iDirect == 1){
			if(val > midt) {
				i = ++mid;
				ret = mid;
				continue;
			}
		}else if(iDirect == -1){
			if(val < midt) {
				j = --mid;
				ret = mid;
				continue;
			}
		}
		if(val == midt) {ret = mid; break;}
		else if(val < midt) {ret = mid; if(j == mid) mid--; j = mid;}
		else if(val > midt) {ret = mid; if(i == mid) mid++; i = mid;}
	}
	if(ret >= (rta.GetCount() - 1)){
		return rta.GetCount() - 1;
	}
	return(ret);
}

void CMainFrame::OnPlaySeekKey(UINT nID)
{
	if(m_kfs.GetCount() > 0)
	{
		HRESULT hr;

		if(GetMediaState() == State_Stopped)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		REFERENCE_TIME rtCurrent, rtDur;
		hr = pMS->GetCurrentPosition(&rtCurrent);
		hr = pMS->GetDuration(&rtDur);

		int dec = 1;
		if(nID == ID_PLAY_SEEKKEYBACKWARD)
			dec = -1;
		int i = rangebsearch(rtCurrent, m_kfs, dec);
// 		if(i > 0) dec = (UINT)max(min(rtCurrent - m_kfs[i-1], 10000000), 0);
// 
// 		rtCurrent = 
// 			nID == ID_PLAY_SEEKKEYBACKWARD ? max(rtCurrent - dec, 0) : 
// 			nID == ID_PLAY_SEEKKEYFORWARD ? rtCurrent : 0;
// 
// 		i = rangebsearch(rtCurrent, m_kfs);

		if(nID == ID_PLAY_SEEKKEYBACKWARD)
			rtCurrent = m_kfs[max(i, 0)];
		else if(nID == ID_PLAY_SEEKKEYFORWARD && i < m_kfs.GetCount()-1)
			rtCurrent = m_kfs[i+1];
		else
			return;

		// HACK: if d3d or something changes fpu control word the values of 
		// m_kfs may be different now (if it was asked again), adding a little
		// to the seek position eliminates this error usually.

		rtCurrent += 10;

		hr = pMS->SetPositions(
			&rtCurrent, AM_SEEKING_AbsolutePositioning|AM_SEEKING_SeekToKeyFrame, 
			NULL, AM_SEEKING_NoPositioning);
	}
}

void CMainFrame::SeekTo(REFERENCE_TIME rtPos, int fSeekToKeyFrame, REFERENCE_TIME maxStep)
{
	OAFilterState fs = GetMediaState();

	if(rtPos < 0) rtPos = 0;

	if(m_iPlaybackMode == PM_FILE)
	{
		if(fs == State_Stopped)
			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

		HRESULT hr;
		int iKeyFlag = 0;
		if(fSeekToKeyFrame)
		{
			if(!m_kfs.IsEmpty())
			{
				UINT i = rangebsearch(rtPos, m_kfs, fSeekToKeyFrame);
				if(i >= 0 && i < m_kfs.GetCount()){
					if( maxStep <= 0 || ( maxStep > 0 && _abs64( m_kfs[i] - rtPos ) < maxStep ) ){
						rtPos = m_kfs[i];
						iKeyFlag = AM_SEEKING_SeekToKeyFrame;
					}
					
				}
			}
			
		}

		hr = pMS->SetPositions(&rtPos, AM_SEEKING_AbsolutePositioning|iKeyFlag, NULL, AM_SEEKING_NoPositioning);
	}
	else if(m_iPlaybackMode == PM_DVD && m_iDVDDomain == DVD_DOMAIN_Title)
	{
		if(fs != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		DVD_HMSF_TIMECODE tc = RT2HMSF(rtPos);
		pDVDC->PlayAtTime(&tc, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);

		//		if(fs != State_Running)
		//			SendMessage(WM_COMMAND, ID_PLAY_PAUSE);
	}
	else if(m_iPlaybackMode == PM_CAPTURE)
	{
		TRACE(_T("Warning (CMainFrame::SeekTo): Trying to seek in capture mode"));
	}

	m_fEndOfStream = false;
}

void CMainFrame::OnUpdatePlaySeek(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	OAFilterState fs = GetMediaState();

	if(m_iMediaLoadState == MLS_LOADED && (fs == State_Paused || fs == State_Running))
	{
		fEnable = true;
		if(m_iPlaybackMode == PM_DVD && (m_iDVDDomain != DVD_DOMAIN_Title || fs != State_Running)) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayGoto()
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	REFTIME atpf = 0;
	if(FAILED(pBV->get_AvgTimePerFrame(&atpf)) || atpf < 0)
	{
		atpf = 0;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(atpf > 0) break;

			BeginEnumPins(pBF, pEP, pPin)
			{
				if(atpf > 0) break;

				AM_MEDIA_TYPE mt;
				pPin->ConnectionMediaType(&mt);

				if(mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo)
				{
					atpf = (REFTIME)((VIDEOINFOHEADER*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				}
				else if(mt.majortype == MEDIATYPE_Video && mt.formattype == FORMAT_VideoInfo2)
				{
					atpf = (REFTIME)((VIDEOINFOHEADER2*)mt.pbFormat)->AvgTimePerFrame / 10000000i64;
				}
			}
			EndEnumPins
		}
		EndEnumFilters
	}

	CGoToDlg dlg((int)(m_wndSeekBar.GetPos()/10000), atpf > 0 ? (float)(1.0/atpf) : 0);
	if(IDOK != dlg.DoModal() || dlg.m_time < 0) return;

	SeekTo(10000i64 * dlg.m_time);
}

void CMainFrame::OnUpdateGoto(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		fEnable = true;
		if(m_iPlaybackMode == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayChangeRate(UINT nID)
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	/* //need no more since tool bar dont have this anymore
	if(m_wndToolBar.iFastFFWCount == 0){
			if(nID == ID_PLAY_DECRATE)
				PostMessage( WM_COMMAND, ID_PLAY_SEEKFORWARDMED);
			else if(nID == ID_PLAY_DECRATE)
				PostMessage( WM_COMMAND, ID_PLAY_SEEKBACKWARDMED);
		}else if(m_wndToolBar.iFastFFWCount > 0)
			return;
		*/
	
	
	if(m_iPlaybackMode == PM_CAPTURE)
	{
		if(GetMediaState() != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		long lChannelMin = 0, lChannelMax = 0;
		pAMTuner->ChannelMinMax(&lChannelMin, &lChannelMax);
		long lChannel = 0, lVivSub = 0, lAudSub = 0;
		pAMTuner->get_Channel(&lChannel, &lVivSub, &lAudSub);

		long lFreqOrg = 0, lFreqNew = -1;
		pAMTuner->get_VideoFrequency(&lFreqOrg);

//		long lSignalStrength;
		do
		{
			if(nID == ID_PLAY_DECRATE) lChannel--;
			else if(nID == ID_PLAY_INCRATE) lChannel++;

//			if(lChannel < lChannelMin) lChannel = lChannelMax;
//			if(lChannel > lChannelMax) lChannel = lChannelMin;

			if(lChannel < lChannelMin || lChannel > lChannelMax) 
				break;

			if(FAILED(pAMTuner->put_Channel(lChannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT)))
				break;
		
			long flFoundSignal;
			pAMTuner->AutoTune(lChannel, &flFoundSignal);

			pAMTuner->get_VideoFrequency(&lFreqNew);
		}
		while(FALSE);
/*			SUCCEEDED(pAMTuner->SignalPresent(&lSignalStrength)) 
			&& (lSignalStrength != AMTUNER_SIGNALPRESENT || lFreqNew == lFreqOrg));*/

	}
	else
	{
		int iNewSpeedLevel;

		if(nID == ID_PLAY_INCRATE) {
			if(m_iSpeedLevel < 10 && m_iSpeedLevel > -30){
				iNewSpeedLevel = m_iSpeedLevel+1;
			}else{
				iNewSpeedLevel = m_iSpeedLevel+5;
			}
		}
		else if(nID == ID_PLAY_DECRATE) {
			if(m_iSpeedLevel < 10 && m_iSpeedLevel > -30){
				iNewSpeedLevel = m_iSpeedLevel-1;
			}else{
				iNewSpeedLevel = m_iSpeedLevel-5;
			}
			
		}
		//else return;
		
		HRESULT hr = E_FAIL;
		CString szMsg ;

		if(iNewSpeedLevel == -10)
		{
			if(GetMediaState() != State_Paused)
				SendMessage(WM_COMMAND, ID_PLAY_PAUSE);

			if(GetMediaState() == State_Paused) hr = S_OK;
		}
		else
		{
			double dRate = 1.0 + 0.1*iNewSpeedLevel;;//pow(2.0, iNewSpeedLevel >= -3 ? iNewSpeedLevel : (-iNewSpeedLevel - 8));
			if(fabs(dRate - 1.0) < 0.01) dRate = 1.0;
			
			if(GetMediaState() != State_Running)
				SendMessage(WM_COMMAND, ID_PLAY_PLAY);

			if(m_iPlaybackMode == PM_FILE)
			{
				hr = pMS->SetRate(dRate);
			}
			else if(m_iPlaybackMode == PM_DVD)
			{
				if(iNewSpeedLevel >= -10)
					hr = pDVDC->PlayForwards(dRate, DVD_CMD_FLAG_Block, NULL);
				else
					hr = pDVDC->PlayBackwards(dRate, DVD_CMD_FLAG_Block, NULL);
			}
			szMsg.Format(_T("播放速度变为 %0.1f"), dRate);
			
			if(FAILED(hr)){
				AppSettings& s = AfxGetAppSettings();
				if(!s.bUseWaveOutDeviceByDefault){
					s.AudioRendererDisplayName = _T("");
					s.bUseWaveOutDeviceByDefault = true;
					//TODO: switch to waveOut Device
					szMsg.Format( _T("正在切换设置以允许高速播放...") );
					SendStatusMessage(szMsg, 3000);
					ReRenderOrLoadMedia();
				}else{
					szMsg.Format( _T("建议使用WaveOut而非DirectSound类音频输出选项以支持此速率 (%0.1f)"), dRate);
				}
			}
		}

		//if(SUCCEEDED(hr))
			m_iSpeedLevel = iNewSpeedLevel;
		SendStatusMessage(szMsg, 3000);
	}
}

void CMainFrame::OnUpdatePlayChangeRate(CCmdUI* pCmdUI)
{
	bool fEnable = false;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		bool fInc = (pCmdUI->m_nID == ID_PLAY_INCRATE || pCmdUI->m_nID == ID_PLAY_FWD );

		fEnable = true;
		//if(fInc && m_iSpeedLevel >= 3) fEnable = false;
		//if(!fInc && m_iPlaybackMode == PM_FILE && m_iSpeedLevel <= -4) fEnable = false;
		//else if(!fInc && m_iPlaybackMode == PM_DVD && m_iSpeedLevel <= -11) fEnable = false;
		if(m_iPlaybackMode == PM_DVD && m_iDVDDomain != DVD_DOMAIN_Title) fEnable = false;
		else if(m_fRealMediaGraph || m_fShockwaveGraph) fEnable = false;
		else if(m_iPlaybackMode == PM_CAPTURE && (!m_wndCaptureBar.m_capdlg.IsTunerActive() || m_fCapturing)) fEnable = false;
		else if(m_fLiveWM) fEnable = false;
	}

	pCmdUI->Enable(fEnable);
}

void CMainFrame::OnPlayResetRate()
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	HRESULT hr = E_FAIL;

	if(GetMediaState() != State_Running)
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);

	if(m_iPlaybackMode == PM_FILE)
	{
		hr = pMS->SetRate(1.0);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		hr = pDVDC->PlayForwards(1.0, DVD_CMD_FLAG_Block, NULL);
	}

	if(SUCCEEDED(hr))
		m_iSpeedLevel = 0;
}

void CMainFrame::OnUpdatePlayResetRate(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED);
}

void CMainFrame::SetSubtitleDelay(int delay_ms)
{
	if(m_pCAP) {
		m_pCAP->SetSubtitleDelay(delay_ms);
		getCurPlayingSubfile();
		//CString str;
		//str.Format(_T("主字幕延时已经设为： %d ms"), delay_ms);
		//SendStatusMessage(str, 5000);
	}
	time(&m_tPlayStartTime);
}
void CMainFrame::SetSubtitleDelay2(int delay_ms)
{
	if(m_pCAP) {
		m_pCAP->SetSubtitleDelay2(delay_ms);
		getCurPlayingSubfile(NULL, 2);
// 		CString str;
// 		str.Format(_T("第二字幕延时已经设为： %d ms"), delay_ms);
// 		SendStatusMessage(str, 5000);
	}
	time(&m_tPlayStartTime);
}

void CMainFrame::OnPlayChangeAudDelay(UINT nID)
{
	if(CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pGB))
	{
		REFERENCE_TIME rtShift = pASF->GetAudioTimeShift();
		rtShift += 
			nID == ID_PLAY_INCAUDDELAY ? 500000 :
			nID == ID_PLAY_DECAUDDELAY ? -500000 : 
			0;
		pASF->SetAudioTimeShift(rtShift);

		CString str;
		str.Format(_T("Audio Delay: %I64dms"), rtShift/10000);
		SendStatusMessage(str, 3000);
	}
}

void CMainFrame::OnUpdatePlayChangeAudDelay(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!pGB /*&& !!FindFilter(__uuidof(CAudioSwitcherFilter), pGB)*/);
}

#include "ComPropertySheet.h"

void CMainFrame::OnPlayFilters(UINT nID)
{
//	ShowPPage(m_spparray[nID - ID_FILTERS_SUBITEM_START], m_hWnd);

	CComPtr<IUnknown> pUnk = m_pparray[nID - ID_FILTERS_SUBITEM_START];

	CComPropertySheet ps(ResStr(IDS_PROPSHEET_PROPERTIES), this);

	if(CComQIPtr<ISpecifyPropertyPages> pSPP = pUnk)
	{
		ps.AddPages(pSPP);
	}

	if(CComQIPtr<IBaseFilter> pBF = pUnk)
	{
		HRESULT hr;
		CComPtr<IPropertyPage> pPP = new CInternalPropertyPageTempl<CPinInfoWnd>(NULL, &hr);
		ps.AddPage(pPP, pBF);
	}

	if(ps.GetPageCount() > 0)
	{
		ps.DoModal();
		OpenSetupStatusBar();
	}
}

void CMainFrame::OnUpdatePlayFilters(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!m_fCapturing);
}

void CMainFrame::OnPlayShaders(UINT nID)
{
	if(nID == ID_SHADERS_START+2)
	{
		ShowControlBar(&m_wndShaderEditorBar, TRUE, TRUE);
		return;
	}

	if(!m_pCAP) return;

	if(nID == ID_SHADERS_START)
	{
		m_shaderlabels.RemoveAll();
	}
	else if(nID == ID_SHADERS_START+1)
	{
		if(IDOK != CShaderCombineDlg(m_shaderlabels, this).DoModal())
			return;
	}
	else if(nID >= ID_SHADERS_START+3)
	{
		MENUITEMINFO mii;
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask = MIIM_DATA;
		m_shaders.GetMenuItemInfo(nID, &mii);

		m_shaderlabels.RemoveAll();
		m_shaderlabels.AddTail(((const AppSettings::Shader*)mii.dwItemData)->label);
	}

	SetShaders();
}

void CMainFrame::OnUpdatePlayShaders(CCmdUI* pCmdUI)
{
	if(pCmdUI->m_nID >= ID_SHADERS_START)
	{
		
		pCmdUI->Enable(!!m_pCAP);

		if(pCmdUI->m_nID == ID_SHADERS_START)
		{
			pCmdUI->SetRadio(m_shaderlabels.IsEmpty());
		}
		else if(pCmdUI->m_nID == ID_SHADERS_START+1)
		{
			pCmdUI->SetRadio(m_shaderlabels.GetCount() > 1);
		}
		else if(pCmdUI->m_nID == ID_SHADERS_START+2)
		{
			pCmdUI->Enable(TRUE);
		}
		else
		{
			MENUITEMINFO mii;
			memset(&mii, 0, sizeof(mii));
			mii.cbSize = sizeof(mii);
			mii.fMask = MIIM_DATA;
			m_shaders.GetMenuItemInfo(pCmdUI->m_nID, &mii);

			pCmdUI->SetRadio(m_shaderlabels.GetCount() == 1 
				&& m_shaderlabels.GetHead() == ((AppSettings::Shader*)mii.dwItemData)->label);
		}
	}
}

void CMainFrame::OnPlayAudio(UINT nID)
{
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	if(i == -1)
	{
		ShowOptions(CPPageAudioSwitcher::IDD);
	}
	else if(i >= 0 && pSS)
	{
		pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
	}
}

void CMainFrame::OnUpdatePlayAudio(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (1 + ID_AUDIO_SUBITEM_START);

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	/*if(i == -1)
	{
		// TODO****
	}
	else*/ if(i >= 0 && pSS)
	{
		DWORD flags = 0;

		if(SUCCEEDED(pSS->Info(i, NULL, &flags, NULL, NULL, NULL, NULL, NULL)))
		{
			if(flags&AMSTREAMSELECTINFO_EXCLUSIVE) pCmdUI->SetRadio(TRUE);
			else if(flags&AMSTREAMSELECTINFO_ENABLED) pCmdUI->SetCheck(TRUE);	
			else pCmdUI->SetCheck(FALSE);
		}
		else
		{
			pCmdUI->Enable(FALSE);
		}
	}
}
void CMainFrame::OnToogleSubtitle(){
	
	m_iSubtitleSel ^= (1<<31);
	if(m_iSubtitleSel < 0 && m_iSubtitleSel2 >= 0){
		m_iSubtitleSel2 ^= (1<<31);
		UpdateSubtitle2();
	}
	UpdateSubtitle();		
	
}
void CMainFrame::OnPlaySubtitles(UINT nID)
{
	int i = (int)nID - (4 + ID_SUBTITLES_SUBITEM_START);
	int secondSub = 0;
	if(i > 100){
		secondSub = 1;
		i -= 2000;
	}

	if(i == -4)  //设置
	{
		ShowOptions(CPPageSubtitles::IDD); 
	}
	else if(i == -3)
	{   //字幕风格设置
		int i = m_iSubtitleSel;
		if(secondSub)
			i = m_iSubtitleSel2;

		POSITION pos = m_pSubStreams.GetHeadPosition();
		if(secondSub){pos = m_pSubStreams2.GetHeadPosition();}
		while(pos && i >= 0)
		{
			CComPtr<ISubStream> pSubStream ;
			if(secondSub){pSubStream = m_pSubStreams2.GetNext(pos);}else{ pSubStream = m_pSubStreams.GetNext(pos); }
			if(i < pSubStream->GetStreamCount())
			{
				CLSID clsid;
				if(FAILED(pSubStream->GetClassID(&clsid)))
					continue;

				if(clsid == __uuidof(CRenderedTextSubtitle))
				{
					CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

					CAutoPtrArray<CPPageSubStyle> pages;
					CAtlArray<STSStyle*> styles;

					POSITION pos = pRTS->m_styles.GetStartPosition();
					for(int i = 0; pos; i++)
					{
						CString key;
						STSStyle* val;
						pRTS->m_styles.GetNextAssoc(pos, key, val);
						CAutoPtr<CPPageSubStyle> page(new CPPageSubStyle(secondSub,pRTS->m_sCharSet ) );
						page->InitStyle(key, *val);
						pages.Add(page);
						styles.Add(val);
					}

					CPropertySheet dlg(_T("Styles..."), this);
					for(int i = 0; i < (int)pages.GetCount(); i++) 
						dlg.AddPage(pages[i]);

					if(dlg.DoModal() == IDOK)
					{
						for(int j = 0; j < (int)pages.GetCount(); j++)
							pages[j]->GetStyle(*styles[j]);
						if(secondSub){
							UpdateSubtitle2(true);
						}else{
							UpdateSubtitle(true);
						}
					}

					return;
				}
			}

			i -= pSubStream->GetStreamCount();
		}
	}
	else if(i == -2)
	{	
		ReloadSubtitle();
	}
	else if(i == -1) //禁用或启用
	{
		if(secondSub){
			if(m_iSubtitleSel2 == -1){
				
				if(m_iSubtitleSel == 0 && m_pSubStreams2.GetCount() > 1){
					m_iSubtitleSel2 = m_iSubtitleSel + 1;
				}else{
					m_iSubtitleSel2 = 0;
				}
			}
			else m_iSubtitleSel2 ^= (1<<31);
			UpdateSubtitle2();
			
		}else{
			if(m_iSubtitleSel == -1){
				m_iSubtitleSel = 0;
				
				if(m_iSubtitleSel2 == 0 && m_pSubStreams.GetCount() > 1)
				{
					m_iSubtitleSel = m_iSubtitleSel2 + 1;
				}else{
					m_iSubtitleSel = 0;
				}
				
			}
			else m_iSubtitleSel ^= (1<<31);
				
			UpdateSubtitle();		
		}
	}
	else if(i >= 0) //选择字幕
	{
		if(secondSub){
			m_iSubtitleSel2 = i;
			UpdateSubtitle2();
 			
		}else{
 			m_iSubtitleSel = i;
			UpdateSubtitle();	
 			
		}

	}
	if(secondSub){
		AfxGetAppSettings().fEnableSubtitles2 = !(m_iSubtitleSel2 & 0x80000000);
	}else{
		AfxGetAppSettings().fEnableSubtitles = !(m_iSubtitleSel & 0x80000000);
	}
}

void CMainFrame::OnUpdatePlaySubtitles(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID;
	int i = (int)nID - (4 + ID_SUBTITLES_SUBITEM_START);
	int secondSub = 0;
	if(i > 100){
		secondSub = 1;
		i -= 2000;
	}

	pCmdUI->Enable(m_pCAP && !m_fAudioOnly);

	if(i == -3)
	{
		pCmdUI->Enable(FALSE);

		int i = m_iSubtitleSel;
		if(secondSub){
			i = m_iSubtitleSel2;
		}

		POSITION pos = m_pSubStreams.GetHeadPosition();
		if(secondSub){pos = m_pSubStreams2.GetHeadPosition();}
		while(pos && i >= 0)
		{
			CComPtr<ISubStream> pSubStream ;
			if(secondSub){ pSubStream = m_pSubStreams2.GetNext(pos); } else { pSubStream = m_pSubStreams.GetNext(pos); }

			if(i < pSubStream->GetStreamCount())
			{
				CLSID clsid;
				if(FAILED(pSubStream->GetClassID(&clsid)))
					continue;

				if(clsid == __uuidof(CRenderedTextSubtitle))
				{
					pCmdUI->Enable(TRUE);
					break;
				}
			}

			i -= pSubStream->GetStreamCount();
		}
	}
	else if(i == -1)
	{
		if(secondSub){
			pCmdUI->SetCheck(m_iSubtitleSel2 >= 0);
		}else{
			pCmdUI->SetCheck(m_iSubtitleSel >= 0);
		}
	}
	else if(i >= 0)
	{
		if(secondSub){
			pCmdUI->SetRadio(i == abs(m_iSubtitleSel2));
		}else{
			pCmdUI->SetRadio(i == abs(m_iSubtitleSel));
		}
	}
}

void CMainFrame::OnPlayLanguage(UINT nID)
{
	nID -= ID_FILTERSTREAMS_SUBITEM_START;
	CComPtr<IAMStreamSelect> pAMSS = m_ssarray[nID];
	UINT i = nID;
	while(i > 0 && pAMSS == m_ssarray[i-1]) i--;
	if(FAILED(pAMSS->Enable(nID-i, AMSTREAMSELECTENABLE_ENABLE)))
		MessageBeep(-1);

	OpenSetupStatusBar();
}

void CMainFrame::OnUpdatePlayLanguage(CCmdUI* pCmdUI)
{

	UINT nID = pCmdUI->m_nID;
	nID -= ID_FILTERSTREAMS_SUBITEM_START;
	if(nID >= m_ssarray.GetCount()){
		return;
	}
	CComPtr<IAMStreamSelect> pAMSS = m_ssarray.GetAt(nID);
	
	UINT i = nID;
	while(i > 0 && pAMSS == m_ssarray[i-1]) i--;
	DWORD flags = 0;
	pAMSS->Info(nID-i, NULL, &flags, NULL, NULL, NULL, NULL, NULL);
	if(flags&AMSTREAMSELECTINFO_EXCLUSIVE) pCmdUI->SetRadio(TRUE);
	else if(flags&AMSTREAMSELECTINFO_ENABLED) pCmdUI->SetCheck(TRUE);	
	else pCmdUI->SetCheck(FALSE);
}

void CMainFrame::OnPlayVolume(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();
	int iPlayerVol;
	if(m_WndSizeInited){
		CString szStat;
		iPlayerVol =  m_wndToolBar.m_volctrl.GetPos();
		
		s.nVolume  = iPlayerVol ;


		if( s.AudioBoost < 1 || iPlayerVol <= 100)
			s.AudioBoost = 1;
		else
			iPlayerVol = 100 +  ( iPlayerVol - 100) * 900/ 20;

		if(iPlayerVol > 1000) {iPlayerVol = 1000;}
		szStat.Format(_T("音量: %d%%  ") , iPlayerVol );
		SendStatusMessage(szStat , 2000);
	}
	if(m_iMediaLoadState == MLS_LOADED) 
	{
		pBA->put_Volume(m_wndToolBar.Volume);
		
		
		CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
		if(pASF)
			pASF->SetNormalizeBoost(s.fAudioNormalize, s.fAudioNormalizeRecover, s.AudioBoost);

		
	}
}

void CMainFrame::OnPlayVolumeBoost(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();

	int i = (int)(50.0f*log10(s.AudioBoost));

	switch(nID)
	{
	case ID_VOLUME_BOOST_INC: i = min(i+10, 100); break;
	case ID_VOLUME_BOOST_DEC: i = max(i-10, 0); break;
	case ID_VOLUME_BOOST_MIN: i = 0; break;
	case ID_VOLUME_BOOST_MAX: i = 100; break;
	}

	s.AudioBoost = pow(10.0f, (float)i/50);

	if(CComQIPtr<IAudioSwitcherFilter> pASF = FindFilter(__uuidof(CAudioSwitcherFilter), pGB))
	{
		bool fNormalize, fNormalizeRecover;
		float boost;
		pASF->GetNormalizeBoost(fNormalize, fNormalizeRecover, boost);
		pASF->SetNormalizeBoost(fNormalize, fNormalizeRecover, s.AudioBoost);
	}
}

void CMainFrame::OnUpdatePlayVolumeBoost(CCmdUI* pCmdUI)
{
	pCmdUI->Enable();
}

void CMainFrame::OnAfterplayback(UINT nID)
{
	AppSettings& s = AfxGetAppSettings();

	s.nCLSwitches &= ~CLSW_AFTERPLAYBACK_MASK;

	switch(nID)
	{
	case ID_AFTERPLAYBACK_CLOSE: s.nCLSwitches |= CLSW_CLOSE; break;
	case ID_AFTERPLAYBACK_STANDBY: s.nCLSwitches |= CLSW_STANDBY; break;
	case ID_AFTERPLAYBACK_HIBERNATE: s.nCLSwitches |= CLSW_HIBERNATE; break;
	case ID_AFTERPLAYBACK_SHUTDOWN: s.nCLSwitches |= CLSW_SHUTDOWN; break;
	case ID_AFTERPLAYBACK_LOGOFF: s.nCLSwitches |= CLSW_LOGOFF; break;
	}	
}

void CMainFrame::OnUpdateAfterplayback(CCmdUI* pCmdUI)
{
	AppSettings& s = AfxGetAppSettings();

	bool fChecked = false;

	switch(pCmdUI->m_nID)
	{
	case ID_AFTERPLAYBACK_CLOSE: fChecked = !!(s.nCLSwitches & CLSW_CLOSE); break;
	case ID_AFTERPLAYBACK_STANDBY: fChecked = !!(s.nCLSwitches & CLSW_STANDBY); break;
	case ID_AFTERPLAYBACK_HIBERNATE: fChecked = !!(s.nCLSwitches & CLSW_HIBERNATE); break;
	case ID_AFTERPLAYBACK_SHUTDOWN: fChecked = !!(s.nCLSwitches & CLSW_SHUTDOWN); break;
	case ID_AFTERPLAYBACK_LOGOFF: fChecked = !!(s.nCLSwitches & CLSW_LOGOFF); break;
	case ID_AFTERPLAYBACK_DONOTHING: fChecked = !(s.nCLSwitches & CLSW_AFTERPLAYBACK_MASK); break;
	}

	pCmdUI->SetRadio(fChecked);
}

// navigate

void CMainFrame::OnNavigateSkip(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE)
	{
		if(m_iPlaybackMode == PM_FILE) SetupChapters();

		if(DWORD nChapters = m_pCB->ChapGetCount())
		{
			REFERENCE_TIME rtCur;
			pMS->GetCurrentPosition(&rtCur);

			REFERENCE_TIME rt = rtCur;
			CComBSTR name;
			long i;

			if(nID == ID_NAVIGATE_SKIPBACK)
			{
				rt -= 30000000;
				i = m_pCB->ChapLookup(&rt, &name);
			}
			else if(nID == ID_NAVIGATE_SKIPFORWARD)
			{
				i = m_pCB->ChapLookup(&rt, &name) + 1;
				name.Empty();
				if(i < nChapters) m_pCB->ChapGet(i, &rt, &name);
			}

			if(i >= 0 && i < nChapters)
			{
				SeekTo(rt);
				SendStatusMessage(_T("Chapter: ") + CString(name), 3000);
				return;
			}
		}

		if(nID == ID_NAVIGATE_SKIPBACK)
		{
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPBACKPLITEM);
		}
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
		{
			SendMessage(WM_COMMAND, ID_NAVIGATE_SKIPFORWARDPLITEM);
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		m_iSpeedLevel = 0;

		if(GetMediaState() != State_Running)
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);

		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		if(nID == ID_NAVIGATE_SKIPBACK)
		{
			if(Location.ChapterNum == 1 && Location.TitleNum > 1)
			{
				pDVDI->GetNumberOfChapters(Location.TitleNum-1, &ulNumOfChapters);
				pDVDC->PlayChapterInTitle(Location.TitleNum-1, ulNumOfChapters, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
			else
			{
				pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
		}
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
		{
			if(Location.ChapterNum == ulNumOfChapters && Location.TitleNum < ulNumOfTitles)
			{
				pDVDC->PlayChapterInTitle(Location.TitleNum+1, 1, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
			else
			{
				pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			}
		}
/*
        if(nID == ID_NAVIGATE_SKIPBACK)
			pDVDC->PlayPrevChapter(DVD_CMD_FLAG_Block, NULL);
		else if(nID == ID_NAVIGATE_SKIPFORWARD)
			pDVDC->PlayNextChapter(DVD_CMD_FLAG_Block, NULL);
*/
	}
}

void CMainFrame::OnUpdateNavigateSkip(CCmdUI* pCmdUI)
{
	// moved to the timer callback function, that runs less frequent
//	if(m_iPlaybackMode == PM_FILE) SetupChapters();

	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
		&& ((m_iPlaybackMode == PM_DVD 
				&& m_iDVDDomain != DVD_DOMAIN_VideoManagerMenu 
				&& m_iDVDDomain != DVD_DOMAIN_VideoTitleSetMenu)
			|| (m_iPlaybackMode == PM_FILE && (m_wndPlaylistBar.GetCount() > 1/*0*/ || m_pCB->ChapGetCount() > 1))
			|| (m_iPlaybackMode == PM_CAPTURE && !m_fCapturing))); // TODO
}

void CMainFrame::OnNavigateSkipPlaylistItem(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE)
	{
		if(m_wndPlaylistBar.GetCount() == 1)
		{
			SendMessage(WM_COMMAND, ID_PLAY_STOP); // do not remove this, unless you want a circular call with OnPlayPlay()
			SendMessage(WM_COMMAND, ID_PLAY_PLAY);
		}
		else
		{
			if(nID == ID_NAVIGATE_SKIPBACKPLITEM)
			{
				m_wndPlaylistBar.SetPrev();
			}
			else if(nID == ID_NAVIGATE_SKIPFORWARDPLITEM)
			{
				m_wndPlaylistBar.SetNext();
			}

			OpenCurPlaylistItem( -1);
		}
	}
}

void CMainFrame::OnUpdateNavigateSkipPlaylistItem(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED
		&& ((m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_CAPTURE && !m_fCapturing) && m_wndPlaylistBar.GetCount() > 1/*0*/));
}

void CMainFrame::OnNavigateMenu(UINT nID)
{
	nID -= ID_NAVIGATE_TITLEMENU;

	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_DVD)
		return;

	m_iSpeedLevel = 0;

	if(GetMediaState() != State_Running)
		SendMessage(WM_COMMAND, ID_PLAY_PLAY);

	pDVDC->ShowMenu((DVD_MENU_ID)(nID+2), DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
}

void CMainFrame::OnUpdateNavigateMenu(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_TITLEMENU;
	ULONG ulUOPs;

	if(m_iMediaLoadState != MLS_LOADED || m_iPlaybackMode != PM_DVD 
		|| FAILED(pDVDI->GetCurrentUOPS(&ulUOPs)))
	{
		pCmdUI->Enable(FALSE);
		return;
	}

	pCmdUI->Enable(!(ulUOPs & (UOP_FLAG_ShowMenu_Title << nID)));
}

void CMainFrame::OnNavigateAudio(UINT nID)
{
	nID -= ID_NAVIGATE_AUDIO_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavStreamSelectSubMenu(nID, 1);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		pDVDC->SelectAudioStream(nID, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnNavigateSubpic(UINT nID)
{
	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavStreamSelectSubMenu(nID - ID_NAVIGATE_SUBP_SUBITEM_START, 2);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		int i = (int)nID - (1 + ID_NAVIGATE_SUBP_SUBITEM_START);

		if(i == -1)
		{
			ULONG ulStreamsAvailable, ulCurrentStream;
			BOOL bIsDisabled;
			if(SUCCEEDED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled)))
				pDVDC->SetSubpictureState(bIsDisabled, DVD_CMD_FLAG_Block, NULL);
		}
		else
		{
			pDVDC->SelectSubpictureStream(i, DVD_CMD_FLAG_Block, NULL);
			pDVDC->SetSubpictureState(TRUE, DVD_CMD_FLAG_Block, NULL);
		}
	}
}

void CMainFrame::OnNavigateAngle(UINT nID)
{
	nID -= ID_NAVIGATE_ANGLE_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		OnNavStreamSelectSubMenu(nID, 0);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		pDVDC->SelectAngle(nID+1, DVD_CMD_FLAG_Block, NULL);
	}
}

void CMainFrame::OnNavigateChapters(UINT nID)
{
	nID -= ID_NAVIGATE_CHAP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		if((int)nID >= 0 && nID < m_pCB->ChapGetCount())
		{
			REFERENCE_TIME rt;
			CComBSTR name;
			if(SUCCEEDED(m_pCB->ChapGet(nID, &rt, &name)))
			{
				SeekTo(rt);
				SendStatusMessage(_T("Chapter: ") + CString(name), 3000);
			}
			return;
		}

		nID -= m_pCB->ChapGetCount();

		if((int)nID >= 0 && (int)nID < m_wndPlaylistBar.GetCount()
		&& m_wndPlaylistBar.GetSelIdx() != (int)nID)
		{
			m_wndPlaylistBar.SetSelIdx(nID);
			OpenCurPlaylistItem();
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		nID++;

		if(nID > 0 && nID <= ulNumOfTitles)
		{
			pDVDC->PlayTitle(nID, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); // sometimes this does not do anything ... 
			pDVDC->PlayChapterInTitle(nID, 1, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); // ... but this does!
			return;
		}

		nID -= ulNumOfTitles;

		if(nID > 0 && nID <= ulNumOfChapters)
		{
			pDVDC->PlayChapter(nID, DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL);
			return;
		}
	}
}

void CMainFrame::OnNavigateMenuItem(UINT nID)
{
	nID -= ID_NAVIGATE_MENU_LEFT;

	if(m_iPlaybackMode == PM_DVD)
	{
		switch(nID)
		{
		case 0: pDVDC->SelectRelativeButton(DVD_Relative_Left); break;
		case 1: pDVDC->SelectRelativeButton(DVD_Relative_Right); break;
		case 2: pDVDC->SelectRelativeButton(DVD_Relative_Upper); break;
		case 3: pDVDC->SelectRelativeButton(DVD_Relative_Lower); break;
		case 4: pDVDC->ActivateButton(); break;
		case 5: pDVDC->ReturnFromSubmenu(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); break;
		case 6: pDVDC->Resume(DVD_CMD_FLAG_Block|DVD_CMD_FLAG_Flush, NULL); break;
		default: break;
		}
	}
}

void CMainFrame::OnUpdateNavigateMenuItem(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_NAVIGATE_MENU_LEFT;
	pCmdUI->Enable(m_iMediaLoadState == MLS_LOADED && m_iPlaybackMode == PM_DVD);
}

// favorites

class CDVDStateStream : public CUnknown, public IStream
{
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv)
	{
		return 
			QI(IStream)
			CUnknown::NonDelegatingQueryInterface(riid, ppv);
	}

	__int64 m_pos;

public:
	CDVDStateStream() : CUnknown(NAME("CDVDStateStream"), NULL) {m_pos = 0;}

	DECLARE_IUNKNOWN;

	CAtlArray<BYTE> m_data;

    // ISequentialStream
	STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead)
	{
		__int64 cbRead = min(m_data.GetCount() - m_pos, (__int64)cb);
		cbRead = max(cbRead, 0);
		memcpy(pv, &m_data[(INT_PTR)m_pos], (int)cbRead);
		if(pcbRead) *pcbRead = (ULONG)cbRead;
		m_pos += cbRead;
		return S_OK;
	}
	STDMETHODIMP Write(const void* pv, ULONG cb, ULONG* pcbWritten)
	{
		BYTE* p = (BYTE*)pv;
		ULONG cbWritten = -1;
		while(++cbWritten < cb)
			m_data.Add(*p++);
		if(pcbWritten) *pcbWritten = cbWritten;
		return S_OK;
	}

	// IStream
	STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) {return E_NOTIMPL;}
	STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) {return E_NOTIMPL;}
	STDMETHODIMP CopyTo(IStream* pstm, ULARGE_INTEGER cb, ULARGE_INTEGER* pcbRead, ULARGE_INTEGER* pcbWritten) {return E_NOTIMPL;}
	STDMETHODIMP Commit(DWORD grfCommitFlags) {return E_NOTIMPL;}
	STDMETHODIMP Revert() {return E_NOTIMPL;}
	STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
	STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {return E_NOTIMPL;}
	STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) {return E_NOTIMPL;}
	STDMETHODIMP Clone(IStream** ppstm) {return E_NOTIMPL;}
};


void CMainFrame::OnFavoritesAdd(){
	OnFavoritesAddReal();
}
void CMainFrame::OnFavoritesAddReal( BOOL bRecent)
{
	AppSettings& s = AfxGetAppSettings();

	REFERENCE_TIME rtCurPos; 
	REFERENCE_TIME rtDurT; 
	BOOL bDelFav = FALSE;
	if(bRecent){
		rtCurPos = GetPos();
		rtDurT = GetDur();
		if( rtCurPos > (rtDurT * 0.93) || rtCurPos < (rtDurT * 0.07)){
			bDelFav = TRUE;
		}
	}
	if(m_iPlaybackMode == PM_FILE)
	{
		CString fn = m_wndPlaylistBar.GetCur();//m_fnCurPlayingFile;// 
		if(fn.IsEmpty()) {fn = m_fnCurPlayingFile;}
		if(fn.IsEmpty()) return;

		CString desc = fn;
		desc.Replace('\\', '/');
		int i = desc.Find(_T("://")), j = desc.Find(_T("?")), k = desc.ReverseFind('/');
		if(i >= 0) desc = j >= 0 ? desc.Left(j) : desc;
		else if(k >= 0) desc = desc.Mid(k+1);

		CString str;
		BOOL bRememberPos = TRUE;
		if(bRecent){
			str = fn;
			
		}else{
			CFavoriteAddDlg dlg(desc, fn);
			if(dlg.DoModal() != IDOK) return;
			str = dlg.m_name;
			str.Remove(';');
			bRememberPos = dlg.m_fRememberPos;
		}
		CString pos(_T("0"));
		if(bRememberPos)
			pos.Format(_T("%I64d"), GetPos());
		
		str += ';';
		str += pos;

		if(!bRecent){
			CPlaylistItem pli;
			if(m_wndPlaylistBar.GetCur(pli))
			{
				POSITION pos = pli.m_fns.GetHeadPosition();
				while(pos) str += _T(";") + pli.m_fns.GetNext(pos);
			}
		}

		if(bDelFav)
			s.DelFavByFn(FAV_FILE, bRecent,fn);
		else
			s.AddFav(FAV_FILE, str, bRecent,fn);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		WCHAR path[MAX_PATH];
		ULONG len = 0;
		pDVDI->GetDVDDirectory(path, MAX_PATH, &len);
		CString fn = path;
		fn.TrimRight(_T("/\\"));

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);
		CString desc;
		desc.Format(_T("%s - T%02d C%02d - %02d:%02d:%02d"), fn, Location.TitleNum, Location.ChapterNum, 
			Location.TimeCode.bHours, Location.TimeCode.bMinutes, Location.TimeCode.bSeconds);

		CString str;
		BOOL bRememberPos = FALSE;
		if(bRecent){
			str = fn;
			bRememberPos = TRUE;
		}else{
			CFavoriteAddDlg dlg(fn, desc);
			if(dlg.DoModal() != IDOK) return;

			str = dlg.m_name;
			str.Remove(';');

			bRememberPos = dlg.m_fRememberPos;
		}
		CString pos(_T("0"));
		if(bRememberPos)
		{
			CDVDStateStream stream;
			stream.AddRef();

			CComPtr<IDvdState> pStateData;
			CComQIPtr<IPersistStream> pPersistStream;
			if(SUCCEEDED(pDVDI->GetState(&pStateData))
			&& (pPersistStream = pStateData)
			&& SUCCEEDED(OleSaveToStream(pPersistStream, (IStream*)&stream)))
			{
				pos = BinToCString(stream.m_data.GetData(), stream.m_data.GetCount());
			}
		}

		str += ';';
		str += pos;
		if(!bRecent){
			str += ';';
			str += fn;
		}

		if(bDelFav)
			s.DelFavByFn(FAV_DVD, bRecent,fn);
		else
			s.AddFav(FAV_DVD, str,bRecent,fn);
	}
	else if(m_iPlaybackMode == PM_CAPTURE)
	{
		// TODO
	}
}

void CMainFrame::OnUpdateFavoritesAdd(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(m_iPlaybackMode == PM_FILE || m_iPlaybackMode == PM_DVD);
}

void CMainFrame::OnFavoritesOrganize()
{
	CFavoriteOrganizeDlg dlg;
	dlg.DoModal();
}

void CMainFrame::OnUpdateFavoritesOrganize(CCmdUI* pCmdUI)
{
	// TODO: Add your command update UI handler code here
}

void CMainFrame::OnFavoritesFile(UINT nID)
{
	nID -= ID_FAVORITES_FILE_START;

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	if(POSITION pos = sl.FindIndex(nID))
	{
		CAtlList<CString> fns;
		REFERENCE_TIME rtStart = 0;

		int i = 0, j = 0;
		for(CString s1 = sl.GetAt(pos), s2 = s1.Tokenize(_T(";"), i); 
			!s2.IsEmpty(); 
			s2 = s1.Tokenize(_T(";"), i), j++)
		{
			if(j == 0) ; // desc
			else if(j == 1) _stscanf(s2, _T("%I64d"), &rtStart); // pos
			else fns.AddTail(s2); // subs
		}

		m_wndPlaylistBar.Open(fns, false);
		OpenCurPlaylistItem(max(rtStart, 0));
	}
}

void CMainFrame::OnUpdateFavoritesFile(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_FILE_START;
}

void CMainFrame::OnRecentFile(UINT nID)
{
	nID -= ID_RECENT_FILE_START;
	AppSettings& s = AfxGetAppSettings();

	CRecentFileList& MRU = AfxGetAppSettings().MRU;
	MRU.ReadList();

	
	if(nID >= 0 && nID < MRU.GetSize()){
		if(!MRU[nID].IsEmpty()){
			CAtlList<CString> sl;
			sl.AddTail(MRU[nID]);
			m_wndPlaylistBar.Open(sl, false);
			OpenCurPlaylistItem();
		}
	}
	
}

void CMainFrame::OnUpdateRecentFile(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_RECENT_FILE_START;
}
void CMainFrame::OnFavoritesDVD(UINT nID)
{
	nID -= ID_FAVORITES_DVD_START;

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	if(POSITION pos = sl.FindIndex(nID)) 
	{
		CString fn;

		CDVDStateStream stream;
		stream.AddRef();

		int i = 0, j = 0;
		for(CString s1 = sl.GetAt(pos), s2 = s1.Tokenize(_T(";"), i); 
			!s2.IsEmpty(); 
			s2 = s1.Tokenize(_T(";"), i), j++)
		{
			if(j == 0) ; // desc
			else if(j == 1 && s2 != _T("0")) // state
			{
				CStringToBin(s2, stream.m_data);
			}
			else if(j == 2) fn = s2; // path
		}

		SendMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);

		CComPtr<IDvdState> pDvdState;
		HRESULT hr = OleLoadFromStream((IStream*)&stream, IID_IDvdState, (void**)&pDvdState);

		CAutoPtr<OpenDVDData> p(new OpenDVDData());
		if(p) {p->path = fn; p->pDvdState = pDvdState;}
		OpenMedia(p);
	}
}

void CMainFrame::OnUpdateFavoritesDVD(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DVD_START;
}

void CMainFrame::OnFavoritesDevice(UINT nID)
{
	nID -= ID_FAVORITES_DEVICE_START;
}

void CMainFrame::OnUpdateFavoritesDevice(CCmdUI* pCmdUI)
{
	UINT nID = pCmdUI->m_nID - ID_FAVORITES_DEVICE_START;
}

// help

void CMainFrame::OnHelpHomepage()
{
	ShellExecute(m_hWnd, _T("open"), _T("http://shooter.cn/splayer/"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnHelpDocumentation()
{
	ShellExecute(m_hWnd, _T("open"), _T("http://shooter.cn/splayer/"), NULL, NULL, SW_SHOWDEFAULT);
}

//////////////////////////////////

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	CAtlArray<HMONITOR>* ml = (CAtlArray<HMONITOR>*)dwData;
	ml->Add(hMonitor);
	return TRUE;
}

void CMainFrame::SetDefaultWindowRect(int iMonitor)
{
	AppSettings& s = AfxGetAppSettings();

	{
		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		CSize logosize = m_wndView.GetLogoSize();
		int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
		int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

		//if(GetSystemMetrics(SM_REMOTESESSION))
		//	_DEFCLIENTH = 0;

		DWORD style = GetStyle();

		/*
MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);
*/

		int w = _DEFCLIENTW + GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
			+ r1.Width() - r2.Width();
		int h = _DEFCLIENTH + GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
			//+ (mbi.rcBar.bottom - mbi.rcBar.top)
			+ r1.Height() - r2.Height()
			+ 1; // ???
//			+ 2; // ???
		if(style&WS_CAPTION) h += GetSystemMetrics(SM_CYCAPTION);


		HMONITOR hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);

		if(iMonitor > 0)
		{
			iMonitor--;
			CAtlArray<HMONITOR> ml;
			EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, (LPARAM)&ml);
			if(iMonitor < ml.GetCount()) hMonitor = ml[iMonitor];
		}

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(hMonitor, &mi);

		if(s.HasFixedWindowSize())
		{
			w = s.fixedWindowSize.cx;
			h = s.fixedWindowSize.cy;
		}
		else if(s.fRememberWindowSize)
		{
			w = s.rcLastWindowPos.Width();
			h = s.rcLastWindowPos.Height();
		}else{
			
			w = max( DEFCLIENTW , min( (mi.rcWork.right - mi.rcWork.left)  * 38 /100 , s.rcLastWindowPos.Width() ) ) ;
			h = max( DEFCLIENTH , min( (mi.rcWork.bottom - mi.rcWork.top) * 38 /100, s.rcLastWindowPos.Height()) );
		}

		int x = (mi.rcWork.left+mi.rcWork.right-w)/2;
		int y = (mi.rcWork.top+mi.rcWork.bottom-h)/2;

		if(s.fRememberWindowPos)
		{
			CRect r = s.rcLastWindowPos;
			x = r.CenterPoint().x - w/2;
			y = r.CenterPoint().y - h/2;
//			x = r.TopLeft().x;
//			y = r.TopLeft().y;
		}

		UINT lastWindowType = s.lastWindowType;

		CString szLog;
		//szLog.Format(_T(" %d %d %d %d size"),x, y, w, h);
		//SVP_LogMsg(szLog);
		MoveWindow(x, y, w, h);
		
		if(s.fRememberWindowSize && s.fRememberWindowPos)
		{
			WINDOWPLACEMENT wp;
			memset(&wp, 0, sizeof(wp));
			wp.length = sizeof(WINDOWPLACEMENT);
			if(lastWindowType == SIZE_MAXIMIZED)
				ShowWindow(SW_MAXIMIZE);
			else if(lastWindowType == SIZE_MINIMIZED)
				ShowWindow(SW_MINIMIZE);
		}
	}

	if(s.fHideCaptionMenu)
	{
		ModifyStyle(WS_CAPTION, 0, SWP_NOZORDER);
		::SetMenu(m_hWnd, NULL);
		SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
	}
	m_WndSizeInited = true;
}

void CMainFrame::RestoreDefaultWindowRect()
{
	AppSettings& s = AfxGetAppSettings();

	WINDOWPLACEMENT wp;
	GetWindowPlacement(&wp);
	if(!m_fFullScreen && wp.showCmd != SW_SHOWMAXIMIZED && wp.showCmd != SW_SHOWMINIMIZED
//	&& (GetExStyle()&WS_EX_APPWINDOW)
	&& !s.fRememberWindowSize)
	{
		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		CSize logosize = m_wndView.GetLogoSize();
		int _DEFCLIENTW = max(logosize.cx, DEFCLIENTW);
		int _DEFCLIENTH = max(logosize.cy, DEFCLIENTH);

		DWORD style = GetStyle();

		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		int w = _DEFCLIENTW + GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
			+ r1.Width() - r2.Width();
		int h = _DEFCLIENTH + GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
			+ (mbi.rcBar.bottom - mbi.rcBar.top)
			+ r1.Height() - r2.Height()
			+ 1; // ???
//			+ 2; // ???
		if(style&WS_CAPTION) h += GetSystemMetrics(SM_CYCAPTION);

		if(s.HasFixedWindowSize())
		{
			w = s.fixedWindowSize.cx;
			h = s.fixedWindowSize.cy;
		}

		CRect r;
		GetWindowRect(r);

		int x = r.CenterPoint().x - w/2;
		int y = r.CenterPoint().y - h/2;

		if(s.fRememberWindowPos)
		{
			CRect r = s.rcLastWindowPos;

			x = r.TopLeft().x;
			y = r.TopLeft().y;
		}

		MoveWindow(x, y, w, h);
	}
}

OAFilterState CMainFrame::GetMediaState()
{
	OAFilterState ret = -1;
	if(m_iMediaLoadState == MLS_LOADED) pMC->GetState(0, &ret);
	return(ret);
}

CSize CMainFrame::GetVideoSize()
{
	bool fKeepAspectRatio = AfxGetAppSettings().fKeepAspectRatio;
	bool fCompMonDeskARDiff = AfxGetAppSettings().fCompMonDeskARDiff;

	CSize ret(0,0);
	if(m_iMediaLoadState != MLS_LOADED || m_fAudioOnly)
		return ret;

	CSize wh(0, 0), arxy(0, 0);

	if (m_pMFVDC)
	{
		m_pMFVDC->GetNativeVideoSize(&wh, &arxy);	// TODO : check AR !!
	}
	else if(m_pCAP)
	{
		wh = m_pCAP->GetVideoSize(false);
		arxy = m_pCAP->GetVideoSize(fKeepAspectRatio);

		//CString szLog;
		//szLog.Format(_T("vSize %d %d %d %d") , wh.cx, wh.cy , arxy.cx , arxy.cy);
		//SVP_LogMsg(szLog);
	}
	else
	{
		pBV->GetVideoSize(&wh.cx, &wh.cy);

		long arx = 0, ary = 0;
		CComQIPtr<IBasicVideo2> pBV2 = pBV;
		if(pBV2 && SUCCEEDED(pBV2->GetPreferredAspectRatio(&arx, &ary)) && arx > 0 && ary > 0)
			arxy.SetSize(arx, ary);
	}

	if(wh.cx <= 0 || wh.cy <= 0)
		return ret;

	// with the overlay mixer IBasicVideo2 won't tell the new AR when changed dynamically
	DVD_VideoAttributes VATR;
	if(m_iPlaybackMode == PM_DVD && SUCCEEDED(pDVDI->GetCurrentVideoAttributes(&VATR)))
		arxy.SetSize(VATR.ulAspectX, VATR.ulAspectY);

	CSize& ar = AfxGetAppSettings().AspectRatio;
	if(ar.cx && ar.cy) arxy = ar;

	ret = (!fKeepAspectRatio || arxy.cx <= 0 || arxy.cy <= 0)
		? wh
		: CSize(MulDiv(wh.cy, arxy.cx, arxy.cy), wh.cy);

	if(fCompMonDeskARDiff)
	if(HDC hDC = ::GetDC(0))
	{
		int _HORZSIZE = GetDeviceCaps(hDC, HORZSIZE);
		int _VERTSIZE = GetDeviceCaps(hDC, VERTSIZE);
		int _HORZRES = GetDeviceCaps(hDC, HORZRES);
		int _VERTRES = GetDeviceCaps(hDC, VERTRES);

		if(_HORZSIZE > 0 && _VERTSIZE > 0 && _HORZRES > 0 && _VERTRES > 0)
		{
			double a = 1.0*_HORZSIZE/_VERTSIZE;
			double b = 1.0*_HORZRES/_VERTRES;

			if(b < a)
				ret.cy = (DWORD)(1.0*ret.cy * a / b);
			else if(a < b)
				ret.cx = (DWORD)(1.0*ret.cx * b / a);
		}

		::ReleaseDC(0, hDC);
	}

	return ret;
}

void CMainFrame::ToggleFullscreen(bool fToNearest, bool fSwitchScreenResWhenHasTo)
{
	CRect r;
//	const CWnd* pWndInsertAfter;
	DWORD dwRemove = 0, dwAdd = 0;
	DWORD dwRemoveEx = 0, dwAddEx = 0;
	HMENU hMenu;

	if(!m_fFullScreen)
	{
		GetWindowRect(&m_lastWindowRect);

		dispmode& dm = AfxGetAppSettings().dmFullscreenRes;
		m_dmBeforeFullscreen.fValid = false;
		if(dm.fValid && fSwitchScreenResWhenHasTo)
		{
			GetCurDispMode(m_dmBeforeFullscreen);
			SetDispMode(dm);
		}

		MONITORINFO mi;
		mi.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

		dwRemove = WS_CAPTION|WS_THICKFRAME;
		if(fToNearest) r = mi.rcMonitor;
		else GetDesktopWindow()->GetWindowRect(&r);
		hMenu = NULL;
	}
	else
	{
		if(m_dmBeforeFullscreen.fValid)
			SetDispMode(m_dmBeforeFullscreen);

		dwAdd = (AfxGetAppSettings().fHideCaptionMenu ? 0 : WS_CAPTION) | WS_THICKFRAME;
		r = m_lastWindowRect;
		hMenu = NULL;//AfxGetAppSettings().fHideCaptionMenu ? NULL : m_hMenuDefault;
	}

	m_lastMouseMove.x = m_lastMouseMove.y = -1;

	bool fAudioOnly = m_fAudioOnly;
	m_fAudioOnly = true;

	m_fFullScreen = !m_fFullScreen;

	SetAlwaysOnTop(AfxGetAppSettings().iOnTop);

	ModifyStyle(dwRemove, dwAdd, SWP_NOZORDER);
	ModifyStyleEx(dwRemoveEx, dwAddEx, SWP_NOZORDER);
	::SetMenu(m_hWnd, hMenu);
	SetWindowPos(NULL, r.left, r.top, r.Width(), r.Height(), SWP_NOZORDER|SWP_NOSENDCHANGING /*SWP_FRAMECHANGED*/);
	RedrawNonClientArea();

	if(m_fFullScreen)
	{
		m_fHideCursor = true;
        ShowControls(CS_NONE, false);
	}
	else
	{
		KillTimer(TIMER_FULLSCREENCONTROLBARHIDER);
		KillTimer(TIMER_FULLSCREENMOUSEHIDER);
		m_fHideCursor = false;
        ShowControls(AfxGetAppSettings().nCS);
	}

	m_wndView.SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE|SWP_NOZORDER);
	rePosOSD();


	m_fAudioOnly = fAudioOnly;

	MoveVideoWindow();
}

void CMainFrame::MoveVideoWindow(bool fShowStats)
{
	if(m_iMediaLoadState == MLS_LOADED && !m_fAudioOnly && IsWindowVisible())
	{
		AppSettings &s = AfxGetAppSettings();
		CRect wr;
		if(!m_fFullScreen && ( s.nCS & CS_TOOLBAR ) )
		{
			m_wndView.GetClientRect(wr);
			if(!s.fHideCaptionMenu)
				wr.DeflateRect(2, 2);

			if( m_wndColorControlBar.IsVisible() ){
				CRect rt ;
				m_wndColorControlBar.GetClientRect(rt);
				wr.bottom += rt.Height();
			}
			if( m_wndStatusBar.IsVisible() &&  !( s.nCS & CS_STATUSBAR )  ){
				CRect rt ;
				m_wndStatusBar.GetClientRect(rt);
				wr.bottom += rt.Height();
			}
		}
		else
		{
			GetWindowRect(&wr);

			// HACK
			CRect r;
			m_wndView.GetWindowRect(&r);
			wr -= r.TopLeft();
		}

		CRect vr = CRect(0,0,0,0);

		OAFilterState fs = GetMediaState();
		if(fs == State_Paused || fs == State_Running || fs == State_Stopped && (m_fShockwaveGraph || m_fQuicktimeGraph))
		{
			CSize arxy = GetVideoSize();

			int iDefaultVideoSize = AfxGetAppSettings().iDefaultVideoSize;

			CSize ws = 
				iDefaultVideoSize == DVS_HALF ? CSize(arxy.cx/2, arxy.cy/2) :
				iDefaultVideoSize == DVS_NORMAL ? arxy :
				iDefaultVideoSize == DVS_DOUBLE ? CSize(arxy.cx*2, arxy.cy*2) :
				wr.Size();

			int w = ws.cx;
			int h = ws.cy;

			if(!m_fShockwaveGraph) //&& !m_fQuicktimeGraph)
			{
				if(iDefaultVideoSize == DVS_FROMINSIDE || iDefaultVideoSize == DVS_FROMOUTSIDE)
				{
					h = ws.cy;
					w = MulDiv(h, arxy.cx, arxy.cy);

					if(iDefaultVideoSize == DVS_FROMINSIDE && w > ws.cx
					|| iDefaultVideoSize == DVS_FROMOUTSIDE && w < ws.cx)
					{
						w = ws.cx;
						h = MulDiv(w, arxy.cy, arxy.cx);
					}
				}
			}


			CSize size(
				(int)(m_ZoomX*w), 
				(int)(m_ZoomY*h));

			CPoint pos(
				(int)(m_PosX*(wr.Width()*3 - m_ZoomX*w) - wr.Width()), 
				(int)(m_PosY*(wr.Height()*3 - m_ZoomY*h) - wr.Height()));

/*			CPoint pos(
				(int)(m_PosX*(wr.Width() - size.cx)), 
				(int)(m_PosY*(wr.Height() - size.cy)));

*/
			vr = CRect(pos, size);
		}

		wr |= CRect(0,0,0,0);
		vr |= CRect(0,0,0,0);
		

		//CString szLog;
		//szLog.Format(_T("WVSize3 %d %d %d %d %d %d %d "), wr.Width(), wr.Height(), vr.Width(), vr.Height(), m_AngleX , m_AngleY , m_AngleZ);
		//SVP_LogMsg(szLog);
		if(m_pCAP)
		{
			m_pCAP->SetPosition(wr, vr);
			m_pCAP->SetVideoAngle(Vector(DegToRad(m_AngleX), DegToRad(m_AngleY), DegToRad(m_AngleZ)));
		}
		else
		{
			HRESULT hr;
			hr = pBV->SetDefaultSourcePosition();
			hr = pBV->SetDestinationPosition(vr.left, vr.top, vr.Width(), vr.Height());
			hr = pVW->SetWindowPosition(wr.left, wr.top, wr.Width(), wr.Height());

			if (m_pMFVDC) m_pMFVDC->SetVideoPosition (NULL, wr);
		}

		m_wndView.SetVideoRect(wr);

		if(fShowStats && vr.Height() > 0)
		{
			CString info;
			info.Format(_T("Pos %.2f %.2f, Zoom %.2f %.2f, AR %.2f"), m_PosX, m_PosY, m_ZoomX, m_ZoomY, (float)vr.Width()/vr.Height());
			SendStatusMessage(info, 3000);
		}
	}
	else
	{
		m_wndView.SetVideoRect();
	}
}
void CMainFrame::rePosOSD(){

	if(!m_wndView || !::IsWindow(m_wndView.m_hWnd) || !m_wndNewOSD || !::IsWindow(m_wndNewOSD.m_hWnd) || m_wndNewOSD.m_osdStr.IsEmpty()){
		return;
			
	}
	

	CRect rcView,rc;
	m_wndView.GetWindowRect(&rcView);
	if( ::GetWindowRect( m_wndView.m_hWnd, &rcView ) ){
		GetWindowRect(&rc);

		
		//rcView -= rcView.TopLeft();

		if(m_iOSDAlign == 1) { //TopLeft
			rcView.top += 5;
			rcView.bottom = rcView.top + 22;
		}else{
			rcView.bottom -= 5;
			rcView.top = rcView.bottom - 22;
		}
		rcView.left += 10;
		
		rcView.right = rcView.left + min(m_wndNewOSD.mSize.cx, rcView.Width() *9/10);
		
		m_wndNewOSD.MoveWindow(rcView);
		//m_wndView.m_wndOSD.MoveWindow(rcView);
	}
	return;
}
void CMainFrame::ZoomVideoWindow(double scale)
{
	if(m_iMediaLoadState != MLS_LOADED)
		return;

	AppSettings& s = AfxGetAppSettings();

	BOOL bThisIsAutoZoom = false;

	if(scale <= 0)
	{
		bThisIsAutoZoom = true;
		scale = 
			s.iZoomLevel == 0 ? 0.5 : 
			s.iZoomLevel == 1 ? 1.0 : 
			s.iZoomLevel == 2 ? 2.0 : 
			s.iZoomLevel == 3 ? GetZoomAutoFitScale() : 
			1.0;
	}

	if(m_fFullScreen)
	{
		OnViewFullscreen();
	}

	MINMAXINFO mmi;
	OnGetMinMaxInfo(&mmi);

	CRect r;
	int w = 0, h = 0;

	if(!m_fAudioOnly)
	{
		CSize arxy = GetVideoSize();

		long lWidth = int(arxy.cx * scale + 0.5);
		long lHeight = int(arxy.cy * scale + 0.5);

		DWORD style = GetStyle();

		CRect r1, r2;
		GetClientRect(&r1);
		m_wndView.GetClientRect(&r2);

		w = GetSystemMetrics((style&WS_CAPTION)?SM_CXSIZEFRAME:SM_CXFIXEDFRAME)*2
				+ r1.Width() - r2.Width()
				+ lWidth;

		
		MENUBARINFO mbi;
		memset(&mbi, 0, sizeof(mbi));
		mbi.cbSize = sizeof(mbi);
		::GetMenuBarInfo(m_hWnd, OBJID_MENU, 0, &mbi);

		h = GetSystemMetrics((style&WS_CAPTION)?SM_CYSIZEFRAME:SM_CYFIXEDFRAME)*2
				+ (mbi.rcBar.bottom - mbi.rcBar.top)
				+ r1.Height() - r2.Height()
				+ lHeight;

		if(style&WS_CAPTION)
		{
			h += GetSystemMetrics(SM_CYCAPTION);
			w += 2; h += 2; // for the 1 pixel wide sunken frame
			w += 2; h += 3; // for the inner black border
		}

		GetWindowRect(r);

		w = max(w, 480);
		h = max(h, 280);

		if(bThisIsAutoZoom){
			double mratio = (double)lHeight/lWidth;
			h = max(h , w * mratio + (h - lHeight));
		}
	}
	else
	{
		GetWindowRect(r);

		w = r.Width(); // mmi.ptMinTrackSize.x;
		h = r.Height();//mmi.ptMinTrackSize.y;
	}

	// center window
	//if(!s.fRememberWindowPos)
	{
		CPoint cp = r.CenterPoint();
		r.left = cp.x - w/2;
		r.top = cp.y - h/2;
	}

	r.right = r.left + w;
	r.bottom = r.top + h;

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);
	if(r.right > mi.rcWork.right) r.OffsetRect(mi.rcWork.right-r.right, 0);
	if(r.left < mi.rcWork.left) r.OffsetRect(mi.rcWork.left-r.left, 0);
	if(r.bottom > mi.rcWork.bottom) r.OffsetRect(0, mi.rcWork.bottom-r.bottom);
	if(r.top < mi.rcWork.top) r.OffsetRect(0, mi.rcWork.top-r.top);
	CRect rcWork(mi.rcWork);
	if(r.Width() >  rcWork.Width() ){
		r.left = rcWork.left;
		r.right = rcWork.right;
	}
	if(r.Height() >  rcWork.Height() ){
		r.top = rcWork.top;
		r.bottom = rcWork.bottom;
	}
	if(m_fFullScreen || !s.HasFixedWindowSize())
	{
		MoveWindow(r);
	}

//	ShowWindow(SW_SHOWNORMAL);

	MoveVideoWindow();
}
DWORD CMainFrame::GetUIStat(UINT n_CmdID)
{ //New UI
	DWORD fShow = false;
	if(n_CmdID >= ID_PLAY_PLAY && n_CmdID <= ID_PLAY_STOP ){
		OAFilterState fs = m_fFrameSteppingActive ? State_Paused : GetMediaState();
		if(fs >= 0){
			if(fs != State_Stopped && fs != State_Paused && ( n_CmdID == ID_PLAY_PAUSE || n_CmdID == ID_PLAY_STOP)) fShow = true;
		}


	}
	return fShow ;
}
double CMainFrame::GetZoomAutoFitScale()
{
	if(m_iMediaLoadState != MLS_LOADED || m_fAudioOnly)
		return 1.0;

	CSize arxy = GetVideoSize();

	double sx = 2.0/3 * m_rcDesktop.Width() / arxy.cx;
	double sy = 2.0/3 * m_rcDesktop.Height() / arxy.cy;

	return sx < sy ? sx : sy;
}

void CMainFrame::RepaintVideo()
{
	if(m_pCAP) m_pCAP->Paint(false);
}

void CMainFrame::SetShaders()
{
	if(!m_pCAP) return;

	AppSettings& s = AfxGetAppSettings();

	CAtlStringMap<const AppSettings::Shader*> s2s;

	POSITION pos = s.m_shaders.GetHeadPosition();
	while(pos)
	{
		const AppSettings::Shader* pShader = &s.m_shaders.GetNext(pos);
		s2s[pShader->label] = pShader;
	}

	m_pCAP->SetPixelShader(NULL, NULL);
	if (m_pCAP2)
		m_pCAP2->SetPixelShader2(NULL, NULL, true);

	CAtlList<CString> labels;

	pos = m_shaderlabels.GetHeadPosition();
	while(pos)
	{
		const AppSettings::Shader* pShader = NULL;
		if(s2s.Lookup(m_shaderlabels.GetNext(pos), pShader))
		{
			CStringA target = pShader->target;
			CStringA srcdata = pShader->srcdata;

			HRESULT hr = m_pCAP->SetPixelShader(srcdata, target);

			if(FAILED(hr))
			{
				m_pCAP->SetPixelShader(NULL, NULL);
				if (m_pCAP2)
					m_pCAP2->SetPixelShader2(NULL, NULL, true);
				SendStatusMessage(_T("无法启用DX9特效: ") + pShader->label, 3000);
				return;
			}

			labels.AddTail(pShader->label);
		}
	}

	if(m_iMediaLoadState == MLS_LOADED)
	{
		CString str = Implode(labels, '|');
		str.Replace(_T("|"), _T(", "));
		SendStatusMessage(_T("Shader: ") + str, 3000);
	}
}

void CMainFrame::UpdateShaders(CString label)
{
	if(!m_pCAP) return;

	if(m_shaderlabels.GetCount() <= 1)
		m_shaderlabels.RemoveAll();

	if(m_shaderlabels.IsEmpty() && !label.IsEmpty())
		m_shaderlabels.AddTail(label);

	bool fUpdate = m_shaderlabels.IsEmpty();

	POSITION pos = m_shaderlabels.GetHeadPosition();
	while(pos)
	{
		if(label == m_shaderlabels.GetNext(pos))
		{
			fUpdate = true;
			break;
		}
	}

	if(fUpdate) SetShaders();

}

void CMainFrame::SetBalance(int balance)
{
	AfxGetAppSettings().nBalance = balance;

	int sign = balance>0?-1:1;
	balance = max(100-abs(balance), 1);
	balance = (int)((log10(1.0*balance)-2)*5000*sign);
	balance = max(min(balance, 10000), -10000);

	if(m_iMediaLoadState == MLS_LOADED) 
		pBA->put_Balance(balance);
}

void CMainFrame::SetupIViAudReg()
{
	return;
	//if(!AfxGetAppSettings().fAutoSpeakerConf) 

	DWORD spc = 0, defchnum = 0;

	if(AfxGetAppSettings().fAutoSpeakerConf)
	{
		CComPtr<IDirectSound> pDS;
		if(SUCCEEDED(DirectSoundCreate(NULL, &pDS, NULL))
		&& SUCCEEDED(pDS->SetCooperativeLevel(m_hWnd, DSSCL_NORMAL)))
		{
			if(SUCCEEDED(pDS->GetSpeakerConfig(&spc)))
			{
				switch(spc)
				{
				case DSSPEAKER_DIRECTOUT: defchnum = 6; break;
				case DSSPEAKER_HEADPHONE: defchnum = 2; break;
				case DSSPEAKER_MONO: defchnum = 1; break;
				case DSSPEAKER_QUAD: defchnum = 4; break;
				default:
				case DSSPEAKER_STEREO: defchnum = 2; break;
				case DSSPEAKER_SURROUND: defchnum = 2; break;
				case DSSPEAKER_5POINT1: defchnum = 5; break;
				case DSSPEAKER_7POINT1: defchnum = 5; break;
				}
			}
		}
	}
	else
	{
		defchnum = 2;
	}

	CRegKey iviaud;
	if(ERROR_SUCCESS == iviaud.Create(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\InterVideo\\Common\\AudioDec")))
	{
		DWORD chnum = 0;
		if(FAILED(iviaud.QueryDWORDValue(_T("AUDIO"), chnum))) chnum = 0;
		if(chnum <= defchnum) // check if the user has already set it..., but we won't skip if it's lower than sensible :P
			iviaud.SetDWORDValue(_T("AUDIO"), defchnum);
	}
}

//
// Open/Close
//


void CMainFrame::OpenCreateGraphObject(OpenMediaData* pOMD)
{
	ASSERT(pGB == NULL);

	m_fCustomGraph = false;
	m_fRealMediaGraph = m_fShockwaveGraph = m_fQuicktimeGraph = false;

	AppSettings& s = AfxGetAppSettings();

	if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD))
	{
		engine_t engine = s.Formats.GetEngine(p->fns.GetHead());

		CStringA ct = GetContentType(p->fns.GetHead());

		if(ct == "video/x-ms-asf")
		{
			// TODO: put something here to make the windows media source filter load later
		}
		else if(ct == "audio/x-pn-realaudio"
		|| ct == "audio/x-pn-realaudio-plugin"
		|| ct == "audio/x-realaudio-secure"
		|| ct == "video/vnd.rn-realvideo-secure"
		|| ct == "application/vnd.rn-realmedia"
		|| ct.Find("vnd.rn-") >= 0
		|| ct.Find("realaudio") >= 0
		|| ct.Find("realvideo") >= 0)
		{
			engine = RealMedia;
		}
		else if(ct == "application/x-shockwave-flash")
		{
			engine = ShockWave;
		}
		else if(ct == "video/quicktime"
			|| ct == "application/x-quicktimeplayer")
		{
			engine = QuickTime;
		}

		HRESULT hr;
		CComPtr<IUnknown> pUnk;

		if(engine == RealMedia)
		{			
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CRealMediaGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			if(SUCCEEDED(hr) && !!(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
				m_fRealMediaGraph = true;
		}
		else if(engine == ShockWave)
		{
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CShockwaveGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			if(FAILED(hr) || !(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
				throw _T("Can't create shockwave control");

			m_fShockwaveGraph = true;
		}
		else if(engine == QuickTime)
		{
			if(!(pUnk = (IUnknown*)(INonDelegatingUnknown*)new CQuicktimeGraph(m_wndView.m_hWnd, hr)))
				throw _T("Out of memory");

			if(SUCCEEDED(hr) && !!(pGB = CComQIPtr<IGraphBuilder>(pUnk)))
                m_fQuicktimeGraph = true;
		}

		m_fCustomGraph = m_fRealMediaGraph || m_fShockwaveGraph || m_fQuicktimeGraph;

		if(!m_fCustomGraph)
		{
			pGB = new CFGManagerPlayer(_T("CFGManagerPlayer"), NULL, s.SrcFilters, s.TraFilters, m_wndView.m_hWnd);
		}
	}
	else if(OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD))
	{
		pGB = new CFGManagerDVD(_T("CFGManagerDVD"), NULL, s.SrcFilters, s.TraFilters, m_wndView.m_hWnd);
	}
	else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD))
	{
		pGB = new CFGManagerCapture(_T("CFGManagerCapture"), NULL, s.SrcFilters, s.TraFilters, m_wndView.m_hWnd);
	}

	if(!pGB)
	{
		throw _T("Failed to create the filter graph object");
	}

	pGB->AddToROT();

	pMC = pGB; pME = pGB; pMS = pGB; // general
	pVW = pGB; pBV = pGB; // video
	pBA = pGB; // audio
	pFS = pGB;

	if(!(pMC && pME && pMS)
	|| !(pVW && pBV)
	|| !(pBA))
	{
		throw _T("DirectX系统组件受损，您可能需要重新重新安装DirextX 9+");
	}

	if(FAILED(pME->SetNotifyWindow((OAHWND)m_hWnd, WM_GRAPHNOTIFY, 0)))
	{
		throw _T("Could not set target window for graph notification");
	}

	m_pProv = (IUnknown*)new CKeyProvider();
	
	if(CComQIPtr<IObjectWithSite> pObjectWithSite = pGB)
		pObjectWithSite->SetSite(m_pProv);

	m_pCB = new CDSMChapterBag(NULL, NULL);
}
HRESULT CMainFrame::OpenMMSUrlStream(CString szFn){
	HRESULT ret = E_FAIL;
	IBaseFilter* pSrcFilter;
	IFileSourceFilter* pReader;
	CoCreateInstance(CLSID_WMAsfReader, NULL, CLSCTX_INPROC, IID_IBaseFilter, 
		(LPVOID *)&pSrcFilter);
	pGB->AddFilter(pSrcFilter,_T( "WMASFReader" ));

	ret = pSrcFilter->QueryInterface( IID_IFileSourceFilter, (LPVOID *)&pReader );
	if (FAILED(ret))
	{
		pSrcFilter->Release();
		return ret;
	}

	pReader->Load(szFn, NULL);
	{
		IEnumPins  *pEnum = NULL;
		IPin       *pPin = NULL;
		HRESULT    hr;
		
		hr = pSrcFilter->EnumPins(&pEnum);
		if (FAILED(hr))
		{
			return hr;
		}
		while(pEnum->Next(1, &pPin, 0) == S_OK)
		{
			PIN_DIRECTION PinDirThis;
			hr = pPin->QueryDirection(&PinDirThis);
			if (FAILED(hr))
			{
				pPin->Release();
				ret = hr;
				break;
			}
			if (PINDIR_OUTPUT == PinDirThis)
			{
				// Found a match. Return the IPin pointer to the caller.
				HRESULT hr2 = pGB->Render(pPin);
				if (SUCCEEDED(hr2))
				{
					ret = S_OK;
				}else{

				}
			}
				// Release the pin for the next time through the loop.
			pPin->Release();
			
		}
		// No more pins. We did not find a match.
		pEnum->Release();

	}
	pSrcFilter->Release();
	pReader->Release();
	return ret;
}

class CSVPThreadLoadThreadData{
public:
	CString szVidPath;
	CMainFrame* pFrame;
	CAtlList<CString>* statusmsgs;
};

UINT __cdecl SVPThreadLoadThread( LPVOID lpParam ) 
{ 

	CSVPThreadLoadThreadData* pData = (CSVPThreadLoadThreadData*)lpParam;

	// Print the parameter values using thread-safe functions.
	CStringArray szSubArray;
	CUIntArray siSubDelayArray;
	pData->pFrame->m_bSubDownloading = TRUE;
	SVP_FetchSubFileByVideoFilePath( pData->szVidPath, &szSubArray , pData->statusmsgs) ;
	pData->pFrame->m_bSubDownloading = FALSE;
	BOOL bSubSelected = false;
	for(INT i = 0 ;i < szSubArray.GetCount(); i++){
		if(pData->pFrame->LoadSubtitle(szSubArray[i]) && !bSubSelected){
			pData->pFrame->SetSubtitle( pData->pFrame->m_pSubStreams.GetTail()  ); //Enable Subtitle
			//TODO: select correct language for idx+sub
			bSubSelected = true;
		}

	}
	delete pData;
	return 0; 
} 
void CMainFrame::SVPSubDownloadByVPath(CString szVPath, CAtlList<CString>* szaStatMsgs){
	CSVPThreadLoadThreadData* pData = new CSVPThreadLoadThreadData();
	pData->pFrame = (CMainFrame*)this;
	pData->szVidPath = szVPath;
	if( szaStatMsgs == NULL){
		pData->statusmsgs = &m_statusmsgs;
	}else{
		pData->statusmsgs = szaStatMsgs;
	}
	AfxBeginThread(SVPThreadLoadThread, pData); 
}
class CSVPSubUploadThreadData{
public:
	CString fnVideoFilePath;
	CString szSubPath;
	int iDelayMS;
	CMainFrame* pFrame;
	CAtlList<CString>* statusmsgs;
	CStringArray* szaPostTerms;
};

UINT __cdecl SVPThreadUploadSubFile( LPVOID lpParam ) 
{ 

	CSVPSubUploadThreadData* pData = (CSVPSubUploadThreadData*)lpParam;
	pData->pFrame->m_bSubUploading = TRUE;
	SVP_RealUploadSubFileByVideoAndSubFilePath(pData->fnVideoFilePath,  pData->szSubPath, pData->iDelayMS, pData->szaPostTerms);
	pData->pFrame->m_bSubUploading = FALSE;
	delete pData;
	return 0; 
} 
void CMainFrame::SVP_UploadSubFileByVideoAndSubFilePath(CString fnVideoFilePath, CString szSubPath, int iDelayMS, CAtlList<CString>* szaStatMsgs, CStringArray* szaPostTerms){
	CSVPSubUploadThreadData* pData = new CSVPSubUploadThreadData();
	pData->fnVideoFilePath = fnVideoFilePath;
	pData->szSubPath = szSubPath;
	pData->iDelayMS = iDelayMS;
	if( szaStatMsgs == NULL){
		pData->statusmsgs = &m_statusmsgs;
	}else{
		pData->statusmsgs = szaStatMsgs;
	}
	pData->pFrame = (CMainFrame*)this;
	pData->szaPostTerms = szaPostTerms;
	AfxBeginThread(SVPThreadUploadSubFile, pData); 
}

void CMainFrame::OpenFile(OpenFileData* pOFD)
{
	if(pOFD->fns.IsEmpty())
		throw _T("Invalid argument");

	AppSettings& s = AfxGetAppSettings();

	bool fFirst = true;

	POSITION pos = pOFD->fns.GetHeadPosition();
	while(pos)
	{
		CString fn = pOFD->fns.GetNext(pos);

		fn.Trim();
		if(fn.IsEmpty() && !fFirst)
			break;

		
		HRESULT hr = pGB->RenderFile(CStringW(fn), NULL);
		if(FAILED(hr) && ( fn.MakeLower().Find(_T("mms://")) == 0 || fn.MakeLower().Find(_T("mmsh://")) == 0 )){
			//render mms our own way
			hr = OpenMMSUrlStream(fn);
		}
		if(FAILED(hr))
		{
			if(fFirst)
			{
				//if(s.fReportFailedPins)
				{
					CComQIPtr<IGraphBuilderDeadEnd> pGBDE = pGB;
					if(pGBDE && pGBDE->GetCount()) CMediaTypesDlg(pGBDE, this).DoModal();
				}

				CString err;

				switch(hr)
				{
				case E_ABORT: err = _T("操作已取消"); break;
				case E_FAIL: case E_POINTER: default: 
					err.Format(_T("未能打开文件, %X") , hr);
					break;
				case E_INVALIDARG: err = _T("非法文件名"); break;
				case E_OUTOFMEMORY: err = _T("内存不足"); break;
				case VFW_E_CANNOT_CONNECT: err = _T("未能解码"); break;
				case VFW_E_CANNOT_LOAD_SOURCE_FILTER: err = _T("未能打开文件(Source)"); break;
				case VFW_E_CANNOT_RENDER: err = _T("未能打开文件(Render)"); break;
				case VFW_E_INVALID_FILE_FORMAT: err = _T("Invalid file format"); break;
				case VFW_E_NOT_FOUND: err = _T("文件不存在"); break;
				case VFW_E_UNKNOWN_FILE_TYPE: err = _T("未知文件类型"); break;
				case VFW_E_UNSUPPORTED_STREAM: err = _T("不支持的流类型"); break;
				}

				throw err;
			}
		}

		if(s.fKeepHistory)
		{
			if(this->m_lastUrl == fn){
				CRecentFileList* pMRU = &s.MRUUrl;
				pMRU->ReadList();
				pMRU->Add(fn);
				pMRU->WriteList();
				this->m_lastUrl.Empty();
			}else{
				CRecentFileList* pMRU = fFirst ? &s.MRU : &s.MRUDub;
				pMRU->ReadList();
				pMRU->Add(fn);
				pMRU->WriteList();
			}
		}

		if(fFirst)
		{
			pOFD->title = fn;
			m_fnCurPlayingFile = fn;
			//是否有字幕？ 沒有则下载字幕
			if ( pOFD->subs.GetCount() <= 0){
				AppSettings & s = AfxGetAppSettings();
				if(s.autoDownloadSVPSub){
					CPath fPath(fn);
					CString szExt;
					szExt.Format(_T(" %s;"),fPath.GetExtension());
					if(s.CheckSVPSubExts.Find(szExt) >= 0 ){
						SVPSubDownloadByVPath(fn);
					}else{
						SendStatusMessage(  _T("正在播放的文件类型看来不需要字幕，终止自动智能匹配"), 1000);
					}

					
				}
			}

		}

		fFirst = false;

		if(m_fCustomGraph) break;
	}

	//if(s.fReportFailedPins)
	{
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = pGB;
		if(pGBDE && pGBDE->GetCount()) CMediaTypesDlg(pGBDE, this).DoModal();
	}

	if(!(pAMOP = pGB))
	{
		BeginEnumFilters(pGB, pEF, pBF)
			if(pAMOP = pBF) break;
		EndEnumFilters
	}

	if(FindFilter(__uuidof(CShoutcastSource), pGB))
		m_fUpdateInfoBar = true;

	SetupChapters();

	CComQIPtr<IKeyFrameInfo> pKFI;
	BeginEnumFilters(pGB, pEF, pBF)
		if(pKFI = pBF) break;
	EndEnumFilters
	UINT nKFs = 0, nKFsTmp = 0;
	if(pKFI && S_OK == pKFI->GetKeyFrameCount(nKFs) && nKFs > 0)
	{
		m_kfs.SetCount(nKFsTmp = nKFs);
		if(S_OK != pKFI->GetKeyFrames(&TIME_FORMAT_MEDIA_TIME, m_kfs.GetData(), nKFsTmp) || nKFsTmp != nKFs)
			m_kfs.RemoveAll();
	}

	m_iPlaybackMode = PM_FILE;
}
CString CMainFrame::GetCurPlayingFileName(){
	CString szPlayingFileName;
	if( m_iMediaLoadState == MLS_LOADED ){
		if(m_iPlaybackMode == PM_FILE){
			szPlayingFileName = PathFindFileName(m_fnCurPlayingFile);
			//szPlayingFileName =  m_fnCurPlayingFile;
		}else if(m_iPlaybackMode == PM_DVD){
			szPlayingFileName = _T("DVD");
		}else{
			szPlayingFileName = _T("未知");
		}
	}
	return szPlayingFileName;
}
void CMainFrame::SetupChapters()
{
	ASSERT(m_pCB);

	m_pCB->ChapRemoveAll();

	CInterfaceList<IBaseFilter> pBFs;
	BeginEnumFilters(pGB, pEF, pBF) 
		pBFs.AddTail(pBF);
	EndEnumFilters

	POSITION pos;

	pos = pBFs.GetHeadPosition();
	while(pos && !m_pCB->ChapGetCount())
	{
		IBaseFilter* pBF = pBFs.GetNext(pos);

		CComQIPtr<IDSMChapterBag> pCB = pBF;
		if(!pCB) continue;

		for(DWORD i = 0, cnt = pCB->ChapGetCount(); i < cnt; i++)
		{
			REFERENCE_TIME rt;
			CComBSTR name;
			if(SUCCEEDED(pCB->ChapGet(i, &rt, &name)))
				m_pCB->ChapAppend(rt, name);
		}
	}

	pos = pBFs.GetHeadPosition();
	while(pos && !m_pCB->ChapGetCount())
	{
		IBaseFilter* pBF = pBFs.GetNext(pos);

		CComQIPtr<IChapterInfo> pCI = pBF;
		if(!pCI) continue;

		CHAR iso6391[3];
		::GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, iso6391, 3);
		CStringA iso6392 = ISO6391To6392(iso6391);
		if(iso6392.GetLength() < 3) iso6392 = "eng";

		UINT cnt = pCI->GetChapterCount(CHAPTER_ROOT_ID);
		for(UINT i = 1; i <= cnt; i++)
		{
			UINT cid = pCI->GetChapterId(CHAPTER_ROOT_ID, i);

			ChapterElement ce;
			if(pCI->GetChapterInfo(cid, &ce))
			{
				char pl[3] = {iso6392[0], iso6392[1], iso6392[2]};
				char cc[] = "  ";
				CComBSTR name;
				name.Attach(pCI->GetChapterStringInfo(cid, pl, cc));
				m_pCB->ChapAppend(ce.rtStart, name);
			}
		}
	}

	pos = pBFs.GetHeadPosition();
	while(pos && !m_pCB->ChapGetCount())
	{
		IBaseFilter* pBF = pBFs.GetNext(pos);

		CComQIPtr<IAMExtendedSeeking, &IID_IAMExtendedSeeking> pES = pBF;
		if(!pES) continue;

		long MarkerCount = 0;
		if(SUCCEEDED(pES->get_MarkerCount(&MarkerCount)))
		{
			for(long i = 1; i <= MarkerCount; i++)
			{
				double MarkerTime = 0;
				if(SUCCEEDED(pES->GetMarkerTime(i, &MarkerTime)))
				{
					CStringW name;
					name.Format(L"Chapter %d", i);

					CComBSTR bstr;
					if(S_OK == pES->GetMarkerName(i, &bstr))
						name = bstr;

					m_pCB->ChapAppend(REFERENCE_TIME(MarkerTime*10000000), name);
				}
			}
		}
	}

	pos = pBFs.GetHeadPosition();
	while(pos && !m_pCB->ChapGetCount())
	{
		IBaseFilter* pBF = pBFs.GetNext(pos);

		if(GetCLSID(pBF) != CLSID_OggSplitter)
			continue;

		BeginEnumPins(pBF, pEP, pPin)
		{
			if(m_pCB->ChapGetCount()) break;

			if(CComQIPtr<IPropertyBag> pPB = pPin)
			{
            	for(int i = 1; ; i++)
				{
					CStringW str;
					CComVariant var;

					var.Clear();
					str.Format(L"CHAPTER%02d", i);
					if(S_OK != pPB->Read(str, &var, NULL)) 
						break;

					int h, m, s, ms;
					WCHAR wc;
					if(7 != swscanf(CStringW(var), L"%d%c%d%c%d%c%d", &h, &wc, &m, &wc, &s, &wc, &ms)) 
						break;

					CStringW name;
					name.Format(L"Chapter %d", i);
					var.Clear();
                    str += L"NAME";
					if(S_OK == pPB->Read(str, &var, NULL))
						name = var;

					m_pCB->ChapAppend(10000i64*(((h*60 + m)*60 + s)*1000 + ms), name);
				}
			}
		}
		EndEnumPins
	}

	m_pCB->ChapSort();
}

void CMainFrame::OpenDVD(OpenDVDData* pODD)
{
	HRESULT hr = pGB->RenderFile(CStringW(pODD->path), NULL);

	AppSettings& s = AfxGetAppSettings();

	//if(s.fReportFailedPins)
	{
		CComQIPtr<IGraphBuilderDeadEnd> pGBDE = pGB;
		if(pGBDE && pGBDE->GetCount()) CMediaTypesDlg(pGBDE, this).DoModal();
	}

	BeginEnumFilters(pGB, pEF, pBF)
	{
		if((pDVDC = pBF) && (pDVDI = pBF))
			break;
	}
	EndEnumFilters

	if(hr == E_INVALIDARG)
		throw _T("Cannot find DVD directory");
	else if(hr == VFW_E_CANNOT_RENDER)
		throw _T("Failed to render all pins of the DVD Navigator filter");
	else if(hr == VFW_S_PARTIAL_RENDER)
		throw _T("Failed to render some of the pins of the DVD Navigator filter");
	else if(hr == E_NOINTERFACE || !pDVDC || !pDVDI)
		throw _T("Failed to query the needed interfaces for DVD playback");
	else if(hr == VFW_E_CANNOT_LOAD_SOURCE_FILTER)
		throw _T("Can't create the DVD Navigator filter");
	else if(FAILED(hr))
		throw _T("Failed");

	WCHAR buff[MAX_PATH];
	ULONG len = 0;
	if(SUCCEEDED(hr = pDVDI->GetDVDDirectory(buff, countof(buff), &len)))
		pODD->title = CString(CStringW(buff));

	// TODO: resetdvd

	pDVDC->SetOption(DVD_ResetOnStop, FALSE);
	pDVDC->SetOption(DVD_HMSF_TimeCodeEvents, TRUE);

	if(s.idMenuLang) pDVDC->SelectDefaultMenuLanguage(s.idMenuLang);
	if(s.idAudioLang) pDVDC->SelectDefaultAudioLanguage(s.idAudioLang, DVD_AUD_EXT_NotSpecified);
	if(s.idSubtitlesLang) pDVDC->SelectDefaultSubpictureLanguage(s.idSubtitlesLang, DVD_SP_EXT_NotSpecified);

	m_iDVDDomain = DVD_DOMAIN_Stop;

	m_iPlaybackMode = PM_DVD;
}

void CMainFrame::OpenCapture(OpenDeviceData* pODD)
{
	CStringW vidfrname, audfrname;
	CComPtr<IBaseFilter> pVidCapTmp, pAudCapTmp;

	m_VidDispName = pODD->DisplayName[0];

	if(!m_VidDispName.IsEmpty())
	{
		if(!CreateFilter(m_VidDispName, &pVidCapTmp, vidfrname))
			throw _T("Can't create video capture filter");
	}

	m_AudDispName = pODD->DisplayName[1];

	if(!m_AudDispName.IsEmpty())
	{
		if(!CreateFilter(m_AudDispName, &pAudCapTmp, audfrname))
			throw _T("Can't create video capture filter");
	}

	if(!pVidCapTmp && !pAudCapTmp)
	{
		throw _T("No capture filters");
	}

	pCGB = NULL;
	pVidCap = NULL;
	pAudCap = NULL;

	if(FAILED(pCGB.CoCreateInstance(CLSID_CaptureGraphBuilder2)))
	{
		throw _T("Can't create capture graph builder object");
	}

	HRESULT hr;

	pCGB->SetFiltergraph(pGB);

	if(pVidCapTmp)
	{
		if(FAILED(hr = pGB->AddFilter(pVidCapTmp, vidfrname)))
		{
			throw _T("Can't add video capture filter to the graph");
		}

		pVidCap = pVidCapTmp;

		if(!pAudCapTmp)
		{
			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap))
			&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));

			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Interleaved, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev))
			&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));

			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC))
			&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pVidCap, IID_IAMStreamConfig, (void **)&pAMASC)))
			{
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap"));
			}
			else
			{
				pAudCap = pVidCap;
			}
		}
		else
		{
			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCCap)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));

			if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMStreamConfig, (void **)&pAMVSCPrev)))
				TRACE(_T("Warning: No IAMStreamConfig interface for vidcap capture"));
		}

		if(FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMCrossbar, (void**)&pAMXBar)))
			TRACE(_T("Warning: No IAMCrossbar interface was found\n"));

		if(FAILED(pCGB->FindInterface(&LOOK_UPSTREAM_ONLY, NULL, pVidCap, IID_IAMTVTuner, (void**)&pAMTuner)))
			TRACE(_T("Warning: No IAMTVTuner interface was found\n"));
/*
		if(pAMVSCCap) 
		{
//DumpStreamConfig(_T("c:\\mpclog.txt"), pAMVSCCap);
            CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = pVidCap;
			if(!pAMXBar && pVfwCD)
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(viddispname, pAMVSCCap, pVfwCD);
			}
			else
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(viddispname, pAMVSCCap, pAMXBar, pAMTuner);
			}
		}
*/
		// TODO: init pAMXBar

		if(pAMTuner) // load saved channel
		{
			pAMTuner->put_CountryCode(AfxGetApp()->GetProfileInt(_T("Capture"), _T("Country"), 1));

			int vchannel = pODD->vchannel;
			if(vchannel < 0) vchannel = AfxGetApp()->GetProfileInt(_T("Capture\\") + CString(m_VidDispName), _T("Channel"), -1);
			if(vchannel >= 0)
			{
				OAFilterState fs = State_Stopped;
				pMC->GetState(0, &fs);
				if(fs == State_Running) pMC->Pause();
				pAMTuner->put_Channel(vchannel, AMTUNER_SUBCHAN_DEFAULT, AMTUNER_SUBCHAN_DEFAULT);
				if(fs == State_Running) pMC->Run();
			}
		}
	}

	if(pAudCapTmp)
	{
		if(FAILED(hr = pGB->AddFilter(pAudCapTmp, CStringW(audfrname))))
		{
			throw _T("Can't add audio capture filter to the graph");
		}

		pAudCap = pAudCapTmp;

		if(FAILED(pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC))
		&& FAILED(pCGB->FindInterface(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Audio, pAudCap, IID_IAMStreamConfig, (void **)&pAMASC)))
		{
			TRACE(_T("Warning: No IAMStreamConfig interface for vidcap"));
		}
/*
		CInterfaceArray<IAMAudioInputMixer> pAMAIM;

		BeginEnumPins(pAudCap, pEP, pPin)
		{
			PIN_DIRECTION dir;
			if(FAILED(pPin->QueryDirection(&dir)) || dir != PINDIR_INPUT)
				continue;

			if(CComQIPtr<IAMAudioInputMixer> pAIM = pPin) 
				pAMAIM.Add(pAIM);
		}
		EndEnumPins

		if(pAMASC)
		{
			m_wndCaptureBar.m_capdlg.SetupAudioControls(auddispname, pAMASC, pAMAIM);
		}
*/
	}

	if(!(pVidCap || pAudCap))
	{
		throw _T("Couldn't open any device");
	}

	pODD->title = _T("Live");
	
	m_iPlaybackMode = PM_CAPTURE;
}

void CMainFrame::OpenCustomizeGraph()
{
	if(m_iPlaybackMode == PM_CAPTURE)
		return;

	CleanGraph();

	if(m_iPlaybackMode == PM_FILE)
	{
		if(m_pCAP) {
			if(AfxGetAppSettings().fAutoloadSubtitles) {
				AddTextPassThruFilter();
			}
		}
	}

	if(m_iPlaybackMode == PM_DVD)
	{
		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(CComQIPtr<IDirectVobSub2> pDVS2 = pBF)
			{
//				pDVS2->AdviseSubClock(m_pSubClock = new CSubClock);
//				break;

				// TODO: test multiple dvobsub instances with one clock
				if(!m_pSubClock) m_pSubClock = new CSubClock;
				pDVS2->AdviseSubClock(m_pSubClock);
			}
		}
		EndEnumFilters
	}

	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(GetCLSID(pBF) == CLSID_OggSplitter)
		{
			if(CComQIPtr<IAMStreamSelect> pSS = pBF)
			{
				LCID idAudio = AfxGetAppSettings().idAudioLang;
				if(!idAudio) idAudio = GetUserDefaultLCID();
				LCID idSub = AfxGetAppSettings().idSubtitlesLang;
				if(!idSub) idSub = GetUserDefaultLCID();

				DWORD cnt = 0;
				pSS->Count(&cnt);
				for(DWORD i = 0; i < cnt; i++)
				{
					AM_MEDIA_TYPE* pmt = NULL;
					DWORD dwFlags = 0;
					LCID lcid = 0;
					DWORD dwGroup = 0;
					WCHAR* pszName = NULL;
					if(SUCCEEDED(pSS->Info((long)i, &pmt, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL)))
					{
						CStringW name(pszName), sound(L"Sound"), subtitle(L"Subtitle");

						if(idAudio != -1 && (idAudio&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
						&& name.GetLength() > sound.GetLength()
						&& !name.Left(sound.GetLength()).CompareNoCase(sound))
						{
							if(SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE)))
								idAudio = -1;
						}

						if(idSub != -1 && (idSub&0x3ff) == (lcid&0x3ff) // sublang seems to be zeroed out in ogm...
						&& name.GetLength() > subtitle.GetLength()
						&& !name.Left(subtitle.GetLength()).CompareNoCase(subtitle)
						&& name.Mid(subtitle.GetLength()).Trim().CompareNoCase(L"off"))
						{
							if(SUCCEEDED(pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE)))
								idSub = -1;
						}

						if(pmt) DeleteMediaType(pmt);
						if(pszName) CoTaskMemFree(pszName);
					}
				}
			}
		}
	}
	EndEnumFilters

	CleanGraph();
}

void CMainFrame::OpenSetupVideo()
{
	m_fAudioOnly = true;

	if (m_pMFVDC)		// EVR 
	{
		m_fAudioOnly = false;
	}
	else if(m_pCAP)
	{
		CSize vs = m_pCAP->GetVideoSize();
		m_fAudioOnly = (vs.cx <= 0 || vs.cy <= 0);
	}
	else
	{
		{
			long w = 0, h = 0;

			if(CComQIPtr<IBasicVideo> pBV = pGB)
			{
				pBV->GetVideoSize(&w, &h);
			}

			if(w > 0 && h > 0)
			{
				m_fAudioOnly = false;
			}
		}

		if(m_fAudioOnly)
		{
			BeginEnumFilters(pGB, pEF, pBF)
			{
				long w = 0, h = 0;

				if(CComQIPtr<IVideoWindow> pVW = pBF)
				{
					long lVisible;
					if(FAILED(pVW->get_Visible(&lVisible)))
						continue;

					pVW->get_Width(&w);
					pVW->get_Height(&h);
				}

				if(w > 0 && h > 0)
				{
					m_fAudioOnly = false;
					break;
				}
			}
			EndEnumFilters
		}
	}

	if(m_fShockwaveGraph)
	{
		m_fAudioOnly = false;
	}

	if(m_pCAP)
	{
		SetShaders();
	}
	// else
	{
		// TESTME

		pVW->put_Owner((OAHWND)m_wndView.m_hWnd);
		pVW->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS|WS_CLIPCHILDREN);
		pVW->put_MessageDrain((OAHWND)m_hWnd);

		for(CWnd* pWnd = m_wndView.GetWindow(GW_CHILD); pWnd; pWnd = pWnd->GetNextWindow())
			pWnd->EnableWindow(FALSE); // little trick to let WM_SETCURSOR thru
	}
}

void CMainFrame::OpenSetupAudio()
{
	pBA->put_Volume(m_wndToolBar.Volume);

	// FIXME
	int balance = AfxGetAppSettings().nBalance;
	int sign = balance>0?-1:1;
	balance = max(100-abs(balance), 1);
	balance = (int)((log10(1.0*balance)-2)*5000*sign);
	balance = max(min(balance, 10000), -10000);
	pBA->put_Balance(balance);
}
/*
void CMainFrame::OpenSetupToolBar()
{
//	m_wndToolBar.Volume = AfxGetAppSettings().nVolume;
//	SetBalance(AfxGetAppSettings().nBalance);
}
*/
void CMainFrame::OpenSetupCaptureBar()
{
	if(m_iPlaybackMode == PM_CAPTURE)
	{
		if(pVidCap && pAMVSCCap) 
		{
            CComQIPtr<IAMVfwCaptureDialogs> pVfwCD = pVidCap;

			if(!pAMXBar && pVfwCD)
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pVfwCD);
			}
			else
			{
				m_wndCaptureBar.m_capdlg.SetupVideoControls(m_VidDispName, pAMVSCCap, pAMXBar, pAMTuner);
			}
		}

		if(pAudCap && pAMASC)
		{
			CInterfaceArray<IAMAudioInputMixer> pAMAIM;

			BeginEnumPins(pAudCap, pEP, pPin)
			{
				if(CComQIPtr<IAMAudioInputMixer> pAIM = pPin) 
					pAMAIM.Add(pAIM);
			}
			EndEnumPins

			m_wndCaptureBar.m_capdlg.SetupAudioControls(m_AudDispName, pAMASC, pAMAIM);
		}
	}

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, false, 
		m_wndCaptureBar.m_capdlg.m_fAudPreview, false);
}

void CMainFrame::OpenSetupInfoBar()
{
	if(m_iPlaybackMode == PM_FILE)
	{
		bool fEmpty = true;
		BeginEnumFilters(pGB, pEF, pBF)
		{
			if(CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF)
			{
				CComBSTR bstr;
				if(SUCCEEDED(pAMMC->get_Title(&bstr))) {m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_TITLE), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_AuthorName(&bstr))) {m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUTHOR), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Copyright(&bstr))) {m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_COPYRIGHT), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Rating(&bstr))) {m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_RATING), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(SUCCEEDED(pAMMC->get_Description(&bstr))) {m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DESCRIPTION), bstr.m_str); if(bstr.Length()) fEmpty = false;}
				if(!fEmpty)
				{
					RecalcLayout();
					break;
				}
			}
		}
		EndEnumFilters
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		CString info('-');
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_DOMAIN), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_LOCATION), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_VIDEO), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_AUDIO), info);
		m_wndInfoBar.SetLine(ResStr(IDS_INFOBAR_SUBTITLES), info);
		RecalcLayout();
	}
}

void CMainFrame::OpenSetupStatsBar()
{
	CString info('-');

	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(!pQP && (pQP = pBF))
		{
			m_wndStatsBar.SetLine(_T("Frame-rate"), info);
			m_wndStatsBar.SetLine(_T("Sync Offset"), info);
			m_wndStatsBar.SetLine(_T("Frames"), info);
			m_wndStatsBar.SetLine(_T("Jitter"), info);
			m_wndStatsBar.SetLine(_T("Buffers"), info);
			m_wndStatsBar.SetLine(_T("Bitrate"), info);
			RecalcLayout();
		}

		if(!pBI && (pBI = pBF))
		{
			m_wndStatsBar.SetLine(_T("Buffers"), info);
			m_wndStatsBar.SetLine(_T("Bitrate"), info); // FIXME: shouldn't be here
			RecalcLayout();
		}
	}
	EndEnumFilters
}

void CMainFrame::OpenSetupStatusBar()
{
	m_wndStatusBar.ShowTimer(true);

	//

	if(!m_fCustomGraph)
	{
		UINT id = IDB_NOAUDIO;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			CComQIPtr<IBasicAudio> pBA = pBF;
			if(!pBA) continue;

			BeginEnumPins(pBF, pEP, pPin)
			{
				if(S_OK == pGB->IsPinDirection(pPin, PINDIR_INPUT) 
				&& S_OK == pGB->IsPinConnected(pPin))
				{
					AM_MEDIA_TYPE mt;
					memset(&mt, 0, sizeof(mt));
					pPin->ConnectionMediaType(&mt);

					if(mt.majortype == MEDIATYPE_Audio && mt.formattype == FORMAT_WaveFormatEx)
					{
						switch(((WAVEFORMATEX*)mt.pbFormat)->nChannels)
						{
						case 1: id = IDB_MONO; break;
						case 2: default: id = IDB_STEREO; break;
						}
						break;
					}
					else if(mt.majortype == MEDIATYPE_Midi)
					{
						id = NULL;
						break;
					}
				}
			}
			EndEnumPins

			if(id != IDB_NOAUDIO)
			{
				break;
			}
		}
		EndEnumFilters

		m_wndStatusBar.SetStatusBitmap(id);
	}

	//

	HICON hIcon = NULL;

	if(m_iPlaybackMode == PM_FILE)
	{
		CString fn = m_wndPlaylistBar.GetCur();
		CString ext = fn.Mid(fn.ReverseFind('.')+1);
		hIcon = LoadIcon(ext, true);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		hIcon = LoadIcon(_T(".ifo"), true);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		// hIcon = ; // TODO
	}

	m_wndStatusBar.SetStatusTypeIcon(hIcon);
}

void CMainFrame::OpenSetupWindowTitle(CString fn)
{
	CString title(MAKEINTRESOURCE(IDR_MAINFRAME));

	AppSettings& s = AfxGetAppSettings();

	int i = s.iTitleBarTextStyle;

	if(!fn.IsEmpty() && (i == 0 || i == 1))
	{
		if(i == 1)
		{
			if(m_iPlaybackMode == PM_FILE)
			{
				fn.Replace('\\', '/');
				CString fn2 = fn.Mid(fn.ReverseFind('/')+1);
				if(!fn2.IsEmpty()) fn = fn2;

				if(s.fTitleBarTextTitle)
				{
					BeginEnumFilters(pGB, pEF, pBF)
					{
						if(CComQIPtr<IAMMediaContent, &IID_IAMMediaContent> pAMMC = pBF)
						{
							CComBSTR bstr;
							if(SUCCEEDED(pAMMC->get_Title(&bstr)) && bstr.Length())
							{
								fn = CString(bstr.m_str);
								break;
							}
						}
					}
					EndEnumFilters
				}
			}
			else if(m_iPlaybackMode == PM_DVD)
			{
				fn = _T("DVD");
			}
			else if(m_iPlaybackMode == PM_CAPTURE)
			{
				fn = _T("Live");
			}
		}

		title = fn + _T(" - ") + title;
	}
	CString szBuild;
	szBuild.Format(_T(" (Build %s)"),SVP_REV_STR);
	title += szBuild;
	//SetWindowText(title);
	m_szTitle = title;
	RedrawNonClientArea();
}

void CMainFrame::OnColorControl(UINT nID){
	int act = nID - ID_BRIGHTINC;
	if(m_pMC){
		AppSettings& s = AfxGetAppSettings();
		VMR9ProcAmpControlRange ClrRange;
		ClrRange.dwProperty = ProcAmpControl9_Brightness;
		ClrRange.dwSize = sizeof(VMR9ProcAmpControlRange);
		m_pMC->GetProcAmpControlRange(0, &ClrRange);

		if(act == 2){
			s.dBrightness = ClrRange.DefaultValue;
		}else if(act == 1){
			s.dBrightness -= 1; 
		}else{
			s.dBrightness += 1; 
		}
		s.dBrightness = min( max(s.dBrightness, ClrRange.MinValue) , ClrRange.MaxValue);
		SetVMR9ColorControl(s.dBrightness,s.dContrast,s.dHue,s.dSaturation);
	}else{
		SendStatusMessage(_T("您需要在选项面板中启用内置亮度\\色彩控制器才能控制亮度") , 5000);
	}
}
void CMainFrame::OnColorControlButtonReset(){
	m_wndColorControlBar.OnButtonReset();
}
void CMainFrame::ReRenderOrLoadMedia(){
	int iPlaybackMode = m_iPlaybackMode;
	int iSpeed = m_iSpeedLevel;
	CloseMedia();
	m_wndColorControlBar.CheckAbility();
	if(iPlaybackMode == PM_FILE)
	{
		OpenCurPlaylistItem();
	}else if(iPlaybackMode == PM_DVD)
	{
		OnFileOpendvd();
	}
	m_iSpeedLevel = iSpeed;
	OnPlayChangeRate(0);
}
void CMainFrame::OnEnableDX9(){
	AppSettings& s = AfxGetAppSettings();
		s.iDSVideoRendererType = 6;
		s.iRMVideoRendererType = 2;
		s.iQTVideoRendererType = 2;

		s.iAPSurfaceUsage = VIDRNDT_AP_TEXTURE3D;
	
	if( m_iMediaLoadState != MLS_CLOSED){
		ReRenderOrLoadMedia();
	}
}
void CMainFrame::OnColorControlButtonEnable(){
	AppSettings& s = AfxGetAppSettings();
	s.fVMR9MixerMode = !s.fVMR9MixerMode;
	if(s.fVMR9MixerMode){
		s.iDSVideoRendererType = 6;
		s.iRMVideoRendererType = 2;
		s.iQTVideoRendererType = 2;
	}

	if( m_iMediaLoadState != MLS_CLOSED){
		ReRenderOrLoadMedia();
		
	}else{
		m_wndColorControlBar.CheckAbility();
	}
}
void CMainFrame::OnColorControlUpdateButtonReset(CCmdUI* pCmdUI)
{
	pCmdUI->Enable(!!m_pMC);
}

void CMainFrame::OnColorControlUpdateButtonEnable(CCmdUI* pCmdUI)
{
	AppSettings& s  = AfxGetAppSettings();
	bool bEnable = (s.iDSVideoRendererType != VIDRNDT_DS_VMR9RENDERLESS) || !s.fVMR9MixerMode;
	if( GetMediaState() == State_Running ){
		bEnable = !m_pMC;
	}
	//pCmdUI->Enable(bEnable);
	if(bEnable)
		pCmdUI->SetText(_T("启用"));
	else
		pCmdUI->SetText(_T("禁用"));
	
}

void CMainFrame::SetVMR9ColorControl(float dBrightness, float dContrast, float dHue, float dSaturation)
{
	VMR9ProcAmpControl		ClrControl;

	if(m_pMC ) // Fuck fVMR9MixerYUV && !AfxGetAppSettings().fVMR9MixerYUV
	{
		
		ClrControl.dwSize		= sizeof(ClrControl);
		ClrControl.dwFlags		= ProcAmpControl9_Mask;
		ClrControl.Brightness	= dBrightness;
		ClrControl.Contrast		= dContrast;
		ClrControl.Hue			= 0;
		ClrControl.Saturation	= 1;

		m_pMC->SetProcAmpControl (0, &ClrControl);

		CString szMsg;
		szMsg.Format(_T("亮度: %0.2f  对比度: %0.2f "),dBrightness,dContrast);
		SendStatusMessage( szMsg , 3000);
	}
}
bool CMainFrame::OpenMediaPrivate(CAutoPtr<OpenMediaData> pOMD)
{
	AppSettings& s = AfxGetAppSettings();
	if(m_iMediaLoadState != MLS_CLOSED && m_iMediaLoadState != MLS_LOADING)
	{
		ASSERT(0);
		return(false);
	}

	s.szFGMLog.Empty();
	s.SetChannelMapByNumberOfSpeakers(s.iDecSpeakers , -1);

	m_iMediaLoadState = MLS_LOADING;

	// FIXME: Don't show "Closed" initially
	PostMessage(WM_KICKIDLE);

	CString err, aborted(_T("Aborted"));

	m_fUpdateInfoBar = false;

	try
	{
		if(m_fOpeningAborted) throw aborted;

		if(OpenFileData* pOFD = dynamic_cast<OpenFileData*>(pOMD.m_p))
		{
			if(pOFD->fns.IsEmpty()) throw _T("没有找到文件");

			CString fn = pOFD->fns.GetHead();

			int i = fn.Find(_T(":\\"));
			if(i > 0)
			{
				CString drive = fn.Left(i+2);
				UINT type = GetDriveType(drive);
				CAtlList<CString> sl;
				if(type == DRIVE_REMOVABLE || type == DRIVE_CDROM && GetCDROMType(drive[0], sl) != CDROM_Audio)
				{
					int ret = IDRETRY;
					while(ret == IDRETRY)
					{
						WIN32_FIND_DATA findFileData;
						HANDLE h = FindFirstFile(fn, &findFileData);
						if(h != INVALID_HANDLE_VALUE)
						{
							FindClose(h);
							ret = IDOK;
						}
						else
						{
							CString msg;
							msg.Format(_T("%s was not found, please insert media containing this file."), fn);
							ret = AfxMessageBox(msg, MB_RETRYCANCEL);
						}
					}

					if(ret != IDOK) throw aborted;
				}else{
					CSVPToolBox svpTool;
					if(!svpTool.ifFileExist(fn, true)){
						throw _T("文件不存在");
					}
				}
			}
		}

		if(m_fOpeningAborted) throw aborted;

		OpenCreateGraphObject(pOMD);
		
		if(m_fOpeningAborted) throw aborted;

		SetupIViAudReg();

		if(m_fOpeningAborted) throw aborted;

		m_pCAP2 = NULL;
		m_pCAP = NULL;

		if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p)) OpenFile(p);
		else if(OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD.m_p)) OpenDVD(p);
		else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p)) OpenCapture(p);
		else throw _T("Can't open, invalid input parameters");

		

		pGB->FindInterface(__uuidof(ISubPicAllocatorPresenter), (void**)&m_pCAP, TRUE);
		pGB->FindInterface(__uuidof(ISubPicAllocatorPresenter2), (void**)&m_pCAP2, TRUE);
		pGB->FindInterface(__uuidof(IVMRMixerControl9),			(void**)&m_pMC,  TRUE);
		if (m_pMC)
		{
			SetVMR9ColorControl(s.dBrightness, s.dContrast, s.dHue, s.dSaturation);
		}
		// === EVR !
		pGB->FindInterface(__uuidof(IMFVideoDisplayControl), (void**)&m_pMFVDC,  TRUE);
		if (m_pMFVDC)
		{
			RECT		Rect;
			::GetClientRect (m_wndView.m_hWnd, &Rect);
			m_pMFVDC->SetVideoWindow (m_wndView.m_hWnd);
			m_pMFVDC->SetVideoPosition(NULL, &Rect);
		}

		if(m_fOpeningAborted) throw aborted;

		OpenCustomizeGraph();

		if(m_fOpeningAborted) throw aborted;

		OpenSetupVideo();

		if(m_fOpeningAborted) throw aborted;

		OpenSetupAudio();

		if(m_fOpeningAborted) throw aborted;

		if(m_pCAP && (!m_fAudioOnly || m_fRealMediaGraph))
		{
			POSITION pos = pOMD->subs.GetHeadPosition();
			while(pos){ LoadSubtitle(pOMD->subs.GetNext(pos));}
			if(m_pSubStreams.GetCount() == 0){
				if ( !m_wndPlaylistBar.m_pl.szPlayListSub.IsEmpty() ){
					int delayms = 0 - m_wndPlaylistBar.GetTotalTimeBeforeCur();
					LoadSubtitle(m_wndPlaylistBar.m_pl.szPlayListSub, delayms , true); 
				}
			}

			if(s.fEnableSubtitles && m_pSubStreams.GetCount() > 0){
				
				SetSubtitle(m_pSubStreams.GetHead());
				if(s.fAutoloadSubtitles2 && m_pSubStreams2.GetCount() > 1 ){
					BOOL HavSubs = false;
					POSITION pos = m_pSubStreams2.GetHeadPosition();
					while(pos){
						CComPtr<ISubStream> pSubStream = m_pSubStreams2.GetNext(pos);

						if(!pSubStream) continue;

						for(int i = 0, j = pSubStream->GetStreamCount(); i < j; i++)
						{
							WCHAR* pName = NULL;
							if(SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL)))
							{
								CString name(pName);
								if( name.Find(_T("en")) >= 0){
									SetSubtitle2( pSubStream);
									HavSubs = true;
								}
								CoTaskMemFree(pName);
							}
							
						}
					}
					if(!HavSubs){
						pos = m_pSubStreams2.GetHeadPosition();
						m_pSubStreams2.GetNext(pos);
						if(pos)
							SetSubtitle2( m_pSubStreams2.GetNext(pos));
					}
					
				}
			}

			
		}

		if(m_fOpeningAborted) throw aborted;

		OpenSetupWindowTitle(pOMD->title);

		if(::GetCurrentThreadId() == AfxGetApp()->m_nThreadID)
		{
			OnFilePostOpenmedia();
		}
		else
		{
			PostMessage(WM_COMMAND, ID_FILE_POST_OPENMEDIA);
		}

		while(m_iMediaLoadState != MLS_LOADED 
			&& m_iMediaLoadState != MLS_CLOSING // FIXME
			)
		{
			Sleep(50);
		}

		// PostMessage instead of SendMessage because the user might call CloseMedia and then we would deadlock
		time(&m_tPlayStartTime);

		PostMessage(WM_COMMAND, ID_PLAY_PAUSE);

		if(!(AfxGetAppSettings().nCLSwitches&CLSW_OPEN))
			PostMessage(WM_COMMAND, ID_PLAY_PLAY);

		AfxGetAppSettings().nCLSwitches &= ~CLSW_OPEN;

		if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p))
		{
			if(p->rtStart > 0)
				PostMessage(WM_RESUMEFROMSTATE, (WPARAM)PM_FILE, (LPARAM)(p->rtStart/10000)); // REFERENCE_TIME doesn't fit in LPARAM under a 32bit env.
		}
		else if(OpenDVDData* p = dynamic_cast<OpenDVDData*>(pOMD.m_p))
		{
			if(p->pDvdState)
				PostMessage(WM_RESUMEFROMSTATE, (WPARAM)PM_DVD, (LPARAM)(CComPtr<IDvdState>(p->pDvdState).Detach())); // must be released by the called message handler
		}
		else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p))
		{
			m_wndCaptureBar.m_capdlg.SetVideoInput(p->vinput);
			m_wndCaptureBar.m_capdlg.SetVideoChannel(p->vchannel);
			m_wndCaptureBar.m_capdlg.SetAudioInput(p->ainput);
		}
	}
	catch(LPCTSTR msg)
	{
		err = msg;
	}
	catch(CString msg)
	{
		err = msg;
	}

	if(!err.IsEmpty())
	{
		SendStatusMessage(err, 3000);
		CloseMediaPrivate();
		m_closingmsg = err;

		OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p);
		if(p && err != aborted)
		{
			m_wndPlaylistBar.SetCurValid(false);
			if(m_wndPlaylistBar.GetCount() > 1)
			{
				CPlaylistItem pli[2];
				m_wndPlaylistBar.GetCur(pli[0]);
				m_wndPlaylistBar.SetNext();
				m_wndPlaylistBar.GetCur(pli[1]);
				if(pli[0].m_id != pli[1].m_id)
				{
					//CAutoPtr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD());
					//if(p) OpenMediaPrivate(p);
				}
			}
		}
	}
	else
	{
		m_wndPlaylistBar.SetCurValid(true);
	}

	PostMessage(WM_KICKIDLE); // calls main thread to update things

	return(err.IsEmpty());
}

void CMainFrame::CloseMediaPrivate()
{
	m_iMediaLoadState = MLS_CLOSING;

    OnPlayStop(); // SendMessage(WM_COMMAND, ID_PLAY_STOP);

	m_iPlaybackMode = PM_NONE;
	m_iSpeedLevel = 0;

	m_fLiveWM = false;

	m_fEndOfStream = false;

	m_rtDurationOverride = -1;

	m_kfs.RemoveAll();

	m_pCB = NULL;

//	if(pVW) pVW->put_Visible(OAFALSE);
//	if(pVW) pVW->put_MessageDrain((OAHWND)NULL), pVW->put_Owner((OAHWND)NULL);

	m_pCAP = NULL; // IMPORTANT: IVMRSurfaceAllocatorNotify/IVMRSurfaceAllocatorNotify9 has to be released before the VMR/VMR9, otherwise it will crash in Release()
	m_pCAP2  = NULL;
	m_pMC	 = NULL;
	m_pMFVDC = NULL;

	pAMXBar.Release(); pAMTuner.Release(); pAMDF.Release();
	pAMVCCap.Release(); pAMVCPrev.Release(); pAMVSCCap.Release(); pAMVSCPrev.Release(); pAMASC.Release();
	pVidCap.Release(); pAudCap.Release();
	pCGB.Release();
	pDVDC.Release(); pDVDI.Release();
	pQP.Release(); pBI.Release(); pAMOP.Release(); pFS.Release();
	pMC.Release(); pME.Release(); pMS.Release();
	pVW.Release(); pBV.Release();
	pBA.Release();

	if(pGB) pGB->RemoveFromROT();
	pGB.Release();

	m_fRealMediaGraph = m_fShockwaveGraph = m_fQuicktimeGraph = false;

	m_pSubClock = NULL;

	m_pProv.Release();

	{
		CAutoLock cAutoLock(&m_csSubLock);
		m_pSubStreams.RemoveAll();

		CAutoLock cAutoLock2(&m_csSubLock2);
		m_pSubStreams2.RemoveAll();
	}

	m_VidDispName.Empty();
	m_AudDispName.Empty();

	m_closingmsg = ResStr(IDS_CONTROLS_CLOSED);

	AfxGetAppSettings().nCLSwitches &= CLSW_OPEN|CLSW_PLAY|CLSW_AFTERPLAYBACK_MASK|CLSW_NOFOCUS;

	m_iMediaLoadState = MLS_CLOSED;
}

// msn

void CMainFrame::SendNowPlayingToMSN()
{
	if(!AfxGetAppSettings().fNotifyMSN)
		return;

	CString title, author;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_TITLE), title);
		m_wndInfoBar.GetLine(ResStr(IDS_INFOBAR_AUTHOR), author);

		if(title.IsEmpty())
		{
			CPlaylistItem pli;
			m_wndPlaylistBar.GetCur(pli);

			if(!pli.m_fns.IsEmpty())
			{
				CString label = !pli.m_label.IsEmpty() ? pli.m_label : pli.m_fns.GetHead();

				if(m_iPlaybackMode == PM_FILE)
				{
					CString fn = label;
					if(fn.Find(_T("://")) >= 0) {int i = fn.Find('?'); if(i >= 0) fn = fn.Left(i);}
					CPath path(fn);
					path.StripPath();
					path.MakePretty();
					path.RemoveExtension();
					title = (LPCTSTR)path;
					author.Empty();
				}
				else if(m_iPlaybackMode == PM_CAPTURE)
				{
					title = label != pli.m_fns.GetHead() ? label : _T("Live");
					author.Empty();
				}
				else if(m_iPlaybackMode == PM_DVD)
				{
					title = _T("DVD");
					author.Empty();
				}
			}
		}
	}

	CStringW buff;
	buff += L"\\0Music\\0";
	buff += title.IsEmpty() ? L"0" : L"1";
	buff += L"\\0";
	buff += author.IsEmpty() ? L"{0}" : L"{0} - {1}";
	buff += L"\\0";
	if(!author.IsEmpty()) {buff += CStringW(author) + L"\\0";}
	buff += CStringW(title) + L"\\0";
	buff += L"\\0\\0";

	COPYDATASTRUCT data;
    data.dwData = 0x0547;
    data.lpData = (PVOID)(LPCWSTR)buff;
    data.cbData = buff.GetLength() * 2 + 2;

	HWND hWnd = NULL;
	while(hWnd = ::FindWindowEx(NULL, hWnd, _T("MsnMsgrUIManager"), NULL))
		::SendMessage(hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&data);
}

// mIRC

void CMainFrame::SendNowPlayingTomIRC()
{
	if(!AfxGetAppSettings().fNotifyGTSdll)
		return;

	for(int i = 0; i < 20; i++)
	{
		HANDLE hFMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 1024, _T("mIRC"));
		if(!hFMap) return;

		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hFMap);
			Sleep(50);
			continue;
		}

		if(LPVOID lpMappingAddress = MapViewOfFile(hFMap, FILE_MAP_WRITE, 0, 0, 0))
		{
			LPCSTR cmd = m_fAudioOnly ? "/.timerAUDGTS 1 5 mpcaud" : "/.timerVIDGTS 1 5 mpcvid";
			strcpy((char*)lpMappingAddress, cmd);

			if(HWND hWnd = ::FindWindow(_T("mIRC"), NULL))
				::SendMessage(hWnd, (WM_USER + 200), (WPARAM)1, (LPARAM)0);

			UnmapViewOfFile(lpMappingAddress);
		}
		
		CloseHandle(hFMap);

		break;
	}
}

// dynamic menus

void CMainFrame::SetupOpenCDSubMenu()
{
	CMenu* pSub = &m_opencds;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState == MLS_LOADING) return;

	if(AfxGetAppSettings().fHideCDROMsSubMenu) return;

	UINT id = ID_FILE_OPEN_CD_START;

	for(TCHAR drive = 'C'; drive <= 'Z'; drive++)
	{
		CString label = GetDriveLabel(drive), str;

		
		CAtlList<CString> files;
		switch(GetCDROMType(drive, files, true))
		{
		case CDROM_Unknown:
			if(label.IsEmpty()) label = _T("Disk ");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		case CDROM_Audio:
			if(label.IsEmpty()) label = _T("Audio CD");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		case CDROM_VideoCD:
			if(label.IsEmpty()) label = _T("(S)VCD");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		case CDROM_DVDVideo:
			if(label.IsEmpty()) label = _T("DVD Video");
			str.Format(_T("%s (%c:)"), label, drive);
			break;
		default:
			break;
		}

		if(!str.IsEmpty())
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, str);
	}
}
void CMainFrame::OnUpdateAudioDeviceChange(CCmdUI *pCmdUI){
	AppSettings& s = AfxGetAppSettings();
	if( IDS_CHANGE_AUDIO_DEVICE == pCmdUI->m_nID ){
		BOOL bNotDefault = false;
		if(!s.AudioRendererDisplayName.IsEmpty()){
			for(int i = 0; i < m_AudioDevice.GetCount() ; i+=2){
				if( s.AudioRendererDisplayName == m_AudioDevice.GetAt(i + 1) ){
					bNotDefault = true;
					break;
				}
			}
			if(!bNotDefault){s.AudioRendererDisplayName = _T("");}
		}
		
		pCmdUI->SetRadio(!bNotDefault);
	}else{
		if( m_AudioDevice.GetAt( ( pCmdUI->m_nID - IDS_CHANGE_AUDIO_DEVICE -1 ) * 2 + 1 ) == s.AudioRendererDisplayName ){
			pCmdUI->SetRadio(TRUE);
		}else{
			pCmdUI->SetRadio(FALSE);
		}
	}
}
void CMainFrame::OnAudioDeviceChange(UINT nID){
	AppSettings& s = AfxGetAppSettings();

	if(nID == IDS_CHANGE_AUDIO_DEVICE)
		s.AudioRendererDisplayName = _T("");
	else
		s.AudioRendererDisplayName = m_AudioDevice.GetAt((nID - IDS_CHANGE_AUDIO_DEVICE - 1) * 2 + 1);

	ReRenderOrLoadMedia();
}
void CMainFrame::SetupAudioDeviceSubMenu(){
	CMenu* pSub = &m_audiodevices;
	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	UINT idstart = IDS_CHANGE_AUDIO_DEVICE;
	//m_AudioDevice.RemoveAll();
	if(m_AudioDevice.GetCount() <= 0 ){
		BeginEnumSysDev(CLSID_AudioRendererCategory, pMoniker)
		{
			LPOLESTR olestr = NULL;
			if(FAILED(pMoniker->GetDisplayName(0, 0, &olestr)))
				continue;

			CStringW str(olestr);
			CoTaskMemFree(olestr);

			

			CComPtr<IPropertyBag> pPB;
			if(SUCCEEDED(pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&pPB)))
			{
				CComVariant var;
				pPB->Read(CComBSTR(_T("FriendlyName")), &var, NULL);

				CString fstr(var.bstrVal);

				m_AudioDevice.Add(fstr);
				m_AudioDevice.Add(CString(str));
			}
		}
		EndEnumSysDev
	}
	
	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, idstart++, _T("系统默认"));
	if(m_AudioDevice.GetCount())
		pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

	for(int i = 0 ; i < m_AudioDevice.GetCount(); i+=2){
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, idstart++, m_AudioDevice.GetAt(i));
		if(idstart > IDS_CHANGE_AUDIO_DEVICE_END){
			//too many device...
			break;
		}
	}
}


void CMainFrame::SetupFiltersSubMenu()
{
	CMenu* pSub = &m_filters;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	m_filterpopups.RemoveAll();

	m_pparray.RemoveAll();
	m_ssarray.RemoveAll();

	if(m_iMediaLoadState == MLS_LOADED)
	{
		UINT idf = 0;
		UINT ids = ID_FILTERS_SUBITEM_START;
		UINT idl = ID_FILTERSTREAMS_SUBITEM_START;

		BeginEnumFilters(pGB, pEF, pBF)
		{
			CString name(GetFilterName(pBF));
			if(name.GetLength() >= 43) name = name.Left(40) + _T("...");

			CLSID clsid = GetCLSID(pBF);
			if(clsid == CLSID_AVIDec)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					DWORD c = ((VIDEOINFOHEADER*)mt.pbFormat)->bmiHeader.biCompression;
					switch(c)
					{
					case BI_RGB: name += _T(" (RGB)"); break;
					case BI_RLE4: name += _T(" (RLE4)"); break;
					case BI_RLE8: name += _T(" (RLE8)"); break;
					case BI_BITFIELDS: name += _T(" (BITF)"); break;
					default: name.Format(_T("%s (%c%c%c%c)"), 
								 CString(name), (TCHAR)((c>>0)&0xff), (TCHAR)((c>>8)&0xff), (TCHAR)((c>>16)&0xff), (TCHAR)((c>>24)&0xff)); break;
					}
				}
			}
			else if(clsid == CLSID_ACMWrapper)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					WORD c = ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag;
					name.Format(_T("%s (0x%04x)"), CString(name), (int)c);
				}
			}
			else if(clsid == __uuidof(CTextPassThruFilter) || clsid == __uuidof(CNullTextRenderer)
				|| clsid == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) // ISCR
			{
				// hide these
				continue;
			}

			CAutoPtr<CMenu> pSubSub(new CMenu);
			pSubSub->CreatePopupMenu();

			int nPPages = 0;

			CComQIPtr<ISpecifyPropertyPages> pSPP = pBF;

			/*			if(pSPP)
			{
			CAUUID caGUID;
			caGUID.pElems = NULL;
			if(SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0)
			{
			*/					m_pparray.Add(pBF);
			pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids, _T("&Properties..."));
			/*
			if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);
			*/
			nPPages++;
			/*				}
			}
			*/
			BeginEnumPins(pBF, pEP, pPin)
			{
				CString name = GetPinName(pPin);
				name.Replace(_T("&"), _T("&&"));

				if(pSPP = pPin)
				{
					CAUUID caGUID;
					caGUID.pElems = NULL;
					if(SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0)
					{
						m_pparray.Add(pPin);
						pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids+nPPages, name + _T(" (pin) properties..."));

						if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);

						nPPages++;
					}
				}
			}
			EndEnumPins

				CComQIPtr<IAMStreamSelect> pSS = pBF;
			if(pSS)
			{
				DWORD nStreams = 0, flags, group, prevgroup = -1;
				LCID lcid;
				WCHAR* wname = NULL;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);

				if(nStreams > 0 && nPPages > 0) pSubSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

				UINT idlstart = idl;

				for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
				{
					m_ssarray.Add(pSS);

					flags = group = 0;
					wname = NULL;
					pSS->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);

					if(group != prevgroup && idl > idlstart)
						pSubSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);
					prevgroup = group;

					if(flags & AMSTREAMSELECTINFO_EXCLUSIVE)
					{
					}
					else if(flags & AMSTREAMSELECTINFO_ENABLED)
					{
					}

					if(!wname) 
					{
						CStringW stream(L"Unknown Stream");
						wname = (WCHAR*)CoTaskMemAlloc((stream.GetLength()+3+1)*sizeof(WCHAR));
						swprintf(wname, L"%s %d", stream, min(i+1,999));
					}

					CString name(wname);
					name.Replace(_T("&"), _T("&&"));

					pSubSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, idl++, name);

					CoTaskMemFree(wname);
				}

				if(nStreams == 0) pSS.Release();
			}

			if(nPPages == 1 && !pSS)
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ids, name);
			}
			else 
			{
				pSub->AppendMenu(MF_BYPOSITION|MF_STRING|MF_DISABLED|MF_GRAYED, idf, name);

				if(nPPages > 0 || pSS)
				{
					MENUITEMINFO mii;
					mii.cbSize = sizeof(mii);
					mii.fMask = MIIM_STATE|MIIM_SUBMENU;
					mii.fType = MF_POPUP;
					mii.hSubMenu = pSubSub->m_hMenu;
					mii.fState = (pSPP || pSS) ? MF_ENABLED : (MF_DISABLED|MF_GRAYED);
					pSub->SetMenuItemInfo(idf, &mii, TRUE);

					m_filterpopups.Add(pSubSub);
				}
			}

			ids += nPPages;
			idf++;
		}
		EndEnumFilters
	}
}
void CMainFrame::SetupAudioSwitcherSubMenu()
{
	CMenu* pSub = &m_audios;
	UINT idl = ID_FILTERSTREAMS_SUBITEM_START;
	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	SetupAudioDeviceSubMenu();

	m_ssarray.RemoveAll();
	CMenu* pSubMenu = &m_audiodevices;
	if(pSubMenu ){
		pSub->AppendMenu(MF_POPUP, (UINT_PTR) pSubMenu->m_hMenu, _T("输出设备"));
		
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_USINGSPDIF, _T("使用数字输出(SPDIF/光纤)"));
	}

	if(m_iMediaLoadState == MLS_LOADED)
	{
		UINT id = ID_AUDIO_SUBITEM_START;

		CComQIPtr<IAMStreamSelect> pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
		if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

		if(pSS)
		{
			if(pSubMenu )
				pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, IDS_AUDIOCHANNALMAPNORMAL, _T("系统默认"));
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, IDS_AUDIOCHANNALMAPLEFT,  _T("只播放左声道"));
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, IDS_AUDIOCHANNALMAPRIGHT,  _T("只播放右声道"));
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, IDS_AUDIOCHANNALMAPCENTER,  _T("只播放中置声道"));
			pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

			DWORD cStreams = 0;
			if(SUCCEEDED(pSS->Count(&cStreams)) && cStreams > 0)
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, L"设置...");
				pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

				for(int i = 0; i < (int)cStreams; i++)
				{
					WCHAR* pName = NULL;
					if(FAILED(pSS->Info(i, NULL, NULL, NULL, NULL, &pName, NULL, NULL)))
						break;

					CString name(pName);
					name.Replace(_T("&"), _T("&&"));

					pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, name);

					CoTaskMemFree(pName);
				}
			}
			pSub->AppendMenu(MF_SEPARATOR|MF_ENABLED);

			BeginEnumFilters(pGB, pEF, pBF)
			{
				
				CLSID clsid = GetCLSID(pBF);
				
				if(clsid == CLSID_AVIDec)
				{
					CComPtr<IPin> pPin = GetFirstPin(pBF);
				}
				else if(clsid == CLSID_ACMWrapper)
				{
					CComPtr<IPin> pPin = GetFirstPin(pBF);
				
				}
				else if(clsid == __uuidof(CTextPassThruFilter) || clsid == __uuidof(CNullTextRenderer)
					|| clsid == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) // ISCR
				{
					// hide these
					continue;
				}
				BeginEnumPins(pBF, pEP, pPin)
				{
					
				}
				EndEnumPins
				

				CComQIPtr<IAMStreamSelect> pSS2 = pBF;
				if(pSS2)
				{
					DWORD nStreams = 0, flags, group, prevgroup = -1;
					LCID lcid;
					WCHAR* wname = NULL;
					CComPtr<IUnknown> pObj, pUnk;

					pSS2->Count(&nStreams);

					


					for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
					{
						m_ssarray.Add(pSS2);

						
						
						flags = group = 0;
						wname = NULL;
						pSS2->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);


						if(!wname) 
						{
							CStringW stream(L"Unknown Stream");
							wname = (WCHAR*)CoTaskMemAlloc((stream.GetLength()+3+1)*sizeof(WCHAR));
							swprintf(wname, L"%s %d", stream, min(i+1,999));
						}

						CString name(wname);
						name.Replace(_T("&"), _T("&&"));
						if(name.Find(_T("Audio")) == 0 || name.Find(_T("声")) == 0 || name.Find(_T("音")) == 0 ){
							CString szLog;
							szLog.Format(L" Audio Menu %d %s", idl, name);
							SVP_LogMsg(szLog);
							pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, idl, name) ;
							
						}
						idl++;

						CoTaskMemFree(wname);
					}

					if(nStreams == 0) pSS2.Release();
				}

				

			}
			EndEnumFilters
		}
	}
}
void CMainFrame::OnAudioChannalMapMenu(UINT nID){
	CComQIPtr<IAudioSwitcherFilter>  pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
	if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);

	if(pSS)
	{
		m_iAudioChannelMaping = nID - IDS_AUDIOCHANNALMAPNORMAL;
		DWORD pSpeakerToChannelMap[18][18]; //Meaning [Total Channel Number] [Speaker] = 1 << Channel
		memset(pSpeakerToChannelMap, 0, sizeof(pSpeakerToChannelMap));
		BOOL bCustomChannelMapping = FALSE;
		UINT iMapedChannel = 0;
		switch(m_iAudioChannelMaping ){
			case 0:
				bCustomChannelMapping = FALSE;
				break;
			case 1:
				bCustomChannelMapping = TRUE;
				iMapedChannel = 0;
				break;
			case 2:		
				iMapedChannel = 1;
				bCustomChannelMapping = TRUE;
			default:
				break;
		}
		if(bCustomChannelMapping){
			int iCurChs = pSS->GetNumberOfInputChannels() - 1;
			for(int i = 0; i < 18; i++){
				pSpeakerToChannelMap[iCurChs][i] = 1<<iMapedChannel;
			}
		}
		if(pSS)
		{
			pSS->SetSpeakerConfig(!!bCustomChannelMapping, pSpeakerToChannelMap);
		}
	}

}
void CMainFrame::OnUpdateChannalMapMenu(CCmdUI *pCmdUI){
	//pCmdUI->m_nID
	int iAudioChannelMaping  = (pCmdUI->m_nID - IDS_AUDIOCHANNALMAPNORMAL);
	if( m_iAudioChannelMaping == iAudioChannelMaping ){
		pCmdUI->SetCheck(TRUE);
	}else{
		pCmdUI->SetCheck(FALSE);
	}
	if (iAudioChannelMaping > 1){
		CComQIPtr<IAudioSwitcherFilter>  pSS = FindFilter(__uuidof(CAudioSwitcherFilter), pGB);
		if(!pSS) pSS = FindFilter(L"{D3CD7858-971A-4838-ACEC-40CA5D529DC8}", pGB);
		if(pSS)
		{
			int iCurChs = pSS->GetNumberOfInputChannels();
			if( iCurChs  < iAudioChannelMaping ){
				pCmdUI->Enable(FALSE);
			}else{
				pCmdUI->Enable(TRUE);
			}
		}
	}else{
		pCmdUI->Enable(TRUE);
	}

}
void CMainFrame::OnSubMenuToolbar(){
	CMenu* pSubMenu;
	CMenu* pSub = &m_subtoolmenu;
	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	SetupSubtitlesSubMenu();
	SetupNavSubtitleSubMenu();
	MenuMerge( &m_subtitles ,  &m_navsubtitle );
	MenuMerge( &m_subtoolmenu , &m_subtitles );

	SetupSubtitlesSubMenu(2);
	MenuMerge( &m_subtitles2 ,  &m_navsubtitle );
	pSubMenu = &m_subtitles2;

	if(m_subtoolmenu.GetMenuItemCount())
		m_subtoolmenu.AppendMenu(MF_SEPARATOR);

	m_subtoolmenu.AppendMenu(MF_POPUP, (UINT)m_subtitles2.m_hMenu,_T("第二字幕"));

	CMenu* netSubMenu = NULL;
	CMenu* popmenuMain = m_popup.GetSubMenu(0);
	for(int iPos = 0; iPos < popmenuMain->GetMenuItemCount(); iPos++){
		CString str;
		if (popmenuMain->GetMenuString(iPos, str, MF_BYPOSITION)){
			if(str.Find(_T("网络字幕")) >= 0){
				netSubMenu = popmenuMain->GetSubMenu(iPos);
				break;
			}
		}
		
	}
	
	if(netSubMenu){
		m_subtoolmenu.AppendMenu(MF_POPUP, (UINT)netSubMenu->m_hMenu,_T("网络字幕..."));
	}
	m_subtoolmenu.AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_FILE_LOAD_SUBTITLE, _T("调入字幕文件..."));


	OnMenu(&m_subtoolmenu);
}
void CMainFrame::SetupSubtitlesSubMenu(int subid)
{
	CMenu* pSub ;
	if (subid == 2){
		pSub = &m_subtitles2;
	}else{
		pSub = &m_subtitles;
	}

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED || m_fAudioOnly || !m_pCAP)
		return;

	UINT id ;
	if (subid == 2){
		id = ID_SUBTITLES_SUBITEM_START2;
	}else{
		id = ID_SUBTITLES_SUBITEM_START;
	}

	POSITION pos = m_pSubStreams.GetHeadPosition();
	if (subid == 2){ pos = m_pSubStreams2.GetHeadPosition(); }

	UINT bHasSub = 0;
	if(pos)
	{
		bHasSub = id;
		id+=3;
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, ResStr(IDS_SUBTITLES_ENABLE));
		pSub->AppendMenu(MF_SEPARATOR);
	}
	BOOL HavSubs = FALSE;

	while(pos)
	{
		CComPtr<ISubStream> pSubStream ;
		if (subid == 2){ pSubStream = m_pSubStreams2.GetNext(pos); } else { pSubStream = m_pSubStreams.GetNext(pos); }
		if(!pSubStream) continue;

		for(int i = 0, j = pSubStream->GetStreamCount(); i < j; i++)
		{
			WCHAR* pName = NULL;
			if(SUCCEEDED(pSubStream->GetStreamInfo(i, &pName, NULL)))
			{
				CString name(pName);
				name.Replace(_T("&"), _T("&&"));

				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, name);
				HavSubs = true;
				CoTaskMemFree(pName);
			}
			else
			{
				pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, _T("<Unknown>"));
				HavSubs = true;
			}
		}

		// TODO: find a better way to group these entries
		if(pos && ( ( (subid == 2) && m_pSubStreams2.GetAt(pos) ) || ( (subid != 2) && m_pSubStreams.GetAt(pos) ) ) )
		{
			CLSID cur, next;
			pSubStream->GetClassID(&cur);
			if (subid == 2)
				m_pSubStreams2.GetAt(pos)->GetClassID(&next);
			else
				m_pSubStreams.GetAt(pos)->GetClassID(&next);

			if(cur != next && HavSubs )
				pSub->AppendMenu(MF_SEPARATOR);
		}
	}

	if(bHasSub){
		pSub->AppendMenu(MF_SEPARATOR);
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, bHasSub++, ResStr(IDS_SUBTITLES_OPTIONS));
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, bHasSub++, ResStr(IDS_SUBTITLES_STYLES));
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, bHasSub++, ResStr(IDS_SUBTITLES_RELOAD));
	
	}
	if (subid == 2){
		if(pSub->GetMenuItemCount() > 0)
			pSub->AppendMenu(MF_SEPARATOR);

		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_CONFIG_AUTOLOADSUBTITLE2 , _T("自动启用第二字幕"));
	}

}

void CMainFrame::SetupNavAudioSubMenu()
{
	CMenu* pSub = &m_navaudio;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_AUDIO_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavStreamSelectSubMenu(pSub, id, 1);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		if(FAILED(pDVDI->GetCurrentAudio(&ulStreamsAvailable, &ulCurrentStream)))
			return;

		LCID DefLanguage;
		DVD_AUDIO_LANG_EXT ext;
		if(FAILED(pDVDI->GetDefaultAudioLanguage(&DefLanguage, &ext)))
			return;

        for(ULONG i = 0; i < ulStreamsAvailable; i++)
		{
			LCID Language;
			if(FAILED(pDVDI->GetAudioLanguage(i, &Language)))
				continue;

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(Language == DefLanguage) flags |= MF_DEFAULT;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str(_T("Unknown"));
			if(Language)
			{
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			DVD_AudioAttributes ATR;
			if(SUCCEEDED(pDVDI->GetAudioAttributes(i, &ATR)))
			{
				switch(ATR.LanguageExtension)
				{
				case DVD_AUD_EXT_NotSpecified:
				default: break;
				case DVD_AUD_EXT_Captions: str += _T(" (Captions)"); break;
				case DVD_AUD_EXT_VisuallyImpaired: str += _T(" (Visually Impaired)"); break;
				case DVD_AUD_EXT_DirectorComments1: str += _T(" (Director Comments 1)"); break;
				case DVD_AUD_EXT_DirectorComments2: str += _T(" (Director Comments 2)"); break;
				}

				CString format;
				switch(ATR.AudioFormat)
				{
				case DVD_AudioFormat_AC3: format = _T("AC3"); break;
				case DVD_AudioFormat_MPEG1: 
				case DVD_AudioFormat_MPEG1_DRC: format = _T("MPEG1"); break;
				case DVD_AudioFormat_MPEG2: 
				case DVD_AudioFormat_MPEG2_DRC: format = _T("MPEG2"); break;
				case DVD_AudioFormat_LPCM: format = _T("LPCM"); break;
				case DVD_AudioFormat_DTS: format = _T("DTS"); break;
				case DVD_AudioFormat_SDDS: format = _T("SDDS"); break;
				}

				if(!format.IsEmpty())
				{
					str.Format(_T("%s, %s %dHz %dbits %d channel(s)"), 
						CString(str),
						format,
						ATR.dwFrequency,
						ATR.bQuantization,
						ATR.bNumberOfChannels);
				}
			}

			str.Replace(_T("&"), _T("&&"));

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavSubtitleSubMenu()
{
	CMenu* pSub = &m_navsubtitle;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_SUBP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavStreamSelectSubMenu(pSub, id, 2);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		BOOL bIsDisabled;
		if(FAILED(pDVDI->GetCurrentSubpicture(&ulStreamsAvailable, &ulCurrentStream, &bIsDisabled))
		|| ulStreamsAvailable == 0)
			return;

		LCID DefLanguage;
		DVD_SUBPICTURE_LANG_EXT ext;
		if(FAILED(pDVDI->GetDefaultSubpictureLanguage(&DefLanguage, &ext)))
			return;

		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|(bIsDisabled?0:MF_CHECKED), id++, _T("Enabled"));
		pSub->AppendMenu(MF_BYCOMMAND|MF_SEPARATOR|MF_ENABLED);

        for(ULONG i = 0; i < ulStreamsAvailable; i++)
		{
			LCID Language;
			if(FAILED(pDVDI->GetSubpictureLanguage(i, &Language)))
				continue;

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(Language == DefLanguage) flags |= MF_DEFAULT;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str(_T("Unknown"));
			if(Language)
			{
				int len = GetLocaleInfo(Language, LOCALE_SENGLANGUAGE, str.GetBuffer(256), 256);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			DVD_SubpictureAttributes ATR;
			if(SUCCEEDED(pDVDI->GetSubpictureAttributes(i, &ATR)))
			{
				switch(ATR.LanguageExtension)
				{
				case DVD_SP_EXT_NotSpecified:
				default: break;
				case DVD_SP_EXT_Caption_Normal: str += _T(""); break;
				case DVD_SP_EXT_Caption_Big: str += _T(" (Big)"); break;
				case DVD_SP_EXT_Caption_Children: str += _T(" (Children)"); break;
				case DVD_SP_EXT_CC_Normal: str += _T(" (CC)"); break;
				case DVD_SP_EXT_CC_Big: str += _T(" (CC Big)"); break;
				case DVD_SP_EXT_CC_Children: str += _T(" (CC Children)"); break;
				case DVD_SP_EXT_Forced: str += _T(" (Forced)"); break;
				case DVD_SP_EXT_DirectorComments_Normal: str += _T(" (Director Comments)"); break;
				case DVD_SP_EXT_DirectorComments_Big: str += _T(" (Director Comments, Big)"); break;
				case DVD_SP_EXT_DirectorComments_Children: str += _T(" (Director Comments, Children)"); break;
				}
			}

			str.Replace(_T("&"), _T("&&"));

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavAngleSubMenu()
{
	CMenu* pSub = &m_navangle;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED) return;

	UINT id = ID_NAVIGATE_ANGLE_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupNavStreamSelectSubMenu(pSub, id, 0);
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulStreamsAvailable, ulCurrentStream;
		if(FAILED(pDVDI->GetCurrentAngle(&ulStreamsAvailable, &ulCurrentStream)))
			return;

		if(ulStreamsAvailable < 2) return; // one choice is not a choice...

        for(ULONG i = 1; i <= ulStreamsAvailable; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
            if(i == ulCurrentStream) flags |= MF_CHECKED;

			CString str;
			str.Format(_T("Angle %d"), i);

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavChaptersSubMenu()
{
	CMenu* pSub = &m_navchapters;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	if(m_iMediaLoadState != MLS_LOADED)
		return;

	UINT id = ID_NAVIGATE_CHAP_SUBITEM_START;

	if(m_iPlaybackMode == PM_FILE)
	{
		SetupChapters();

		REFERENCE_TIME rt = GetPos();
		DWORD j = m_pCB->ChapLookup(&rt, NULL);

		for(DWORD i = 0; i < m_pCB->ChapGetCount(); i++, id++)
		{
			rt = 0;
			CComBSTR bstr;
			if(FAILED(m_pCB->ChapGet(i, &rt, &bstr)))
				continue;

			int s = (int)((rt/10000000)%60);
			int m = (int)((rt/10000000/60)%60);
			int h = (int)((rt/10000000/60/60));

			CString time;
			time.Format(_T("[%02d:%02d:%02d] "), h, m, s);

			CString name = CString(bstr);
			name.Replace(_T("&"), _T("&&"));
			name.Replace(_T("\t"), _T(" "));

			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(i == j) flags |= MF_CHECKED;
			if(id != ID_NAVIGATE_CHAP_SUBITEM_START && i == 0) pSub->AppendMenu(MF_SEPARATOR);
			pSub->AppendMenu(flags, id, name + '\t' + time);
		}

		if(m_wndPlaylistBar.GetCount() > 1)
		{
			POSITION pos = m_wndPlaylistBar.m_pl.GetHeadPosition();
			while(pos)
			{
				UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
				if(pos == m_wndPlaylistBar.m_pl.GetPos()) flags |= MF_CHECKED;
				if(id != ID_NAVIGATE_CHAP_SUBITEM_START && pos == m_wndPlaylistBar.m_pl.GetHeadPosition())
					pSub->AppendMenu(MF_SEPARATOR);
				CPlaylistItem& pli = m_wndPlaylistBar.m_pl.GetNext(pos);
				CString name = pli.GetLabel();
				name.Replace(_T("&"), _T("&&"));
				pSub->AppendMenu(flags, id++, name);
			}
		}
	}
	else if(m_iPlaybackMode == PM_DVD)
	{
		ULONG ulNumOfVolumes, ulVolume;
		DVD_DISC_SIDE Side;
		ULONG ulNumOfTitles = 0;
		pDVDI->GetDVDVolumeInfo(&ulNumOfVolumes, &ulVolume, &Side, &ulNumOfTitles);

		DVD_PLAYBACK_LOCATION2 Location;
		pDVDI->GetCurrentLocation(&Location);

		ULONG ulNumOfChapters = 0;
		pDVDI->GetNumberOfChapters(Location.TitleNum, &ulNumOfChapters);

		ULONG ulUOPs = 0;
		pDVDI->GetCurrentUOPS(&ulUOPs);

		for(ULONG i = 1; i <= ulNumOfTitles; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(i == Location.TitleNum) flags |= MF_CHECKED;
			if(ulUOPs&UOP_FLAG_Play_Title) flags |= MF_DISABLED|MF_GRAYED;

			CString str;
			str.Format(_T("Title %d"), i);

			pSub->AppendMenu(flags, id++, str);
		}

		for(ULONG i = 1; i <= ulNumOfChapters; i++)
		{
			UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
			if(i == Location.ChapterNum) flags |= MF_CHECKED;
			if(ulUOPs&UOP_FLAG_Play_Chapter) flags |= MF_DISABLED|MF_GRAYED;
			if(i == 1) flags |= MF_MENUBARBREAK;

			CString str;
			str.Format(_T("Chapter %d"), i);

			pSub->AppendMenu(flags, id++, str);
		}
	}
}

void CMainFrame::SetupNavStreamSelectSubMenu(CMenu* pSub, UINT id, DWORD dwSelGroup)
{
	UINT baseid = id;

	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) pSS = FindFilter(L"{55DA30FC-F16B-49fc-BAA5-AE59FC65F82D}", pGB);
	if(!pSS) return;

	DWORD cStreams;
	if(FAILED(pSS->Count(&cStreams)))
		return;

	DWORD dwPrevGroup = -1;

	for(int i = 0, j = cStreams; i < j; i++)
	{
		DWORD dwFlags, dwGroup;
		LCID lcid;
		WCHAR* pszName = NULL;

		if(FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
		|| !pszName)
			continue;

		CString name(pszName);
		CString lcname = CString(name).MakeLower();

		if(pszName) CoTaskMemFree(pszName);

		if(dwGroup != dwSelGroup)
			continue;

		if(dwPrevGroup != -1 && dwPrevGroup != dwGroup)
			pSub->AppendMenu(MF_SEPARATOR);
		dwPrevGroup = dwGroup;

		CString str;

		if(lcname.Find(_T(" off")) >= 0)
		{
			str = _T("Disabled");
		}
		else 
		{
			if(lcid == 0)
			{
				str.Format(_T("Unknown %d"), id - baseid);
			}
			else
			{
				int len = GetLocaleInfo(lcid, LOCALE_SENGLANGUAGE, str.GetBuffer(64), 64);
				str.ReleaseBufferSetLength(max(len-1, 0));
			}

			CString lcstr = CString(str).MakeLower();

			if(str.IsEmpty() || lcname.Find(lcstr) >= 0) str = name;
			else if(!name.IsEmpty()) str = CString(name) + _T(" (") + str + _T(")");
		}

		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
		if(dwFlags) flags |= MF_CHECKED;
		
		str.Replace(_T("&"), _T("&&"));
		pSub->AppendMenu(flags, id++, str);
	}
}

void CMainFrame::OnNavStreamSelectSubMenu(UINT id, DWORD dwSelGroup)
{
	CComQIPtr<IAMStreamSelect> pSS = FindFilter(CLSID_OggSplitter, pGB);
	if(!pSS) pSS = FindFilter(L"{55DA30FC-F16B-49fc-BAA5-AE59FC65F82D}", pGB);
	if(!pSS) return;

	DWORD cStreams;
	if(FAILED(pSS->Count(&cStreams)))
		return;

	for(int i = 0, j = cStreams; i < j; i++)
	{
		DWORD dwFlags, dwGroup;
		LCID lcid;
		WCHAR* pszName = NULL;

		if(FAILED(pSS->Info(i, NULL, &dwFlags, &lcid, &dwGroup, &pszName, NULL, NULL))
		|| !pszName)
			continue;

		if(pszName) CoTaskMemFree(pszName);

		if(dwGroup != dwSelGroup)
			continue;

		if(id == 0)
		{
			pSS->Enable(i, AMSTREAMSELECTENABLE_ENABLE);
			break;
		}

		id--;
	}
}
void CMainFrame::OnRecentFileClear(){
	AppSettings& s = AfxGetAppSettings();
	CRecentFileList& MRU = s.MRU;
	MRU.ReadList();
	for(int i=0;i < MRU.m_nSize;i++) MRU.Remove(0);
	MRU.WriteList();
}
void CMainFrame::OnRecentFileEnable(){
	AppSettings& s = AfxGetAppSettings();
	s.fKeepHistory = true;
}
void CMainFrame::OnRecentFileDisable(){
	AppSettings& s = AfxGetAppSettings();
	s.fKeepHistory = false;
}
void CMainFrame::SetupRecentFileSubMenu(){
	CMenu* pSub = &m_recentfiles;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	AppSettings& s = AfxGetAppSettings();

	int nLastGroupStart = 0;//pSub->GetMenuItemCount();

	UINT id = ID_RECENT_FILE_START;

	CRecentFileList& MRU = s.MRU;
	MRU.ReadList();

	UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;
	CSVPToolBox svpTool;

	for(int i = 0; i < MRU.GetSize(); i++){
		if(!MRU[i].IsEmpty()){
			if(svpTool.ifFileExist(MRU[i], false)){
				pSub->AppendMenu(flags, ID_RECENT_FILE_START+i, MRU[i]);
				nLastGroupStart ++;
			}
			
		}
	}
	if(nLastGroupStart || s.fKeepHistory ){
		if(nLastGroupStart){
			pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR|MF_ENABLED|MF_BYPOSITION);
			pSub->AppendMenu(flags, ID_RECENTFILE_CLEAR, _T("清除历史记录"));
		}
		pSub->AppendMenu(flags, ID_RECENTFILE_DISABLE, _T("禁用历史记录"));
	}else{
		pSub->AppendMenu(flags, ID_RECENTFILE_ENABLE, _T("启用历史记录"));
	}

	
}
void CMainFrame::SetupFavoritesSubMenu()
{
	CMenu* pSub = &m_favorites;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	AppSettings& s = AfxGetAppSettings();

	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_FAVORITES_ADD, ResStr(IDS_FAVORITES_ADD));
	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_FAVORITES_ORGANIZE, ResStr(IDS_FAVORITES_ORGANIZE));

	int nLastGroupStart = pSub->GetMenuItemCount();

	UINT id = ID_FAVORITES_FILE_START;

	CAtlList<CString> sl;
	AfxGetAppSettings().GetFav(FAV_FILE, sl);

	POSITION pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);
		str.Replace(_T("&"), _T("&&"));
		str.Replace(_T("\t"), _T(" "));

		CAtlList<CString> sl;
		Explode(str, sl, ';', 2);

		str = sl.RemoveHead();

		if(!sl.IsEmpty())
		{
			REFERENCE_TIME rt = 0;
			if(1 == _stscanf(sl.GetHead(), _T("%I64d"), &rt) && rt > 0)
			{
				DVD_HMSF_TIMECODE hmsf = RT2HMSF(rt, 0);
				str.Format(_T("%s\t[%02d:%02d:%02d]"), CString(str), hmsf.bHours, hmsf.bMinutes, hmsf.bSeconds);
			}
		}

		if(!str.IsEmpty()) 
			pSub->AppendMenu(flags, id, str);

		id++;
	}

	if(id > ID_FAVORITES_FILE_START)
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR|MF_ENABLED|MF_BYPOSITION);

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DVD_START;

	AfxGetAppSettings().GetFav(FAV_DVD, sl);

	pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);
		str.Replace(_T("&"), _T("&&"));

		CAtlList<CString> sl;
		Explode(str, sl, ';', 2);

		str = sl.RemoveHead();

		if(!sl.IsEmpty())
		{
			// TODO
		}

		if(!str.IsEmpty()) 
			pSub->AppendMenu(flags, id, str);

		id++;
	}

	if(id > ID_FAVORITES_DVD_START)
		pSub->InsertMenu(nLastGroupStart, MF_SEPARATOR|MF_ENABLED|MF_BYPOSITION);

	nLastGroupStart = pSub->GetMenuItemCount();

	id = ID_FAVORITES_DEVICE_START;

	AfxGetAppSettings().GetFav(FAV_DEVICE, sl);

	pos = sl.GetHeadPosition();
	while(pos)
	{
		UINT flags = MF_BYCOMMAND|MF_STRING|MF_ENABLED;

		CString str = sl.GetNext(pos);
		str.Replace(_T("&"), _T("&&"));

		CAtlList<CString> sl;
		Explode(str, sl, ';', 2);

		str = sl.RemoveHead();

		if(!str.IsEmpty()) 
			pSub->AppendMenu(flags, id, str);

		id++;
	}
}

void CMainFrame::SetupShadersSubMenu()
{
	CMenu* pSub = &m_shaders;

	if(!IsMenu(pSub->m_hMenu)) pSub->CreatePopupMenu();
	else while(pSub->RemoveMenu(0, MF_BYPOSITION));

	CWinApp* pApp = AfxGetApp();
	AppSettings& s = AfxGetAppSettings();
	
	if(s.iDSVideoRendererType != 6 || s.iRMVideoRendererType != 2 || s.iQTVideoRendererType != 2 || s.iAPSurfaceUsage != VIDRNDT_AP_TEXTURE3D){
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_SHADERS_SETDX9,_T("需要启用DX9..."));
		return;
	}

	pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, ID_SHADERS_START, ResStr(IDS_SHADER_OFF));

	UINT id = ID_SHADERS_START+1;

	if(POSITION pos = AfxGetAppSettings().m_shaders.GetHeadPosition())
	{
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, ResStr(IDS_SHADER_COMBINE));
		pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id++, ResStr(IDS_SHADER_EDIT));
		pSub->AppendMenu(MF_SEPARATOR);

		MENUITEMINFO mii;
		memset(&mii, 0, sizeof(mii));
		mii.cbSize = sizeof(mii);
		mii.fMask |= MIIM_DATA;

		while(pos)
		{
			const AppSettings::Shader& s = AfxGetAppSettings().m_shaders.GetNext(pos);
			CString label = s.label;
			label.Replace(_T("&"), _T("&&"));
			pSub->AppendMenu(MF_BYCOMMAND|MF_STRING|MF_ENABLED, id, label);
			mii.dwItemData = (ULONG_PTR)&s;
			pSub->SetMenuItemInfo(id, &mii);
			id++;
		}
	}
}

/////////////

void CMainFrame::ShowControls(int nCS, bool fSave)
{
	int nCSprev = AfxGetAppSettings().nCS;
	int hbefore = 0, hafter = 0;
	BOOL bSomthingChanged = false;
	
	nCS &= ~CS_STATUSBAR;
	//if(!IsSomethingLoaded()){
		//nCS &= ~CS_SEEKBAR;
	//}
	m_pLastBar = NULL;

	POSITION pos = m_bars.GetHeadPosition();
	for(int i = 1; pos; i <<= 1)
	{
		CControlBar* pNext = m_bars.GetNext(pos);

		if(!bSomthingChanged){
			if(nCS&i){
				if(!pNext->IsVisible()){
					bSomthingChanged = true;
					
				}
			}else{
				if(pNext->IsVisible()){
					bSomthingChanged = true;
					
				}
			}
		}

		if( (nCS&i) == CS_TOOLBAR && !pNext->IsVisible() && !m_fnCurPlayingFile.IsEmpty()){
			CPath fuPath(m_fnCurPlayingFile);
			CSVPToolBox svpTool;
			fuPath.StripPath();

			SendStatusMessage(CString(_T("正在播放: ")) + CString(fuPath) + _T(" 位于: ") + svpTool.GetDirFromPath(m_fnCurPlayingFile), 2000);
		}

		if( !!(nCS&i) != !!pNext->IsVisible() )
			ShowControlBar(pNext, !!(nCS&i), TRUE);

		if(nCS&i) m_pLastBar = pNext;

		CSize s = pNext->CalcFixedLayout(FALSE, TRUE);
		if(nCSprev&i) hbefore += s.cy;
		if(nCS&i) hafter += s.cy;
	}

	WINDOWPLACEMENT wp;
	wp.length = sizeof(wp);
	GetWindowPlacement(&wp);

	/* Hold Position
	if(wp.showCmd != SW_SHOWMAXIMIZED && !m_fFullScreen)
		{
			CRect r;
			GetWindowRect(r);
			MoveWindow(r.left, r.top, r.Width(), r.Height()+(hafter-hbefore));
		}
	*/
	
	
    if(fSave)
		AfxGetAppSettings().nCS = nCS;

	if(bSomthingChanged){
		if(!m_fFullScreen){
			RecalcLayout();

			RedrawNonClientArea();
		}
	}
}

void CMainFrame::SetAlwaysOnTop(int i, BOOL setSetting)
{
	if(setSetting){
		AfxGetAppSettings().iOnTop = i;
	}

	if(1)//!m_fFullScreen
	{
		const CWnd* pInsertAfter = NULL;

		if(i == 0)
			pInsertAfter = &wndNoTopMost;
		else if(i == 1)
			pInsertAfter = &wndTopMost;
		else // if(i == 2)
			pInsertAfter = GetMediaState() == State_Running ? &wndTopMost : &wndNoTopMost;

		SetWindowPos(pInsertAfter, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}
	else if(!(GetWindowLong(m_hWnd, GWL_EXSTYLE)&WS_EX_TOPMOST))
	{
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}
}

void CMainFrame::AddTextPassThruFilter()
{
	BeginEnumFilters(pGB, pEF, pBF)
	{
		if(!IsSplitter(pBF)) continue;

		BeginEnumPins(pBF, pEP, pPin)
		{
			CComPtr<IPin> pPinTo;
			AM_MEDIA_TYPE mt;
			if(FAILED(pPin->ConnectedTo(&pPinTo)) || !pPinTo 
			|| FAILED(pPin->ConnectionMediaType(&mt)) 
			|| mt.majortype != MEDIATYPE_Text && mt.majortype != MEDIATYPE_Subtitle)
				continue;

			CComQIPtr<IBaseFilter> pTPTF = new CTextPassThruFilter(this);
			CStringW name;
			name.Format(L"TextPassThru%08x", pTPTF);
			if(FAILED(pGB->AddFilter(pTPTF, name)))
				continue;

			HRESULT hr;

			hr = pPinTo->Disconnect();
			hr = pPin->Disconnect();

			if(FAILED(hr = pGB->ConnectDirect(pPin, GetFirstPin(pTPTF, PINDIR_INPUT), NULL))
			|| FAILED(hr = pGB->ConnectDirect(GetFirstPin(pTPTF, PINDIR_OUTPUT), pPinTo, NULL)))
				hr = pGB->ConnectDirect(pPin, pPinTo, NULL);
			else{
				m_pSubStreams.AddTail(CComQIPtr<ISubStream>(pTPTF));
				m_pSubStreams2.AddTail(CComQIPtr<ISubStream>(pTPTF));
			}
		}
		EndEnumPins
	}
	EndEnumFilters
}

bool CMainFrame::LoadSubtitle(CString fn, int sub_delay_ms, BOOL bIsForPlayList)
{
	CString szBuf;
	szBuf.Format(_T("Loading subtile %s delay %d %s"), fn , sub_delay_ms , ( bIsForPlayList ? _T("for playlist") : _T("") ) );
	SVP_LogMsg(szBuf);

	CComPtr<ISubStream> pSubStream;
	CComPtr<ISubStream> pSubStream2;

	// TMP: maybe this will catch something for those who get a runtime error dialog when opening subtitles from cds
	try
	{
		if(!pSubStream)
		{
			CAutoPtr<CVobSubFile> p(new CVobSubFile(&m_csSubLock));
			if(CString(CPath(fn).GetExtension()).MakeLower() == _T(".idx") && p && p->Open(fn) && p->GetStreamCount() > 0)
				pSubStream = p.Detach();

			CAutoPtr<CVobSubFile> p2(new CVobSubFile(&m_csSubLock2));
			if(CString(CPath(fn).GetExtension()).MakeLower() == _T(".idx") && p2 && p2->Open(fn) && p2->GetStreamCount() > 0)
				pSubStream2 = p2.Detach();
		}

		if(!pSubStream)
		{
			CAutoPtr<ssf::CRenderer> p(new ssf::CRenderer(&m_csSubLock));
			if(p && p->Open(fn) && p->GetStreamCount() > 0)
				pSubStream = p.Detach();

			CAutoPtr<ssf::CRenderer> p2(new ssf::CRenderer(&m_csSubLock2));
			if(p2 && p2->Open(fn) && p2->GetStreamCount() > 0)
				pSubStream2 = p2.Detach();
		}

		if(!pSubStream)
		{
			CAutoPtr<CRenderedTextSubtitle> p(new CRenderedTextSubtitle(&m_csSubLock));
			//detect fn charst
			CSVPToolBox svt;
			int cset = svt.DetectFileCharset(fn);
			
			if(p && p->Open(fn, cset) && p->GetStreamCount() > 0)
				pSubStream = p.Detach();

			CAutoPtr<CRenderedTextSubtitle> p2(new CRenderedTextSubtitle(&m_csSubLock2));
			if(p2 && p2->Open(fn, cset) && p2->GetStreamCount() > 0)
				pSubStream2 = p2.Detach();
		}
	}
	catch(CException* e)
	{
		e->Delete();
	}

	
	CSVPToolBox svTool;
	if(!sub_delay_ms){
		//如果没有预设字幕延迟，视图读取 字幕.delay 获得delay参数
		sub_delay_ms = _wtoi ( svTool.fileGetContent( fn+_T(".delay")) );
	}else{
		//如果有字幕延迟， 而且不是playlist subtitles， 保存到.delay文件
		if(!bIsForPlayList){
			szBuf.Format(_T("%d"), sub_delay_ms);
			svTool.filePutContent(  fn+_T(".delay"), szBuf );
		}
	}

	if(pSubStream)
	{
		pSubStream->notSaveDelay = bIsForPlayList;
		pSubStream->sub_delay_ms = sub_delay_ms;
		m_pSubStreams.AddTail(pSubStream);
	}

	if(pSubStream2)
	{
		pSubStream2->notSaveDelay = bIsForPlayList;
		pSubStream2->sub_delay_ms = sub_delay_ms;
		m_pSubStreams2.AddTail(pSubStream2);
	}

	return(!!pSubStream);
}
void CMainFrame::UpdateSubtitle2(bool fApplyDefStyle)
{
	if(!m_pCAP) return;

	int i = m_iSubtitleSel2;

	POSITION pos = m_pSubStreams2.GetHeadPosition();
	while(pos && i >= 0)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams2.GetNext(pos);

		if(i < pSubStream->GetStreamCount()) 
		{
			CAutoLock cAutoLock(&m_csSubLock2);
			pSubStream->SetStream(i);
			SetSubtitle2(pSubStream, fApplyDefStyle);
			return;
		}

		i -= pSubStream->GetStreamCount();
	}
	SendStatusMessage(_T("第二字幕已关闭") , 4000 );
	m_pCAP->SetSubPicProvider2(NULL);
}
void CMainFrame::UpdateSubtitle(bool fApplyDefStyle)
{
	if(!m_pCAP) return;

	int i = m_iSubtitleSel;

	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos && i >= 0)
	{
		CComPtr<ISubStream> pSubStream = m_pSubStreams.GetNext(pos);

		if(i < pSubStream->GetStreamCount()) 
		{
			CAutoLock cAutoLock(&m_csSubLock);
			pSubStream->SetStream(i);
			SetSubtitle(pSubStream, fApplyDefStyle);
			return;
		}

		i -= pSubStream->GetStreamCount();
	}
	SendStatusMessage(_T("主字幕已关闭") , 4000 );
	m_pCAP->SetSubPicProvider(NULL);
}
void CMainFrame::SetSubtitle2(ISubStream* pSubStream, bool fApplyDefStyle)
{
	AppSettings& s = AfxGetAppSettings();

	if(pSubStream)
	{
		CLSID clsid;
		pSubStream->GetClassID(&clsid);

		int xalign = 1, yalign = 0;
		if(s.subdefstyle2.scrAlignment > 0){
			xalign = (s.subdefstyle2.scrAlignment - 1) % 3;
			yalign = 2 - (int)( (s.subdefstyle2.scrAlignment - 1) / 3 );
		}
		if(clsid == __uuidof(CVobSubFile))
		{
			CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				// 0: left/top, 1: center, 2: right/bottom
				pVSF->SetAlignment(TRUE, s.nHorPos2, s.nVerPos2,xalign, yalign);// 1, 0);
			}
		}
		else if(clsid == __uuidof(CVobSubStream))
		{
			CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSS->SetAlignment(TRUE, s.nHorPos2, s.nVerPos2, xalign, yalign);// 1, 0);
			}
		}
		else if(clsid == __uuidof(CRenderedTextSubtitle))
		{
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

			STSStyle style;
			CRect nPosRect(0,0,0,0);
			if(fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle)
			{
				style = s.subdefstyle2;

				
				if(s.fOverridePlacement2)
				{
					//style.scrAlignment = 8; // 1 - 9: as on the numpad, 0: default
					int w = pRTS->m_dstScreenSize.cx;
					int h = pRTS->m_dstScreenSize.cy;
					int mw = w - style.marginRect.left - style.marginRect.right;
					style.marginRect.bottom = h - MulDiv(h, s.nVerPos2, 100);
					style.marginRect.left = MulDiv(w, s.nHorPos2, 100) - mw/2;
					style.marginRect.right = w - (style.marginRect.left + mw);

					nPosRect = style.marginRect;
				}
				
				pRTS->SetDefaultStyle(style);
				pRTS->SetAliPos(  style.scrAlignment, nPosRect);
			}
			
			if(pRTS->GetDefaultStyle(style)/* && style.relativeTo == 8*/)
			{
				style.relativeTo = s.subdefstyle2.relativeTo;
				pRTS->SetDefaultStyle(style);
			}


			pRTS->Deinit();
		}
	}
	
// 	if(!fApplyDefStyle)
// 	{
		m_iSubtitleSel2 = -1;

		if(pSubStream)
		{

			int i = 0;

			POSITION pos = m_pSubStreams2.GetHeadPosition();
			while(pos)
			{
				CComPtr<ISubStream> pSubStream2 = m_pSubStreams2.GetNext(pos);

				if(pSubStream == pSubStream2)
				{
					m_iSubtitleSel2 = i + pSubStream2->GetStream();
					break;
				}

				i += pSubStream2->GetStreamCount();
			}

		}
	/*}*/

	m_nSubtitleId2 = (DWORD_PTR)pSubStream;

	if(m_pCAP)
	{
		CString szBuf;
		CString subName;
		WCHAR* pName = NULL;
		if(SUCCEEDED(pSubStream->GetStreamInfo(pSubStream->GetStream(), &pName, NULL)))
		{
			subName = CString(pName);
			subName.Replace(_T("&"), _T("&&"));
			CoTaskMemFree(pName);
		}

		szBuf.Format(_T("正在显示第二字幕 %s 延时设为 %d 毫秒 高度：%d%%"), subName, pSubStream->sub_delay_ms, s.nVerPos2);
		SVP_LogMsg(szBuf);
		SendStatusMessage(szBuf , 4000 );
		m_pCAP->SetSubPicProvider2(CComQIPtr<ISubPicProvider>(pSubStream));
		SetSubtitleDelay2(pSubStream->sub_delay_ms); 

	}else{
		SendStatusMessage(_T("第二字幕已关闭") , 4000 );
	}
}
void CMainFrame::SetSubtitle(ISubStream* pSubStream, bool fApplyDefStyle)
{
	AppSettings& s = AfxGetAppSettings();

	if(pSubStream)
	{
		CLSID clsid;
		pSubStream->GetClassID(&clsid);

		int xalign = 1, yalign = 1;
		if(s.subdefstyle.scrAlignment > 0){
			xalign = (s.subdefstyle.scrAlignment - 1) % 3;
			yalign = 2 - (int)( (s.subdefstyle.scrAlignment - 1) / 3 );
		}

		if(clsid == __uuidof(CVobSubFile))
		{
			CVobSubFile* pVSF = (CVobSubFile*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSF->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos,  xalign, yalign);//1, 1);
			}
		}
		else if(clsid == __uuidof(CVobSubStream))
		{
			CVobSubStream* pVSS = (CVobSubStream*)(ISubStream*)pSubStream;

			if(fApplyDefStyle)
			{
				pVSS->SetAlignment(s.fOverridePlacement, s.nHorPos, s.nVerPos, xalign, yalign);// 1, 1);
			}
		}
		else if(clsid == __uuidof(CRenderedTextSubtitle))
		{
			CRenderedTextSubtitle* pRTS = (CRenderedTextSubtitle*)(ISubStream*)pSubStream;

			STSStyle style;
			CRect nPosRect(0,0,0,0);

			
			if(fApplyDefStyle || pRTS->m_fUsingAutoGeneratedDefaultStyle)
			{
				style = s.subdefstyle;
			
				if(s.fOverridePlacement)
				{
					//style.scrAlignment = 2;
					int w = pRTS->m_dstScreenSize.cx;
					int h = pRTS->m_dstScreenSize.cy;
					int mw = w - style.marginRect.left - style.marginRect.right;
					style.marginRect.bottom = h - MulDiv(h, s.nVerPos, 100);
					style.marginRect.left = MulDiv(w, s.nHorPos, 100) - mw/2;
					style.marginRect.right = w - (style.marginRect.left + mw);
			
					nPosRect = style.marginRect;
				}
			
				pRTS->SetDefaultStyle(style);
			
				pRTS->SetAliPos(style.scrAlignment, nPosRect );
			}
			
			if(pRTS->GetDefaultStyle(style) /*&& style.relativeTo == 2*/)
 			{
 				style.relativeTo = s.subdefstyle.relativeTo;
 				pRTS->SetDefaultStyle(style);
 			}

			pRTS->Deinit();
		}
	}

	///if(!fApplyDefStyle) // no idea why doing this?? -- Tomas
	//{
		m_iSubtitleSel = -1;

		if(pSubStream)
		{

			int i = 0;

			POSITION pos = m_pSubStreams.GetHeadPosition();
			while(pos)
			{
				CComPtr<ISubStream> pSubStream2 = m_pSubStreams.GetNext(pos);

				if(pSubStream == pSubStream2)
				{
					m_iSubtitleSel = i + pSubStream2->GetStream();
					break;
				}

				i += pSubStream2->GetStreamCount();
			}

		}
	//}

	m_nSubtitleId = (DWORD_PTR)pSubStream;

	if(m_pCAP && pSubStream)
	{
		//m_wndSubresyncBar.SetSubtitle(pSubStream, m_pCAP->GetFPS());
		CString szBuf;
		CString subName;
		WCHAR* pName = NULL;
		if(SUCCEEDED(pSubStream->GetStreamInfo(pSubStream->GetStream(), &pName, NULL)))
		{
			subName = CString(pName);
			subName.Replace(_T("&"), _T("&&"));
			CoTaskMemFree(pName);
		}
		
		
		szBuf.Format(_T("正在显示主字幕 %s 延时设为 %d 毫秒 高度：%d%%"), subName,  pSubStream->sub_delay_ms,s.nVerPos);
		SVP_LogMsg(szBuf);
		SendStatusMessage(szBuf , 4000 );
		
		m_pCAP->SetSubPicProvider(CComQIPtr<ISubPicProvider>(pSubStream));
		SetSubtitleDelay(pSubStream->sub_delay_ms); 
		
		
	}else{
		SendStatusMessage(_T("主字幕已关闭") , 4000 );
	}
}

void CMainFrame::ReplaceSubtitle(ISubStream* pSubStreamOld, ISubStream* pSubStreamNew, int secondSub)
{
	POSITION pos = m_pSubStreams.GetHeadPosition();
	if(secondSub){ pos = m_pSubStreams2.GetHeadPosition();}
	while(pos) 
	{
		POSITION cur = pos;
		if(secondSub){
			if(pSubStreamOld == m_pSubStreams2.GetNext(pos))
			{
				m_pSubStreams2.SetAt(cur, pSubStreamNew);
				UpdateSubtitle2();
				break;
			}
		}else{
			if(pSubStreamOld == m_pSubStreams.GetNext(pos))
			{
				m_pSubStreams.SetAt(cur, pSubStreamNew);
				UpdateSubtitle();
				break;
			}
		}
	}
}

void CMainFrame::InvalidateSubtitle(DWORD_PTR nSubtitleId, REFERENCE_TIME rtInvalidate)
{
	if(m_pCAP)
	{
		if(nSubtitleId == -1 || nSubtitleId == m_nSubtitleId)
			m_pCAP->Invalidate(rtInvalidate);
	}
}

void CMainFrame::ReloadSubtitle()
{
	POSITION pos = m_pSubStreams.GetHeadPosition();
	while(pos) m_pSubStreams.GetNext(pos)->Reload();
	pos = m_pSubStreams2.GetHeadPosition();
	while(pos) m_pSubStreams2.GetNext(pos)->Reload();
	UpdateSubtitle();
	UpdateSubtitle2();
}


REFERENCE_TIME CMainFrame::GetPos()
{
	return(m_iMediaLoadState == MLS_LOADED ? m_wndSeekBar.GetPos() : 0);
}

REFERENCE_TIME CMainFrame::GetDur()
{
	__int64 start, stop;
	m_wndSeekBar.GetRange(start, stop);
	return(m_iMediaLoadState == MLS_LOADED ? stop : 0);
}

void CMainFrame::CleanGraph()
{
	if(!pGB) return;

	BeginEnumFilters(pGB, pEF, pBF)
	{
		CComQIPtr<IAMFilterMiscFlags> pAMMF(pBF);
		if(pAMMF && (pAMMF->GetMiscFlags()&AM_FILTER_MISC_FLAGS_IS_SOURCE))
			continue;

		// some capture filters forget to set AM_FILTER_MISC_FLAGS_IS_SOURCE 
		// or to implement the IAMFilterMiscFlags interface
		if(pBF == pVidCap || pBF == pAudCap) 
			continue;

		if(CComQIPtr<IFileSourceFilter>(pBF))
			continue;

		int nIn, nOut, nInC, nOutC;
		if(CountPins(pBF, nIn, nOut, nInC, nOutC) > 0 && (nInC+nOutC) == 0)
		{
			TRACE(CStringW(L"Removing: ") + GetFilterName(pBF) + '\n');

			pGB->RemoveFilter(pBF);
			pEF->Reset();
		}
	}
	EndEnumFilters
}

#define AUDIOBUFFERLEN 500

static void SetLatency(IBaseFilter* pBF, int cbBuffer)
{
	BeginEnumPins(pBF, pEP, pPin)
	{
		if(CComQIPtr<IAMBufferNegotiation> pAMBN = pPin)
		{
			ALLOCATOR_PROPERTIES ap;
			ap.cbAlign = -1;  // -1 means no preference.
			ap.cbBuffer = cbBuffer;
			ap.cbPrefix = -1;
			ap.cBuffers = -1;
			pAMBN->SuggestAllocatorProperties(&ap);
		}
	}
	EndEnumPins
}

HRESULT CMainFrame::BuildCapture(IPin* pPin, IBaseFilter* pBF[3], const GUID& majortype, AM_MEDIA_TYPE* pmt)
{
	IBaseFilter* pBuff = pBF[0];
	IBaseFilter* pEnc = pBF[1];
	IBaseFilter* pMux = pBF[2];

	if(!pPin || !pMux) return E_FAIL;

	CString err;

	HRESULT hr = S_OK;

	CFilterInfo fi;
	if(FAILED(pMux->QueryFilterInfo(&fi)) || !fi.pGraph)
		pGB->AddFilter(pMux, L"Multiplexer");

	CStringW prefix, prefixl;
	if(majortype == MEDIATYPE_Video) prefix = L"Video ";
	else if(majortype == MEDIATYPE_Audio) prefix = L"Audio ";
	prefixl = prefix;
	prefixl.MakeLower();

	if(pBuff)
	{
		hr = pGB->AddFilter(pBuff, prefix + L"Buffer");
		if(FAILED(hr))
		{
			err = _T("Can't add ") + CString(prefixl) + _T("buffer filter");
			AfxMessageBox(err);
			return hr;
		}

		hr = pGB->ConnectFilter(pPin, pBuff);
		if(FAILED(hr))
		{
			err = _T("Error connecting the ") + CString(prefixl) + _T("buffer filter");
			AfxMessageBox(err);
			return(hr);
		}

		pPin = GetFirstPin(pBuff, PINDIR_OUTPUT);
	}

	if(pEnc)
	{
		hr = pGB->AddFilter(pEnc, prefix + L"Encoder");
		if(FAILED(hr))
		{
			err = _T("Can't add ") + CString(prefixl) + _T("encoder filter");
			AfxMessageBox(err);
			return hr;
		}

		hr = pGB->ConnectFilter(pPin, pEnc);
		if(FAILED(hr))
		{
			err = _T("Error connecting the ") + CString(prefixl) + _T("encoder filter");
			AfxMessageBox(err);
			return(hr);
		}

		pPin = GetFirstPin(pEnc, PINDIR_OUTPUT);

		if(CComQIPtr<IAMStreamConfig> pAMSC = pPin)
		{
			if(pmt->majortype == majortype)
			{
				hr = pAMSC->SetFormat(pmt);
				if(FAILED(hr))
				{
					err = _T("Can't set compression format on the ") + CString(prefixl) + _T("encoder filter");
					AfxMessageBox(err);
					return(hr);
				}
			}
		}

	}

//	if(pMux)
	{
		hr = pGB->ConnectFilter(pPin, pMux);
		if(FAILED(hr))
		{
			err = _T("Error connecting ") + CString(prefixl) + _T(" to the muliplexer filter");
			AfxMessageBox(err);
			return(hr);
		}
	}

	CleanGraph();

	return S_OK;
}

bool CMainFrame::BuildToCapturePreviewPin(
	IBaseFilter* pVidCap, IPin** ppVidCapPin, IPin** ppVidPrevPin, 
	IBaseFilter* pAudCap, IPin** ppAudCapPin, IPin** ppAudPrevPin)
{
	HRESULT hr;

	*ppVidCapPin = *ppVidPrevPin = NULL; 
	*ppAudCapPin = *ppAudPrevPin = NULL;

	CComPtr<IPin> pDVAudPin;

	if(pVidCap)
	{
		CComPtr<IPin> pPin;
		if(!pAudCap // only look for interleaved stream when we don't use any other audio capture source
		&& SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Interleaved, TRUE, 0, &pPin)))
		{
			CComPtr<IBaseFilter> pDVSplitter;
			hr = pDVSplitter.CoCreateInstance(CLSID_DVSplitter);
			hr = pGB->AddFilter(pDVSplitter, L"DV Splitter");

			hr = pCGB->RenderStream(NULL, &MEDIATYPE_Interleaved, pPin, NULL, pDVSplitter);

			pPin = NULL;
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
			hr = pCGB->FindPin(pDVSplitter, PINDIR_OUTPUT, NULL, &MEDIATYPE_Audio, TRUE, 0, &pDVAudPin);

			CComPtr<IBaseFilter> pDVDec;
			hr = pDVDec.CoCreateInstance(CLSID_DVVideoCodec);
			hr = pGB->AddFilter(pDVDec, L"DV Video Decoder");

			hr = pGB->ConnectFilter(pPin, pDVDec);

			pPin = NULL;
			hr = pCGB->FindPin(pDVDec, PINDIR_OUTPUT, NULL, &MEDIATYPE_Video, TRUE, 0, &pPin);
		}
		else if(SUCCEEDED(pCGB->FindPin(pVidCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, TRUE, 0, &pPin)))
		{
		}
		else
		{
			AfxMessageBox(_T("No video capture pin was found"));
			return(false);
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = pGB->AddFilter(pSmartTee, L"Smart Tee (video)");

		hr = pGB->ConnectFilter(pPin, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppVidPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppVidCapPin);
	}

	if(pAudCap || pDVAudPin)
	{
		CComPtr<IPin> pPin;
		if(pDVAudPin)
		{
			pPin = pDVAudPin;
		}
		else if(SUCCEEDED(pCGB->FindPin(pAudCap, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, TRUE, 0, &pPin)))
		{
		}
		else
		{
			AfxMessageBox(_T("No audio capture pin was found"));
			return(false);
		}

		CComPtr<IBaseFilter> pSmartTee;
		hr = pSmartTee.CoCreateInstance(CLSID_SmartTee);
		hr = pGB->AddFilter(pSmartTee, L"Smart Tee (audio)");

		hr = pGB->ConnectFilter(pPin, pSmartTee);

		hr = pSmartTee->FindPin(L"Preview", ppAudPrevPin);
		hr = pSmartTee->FindPin(L"Capture", ppAudCapPin);
	}

	return(true);
}

bool CMainFrame::BuildGraphVideoAudio(int fVPreview, bool fVCapture, int fAPreview, bool fACapture)
{
	if(!pCGB) return(false);

	SaveMediaState;

	HRESULT hr;

	pGB->NukeDownstream(pVidCap);
	pGB->NukeDownstream(pAudCap);

	CleanGraph();

	if(pAMVSCCap) hr = pAMVSCCap->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	if(pAMVSCPrev) hr = pAMVSCPrev->SetFormat(&m_wndCaptureBar.m_capdlg.m_mtv);
	if(pAMASC) hr = pAMASC->SetFormat(&m_wndCaptureBar.m_capdlg.m_mta);

	CComPtr<IBaseFilter> pVidBuffer = m_wndCaptureBar.m_capdlg.m_pVidBuffer;
	CComPtr<IBaseFilter> pAudBuffer = m_wndCaptureBar.m_capdlg.m_pAudBuffer;
	CComPtr<IBaseFilter> pVidEnc = m_wndCaptureBar.m_capdlg.m_pVidEnc;
	CComPtr<IBaseFilter> pAudEnc = m_wndCaptureBar.m_capdlg.m_pAudEnc;
	CComPtr<IBaseFilter> pMux = m_wndCaptureBar.m_capdlg.m_pMux;
	CComPtr<IBaseFilter> pDst = m_wndCaptureBar.m_capdlg.m_pDst;
	CComPtr<IBaseFilter> pAudMux = m_wndCaptureBar.m_capdlg.m_pAudMux;
	CComPtr<IBaseFilter> pAudDst = m_wndCaptureBar.m_capdlg.m_pAudDst;

	bool fFileOutput = (pMux && pDst) || (pAudMux && pAudDst);
	bool fCapture = (fVCapture || fACapture);

	if(pAudCap)
	{
		AM_MEDIA_TYPE* pmt = &m_wndCaptureBar.m_capdlg.m_mta;
		int ms = (fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput) ? AUDIOBUFFERLEN : 60;
		if(pMux != pAudMux && fACapture) SetLatency(pAudCap, -1);
		else if(pmt->pbFormat) SetLatency(pAudCap, ((WAVEFORMATEX*)pmt->pbFormat)->nAvgBytesPerSec * ms / 1000);
	}

	CComPtr<IPin> pVidCapPin, pVidPrevPin, pAudCapPin, pAudPrevPin;
	BuildToCapturePreviewPin(pVidCap, &pVidCapPin, &pVidPrevPin, pAudCap, &pAudCapPin, &pAudPrevPin);

//	if(pVidCap)
	{
		bool fVidPrev = pVidPrevPin && fVPreview;
		bool fVidCap = pVidCapPin && fVCapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fVidOutput;

		if(fVPreview == 2 && !fVidCap && pVidCapPin)
		{
			pVidPrevPin = pVidCapPin;
			pVidCapPin = NULL;
		}

		if(fVidPrev)
		{
			m_pCAP = NULL;
			m_pCAP2 = NULL;
			pGB->Render(pVidPrevPin);
			pGB->FindInterface(__uuidof(ISubPicAllocatorPresenter), (void**)&m_pCAP, TRUE);
			pGB->FindInterface(__uuidof(ISubPicAllocatorPresenter2), (void**)&m_pCAP2, TRUE);
		}

		if(fVidCap)
		{
			IBaseFilter* pBF[3] = {pVidBuffer, pVidEnc, pMux};
			HRESULT hr = BuildCapture(pVidCapPin, pBF, MEDIATYPE_Video, &m_wndCaptureBar.m_capdlg.m_mtcv);
		}

		pAMDF = NULL;
		pCGB->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVidCap, IID_IAMDroppedFrames, (void**)&pAMDF);
	}

//	if(pAudCap)
	{
		bool fAudPrev = pAudPrevPin && fAPreview;
		bool fAudCap = pAudCapPin && fACapture && fFileOutput && m_wndCaptureBar.m_capdlg.m_fAudOutput;

		if(fAPreview == 2 && !fAudCap && pAudCapPin)
		{
			pAudPrevPin = pAudCapPin;
			pAudCapPin = NULL;
		}

		if(fAudPrev)
		{
			pGB->Render(pAudPrevPin);
		}

		if(fAudCap)
		{
			IBaseFilter* pBF[3] = {pAudBuffer, pAudEnc, pAudMux ? pAudMux : pMux};
			HRESULT hr = BuildCapture(pAudCapPin, pBF, MEDIATYPE_Audio, &m_wndCaptureBar.m_capdlg.m_mtca);
		}
	}

	if((pVidCap || pAudCap) && fCapture && fFileOutput)
	{
		if(pMux != pDst)
		{
			hr = pGB->AddFilter(pDst, L"File Writer V/A");
			hr = pGB->ConnectFilter(GetFirstPin(pMux, PINDIR_OUTPUT), pDst);
		}

		if(CComQIPtr<IConfigAviMux> pCAM = pMux)
		{
			int nIn, nOut, nInC, nOutC;
			CountPins(pMux, nIn, nOut, nInC, nOutC);
			pCAM->SetMasterStream(nInC-1);
//			pCAM->SetMasterStream(-1);
			pCAM->SetOutputCompatibilityIndex(FALSE);
		}

		if(CComQIPtr<IConfigInterleaving> pCI = pMux)
		{
//			if(FAILED(pCI->put_Mode(INTERLEAVE_CAPTURE)))
			if(FAILED(pCI->put_Mode(INTERLEAVE_NONE_BUFFERED)))
				pCI->put_Mode(INTERLEAVE_NONE);

			REFERENCE_TIME rtInterleave = 10000i64*AUDIOBUFFERLEN, rtPreroll = 0;//10000i64*500
			pCI->put_Interleaving(&rtInterleave, &rtPreroll);
		}

		if(pMux != pAudMux && pAudMux != pAudDst)
		{
			hr = pGB->AddFilter(pAudDst, L"File Writer A");
			hr = pGB->ConnectFilter(GetFirstPin(pAudMux, PINDIR_OUTPUT), pAudDst);
		}
	}

	REFERENCE_TIME stop = MAX_TIME;
	hr = pCGB->ControlStream(&PIN_CATEGORY_CAPTURE, NULL, NULL, NULL, &stop, 0, 0); // stop in the infinite

	CleanGraph();

	OpenSetupVideo();
	OpenSetupAudio();
	OpenSetupStatsBar();
	OpenSetupStatusBar();

	RestoreMediaState;

	return(true);
}

bool CMainFrame::StartCapture()
{
	if(!pCGB || m_fCapturing) return(false);

	if(!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) return(false);

	HRESULT hr;

	::SetPriorityClass(::GetCurrentProcess(), HIGH_PRIORITY_CLASS);

	// rare to see two capture filters to support IAMPushSource at the same time...
//	hr = CComQIPtr<IAMGraphStreams>(pGB)->SyncUsingStreamOffset(TRUE); // TODO:

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, true, 
		m_wndCaptureBar.m_capdlg.m_fAudPreview, true);

	hr = pME->CancelDefaultHandling(EC_REPAINT);

	SendMessage(WM_COMMAND, ID_PLAY_PLAY);

	m_fCapturing = true;

	return(true);
}

bool CMainFrame::StopCapture()
{
	if(!pCGB || !m_fCapturing) return(false);

	if(!m_wndCaptureBar.m_capdlg.m_pMux && !m_wndCaptureBar.m_capdlg.m_pDst) return(false);

	HRESULT hr;

	m_wndStatusBar.SetStatusMessage(ResStr(IDS_CONTROLS_COMPLETING));

	m_fCapturing = false;

	BuildGraphVideoAudio(
		m_wndCaptureBar.m_capdlg.m_fVidPreview, false, 
		m_wndCaptureBar.m_capdlg.m_fAudPreview, false);

	hr = pME->RestoreDefaultHandling(EC_REPAINT);

	::SetPriorityClass(::GetCurrentProcess(), AfxGetAppSettings().priority);

	m_rtDurationOverride = -1;

	return(true);
}

//
void CMainFrame::OnAdvOptions()
{
	AppSettings& s = AfxGetAppSettings();

	

	
		CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), pGB, this, NULL);

		if(options.DoModal() == IDOK)
		{
			if(!m_fFullScreen)
				SetAlwaysOnTop(s.iOnTop);

			m_wndView.LoadLogo();

			s.UpdateData(true);
		}		
	
		m_btnList.SetHideStat( HTMINTOTRAY,   s.fTrayIcon);
		ShowTrayIcon(s.fTrayIcon);
		RedrawNonClientArea();
}

void CMainFrame::ShowOptions(int idPage)
{
	AppSettings& s = AfxGetAppSettings();
	
	CUESettingPanel ueOption(pGB, this, idPage);
	
	if(ueOption.DoModal() == IDOK)
	{
		if(!m_fFullScreen)
			SetAlwaysOnTop(s.iOnTop);

		m_wndView.LoadLogo();
		UpdateSubtitle(true);
		UpdateSubtitle2(true);	
		
		
	}else if(ueOption.bOpenAdvancePanel){
		CPPageSheet options(ResStr(IDS_OPTIONS_CAPTION), pGB, this, idPage);

		if(options.DoModal() == IDOK)
		{
			if(!m_fFullScreen)
				SetAlwaysOnTop(s.iOnTop);

			m_wndView.LoadLogo();

			s.UpdateData(true);

			UpdateSubtitle(true);
			UpdateSubtitle2(true);
		}		
	}

	m_btnList.SetHideStat( HTMINTOTRAY,   s.fTrayIcon);
	ShowTrayIcon(s.fTrayIcon);
	RedrawNonClientArea();
}

void CMainFrame::StartWebServer(int nPort)
{
	if(!m_pWebServer)
		m_pWebServer.Attach(new CWebServer(this, nPort));
}

void CMainFrame::StopWebServer()
{
	if(m_pWebServer)
		m_pWebServer.Free();
}

CString CMainFrame::GetStatusMessage()
{
	CString str;
	//m_wndStatusBar.m_status.GetWindowText(str);
	str = m_wndToolBar.m_timerstr;
	return str;
}

void CMainFrame::SendStatusMessage(CString msg, int nTimeOut, int iAlign)
{
	KillTimer(TIMER_STATUSERASER);

	m_playingmsg.Empty();
	if(nTimeOut <= 0) return;

	m_playingmsg = msg;
	
	if(!m_wndStatusBar.IsVisible() && 0){
		KillTimer(TIMER_STATUSBARHIDER);

		if(m_fFullScreen){
			ShowControls(CS_STATUSBAR, false);
			SetTimer( TIMER_FULLSCREENCONTROLBARHIDER , 10000, NULL); // auto close it when full screen
		}else{
			ShowControlBar(&m_wndStatusBar, true, false);
			SetTimer( TIMER_STATUSBARHIDER , 10000, NULL); 
		}
	}
	m_iOSDAlign = iAlign;
	m_wndNewOSD.SendOSDMsg(m_playingmsg , nTimeOut);
	rePosOSD();
	//m_wndToolBar.SetStatusTimer( m_playingmsg , nTimeOut);

	SetTimer(TIMER_STATUSERASER, nTimeOut, NULL);
}

void CMainFrame::OpenCurPlaylistItem(REFERENCE_TIME rtStart)
{
	if(m_wndPlaylistBar.GetCount() == 0)
		return;

	CPlaylistItem pli;
	if(!m_wndPlaylistBar.GetCur(pli)) m_wndPlaylistBar.SetFirst();
	if(!m_wndPlaylistBar.GetCur(pli)) return;
	AppSettings& s = AfxGetAppSettings();
	
	if(rtStart == 0){
		CString fn;
		fn = pli.m_fns.GetHead();
		favtype ft ;
		ft = FAV_FILE;
		if (!fn.IsEmpty() && s.autoResumePlay){
			CAtlList<CString> sl;
			s.GetFav(ft, sl, TRUE);
			CString PosStr ;
			POSITION pos = sl.GetHeadPosition();
			while(pos){
				PosStr = sl.GetNext(pos) ;
				if( PosStr.Find(fn + _T(";")) == 0 ){
					break;
				}else{
					PosStr.Empty();
				}
			}
			if(!PosStr.IsEmpty()){

				int iPos = PosStr.ReverseFind( _T(';') );
				if(iPos >= 0){
					CString s2 = PosStr.Right( PosStr.GetLength() - iPos - 1 );
					_stscanf(s2, _T("%I64d"), &rtStart); // pos
				}
				
			}
				
		}
	}else if(rtStart == -1){
		rtStart = 0;
	}
	CAutoPtr<OpenMediaData> p(m_wndPlaylistBar.GetCurOMD(rtStart));
	

	if(p) OpenMedia(p);

}

void CMainFrame::AddCurDevToPlaylist()
{
	if(m_iPlaybackMode == PM_CAPTURE)
	{
		m_wndPlaylistBar.Append(
			m_VidDispName, 
			m_AudDispName, 
			m_wndCaptureBar.m_capdlg.GetVideoInput(), 
			m_wndCaptureBar.m_capdlg.GetVideoChannel(), 
			m_wndCaptureBar.m_capdlg.GetAudioInput()
			);
	}
}

static int s_fOpenedThruThread = false;

void CMainFrame::OpenMedia(CAutoPtr<OpenMediaData> pOMD)
{
	// shortcut
	if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p))
	{
		if(m_iMediaLoadState == MLS_LOADED && pAMTuner
		&& m_VidDispName == p->DisplayName[0] && m_AudDispName == p->DisplayName[1])
		{
			m_wndCaptureBar.m_capdlg.SetVideoInput(p->vinput);
			m_wndCaptureBar.m_capdlg.SetVideoChannel(p->vchannel);
			m_wndCaptureBar.m_capdlg.SetAudioInput(p->ainput);
			SendNowPlayingToMSN();
			SendNowPlayingTomIRC();
			return;
		}
	}

	if(m_iMediaLoadState != MLS_CLOSED)
		CloseMedia();

	m_iMediaLoadState = MLS_LOADING; // HACK: hides the logo

	AppSettings& s = AfxGetAppSettings();

	bool fUseThread = true;
	s.bUsePowerDVD = false;
	if(OpenFileData* p = dynamic_cast<OpenFileData*>(pOMD.m_p))
	{
		if(p->fns.GetCount() > 0)
		{
			engine_t e = s.Formats.GetEngine(p->fns.GetHead());
			fUseThread = e == DirectShow /*|| e == RealMedia || e == QuickTime*/;

			CString szVPathExt = p->fns.GetHead();
			szVPathExt = szVPathExt.Right(3).MakeLower();
			if( szVPathExt == _T("mkv") ||  szVPathExt == _T("avi") ||  szVPathExt == _T(".ts") ){
				s.bUsePowerDVD = true;
			}
		}
	}
	else if(OpenDeviceData* p = dynamic_cast<OpenDeviceData*>(pOMD.m_p))
	{
		fUseThread = false;
		s.bUsePowerDVD = true;
	}

	if(m_pGraphThread && fUseThread
	&& AfxGetAppSettings().fEnableWorkerThreadForOpening)
	{
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_OPEN, 0, (LPARAM)pOMD.Detach());
		s_fOpenedThruThread = true;
	}
	else
	{
		OpenMediaPrivate(pOMD);
		s_fOpenedThruThread = false;
	}
}

void CMainFrame::CloseMedia()
{

	CString fnx = m_wndPlaylistBar.GetCur();

	if(m_iMediaLoadState == MLS_CLOSING)
	{
		TRACE(_T("WARNING: CMainFrame::CloseMedia() called twice or more\n"));
		return;
	}
	if( m_iMediaLoadState == MLS_LOADED){
			//save time for next play
			OnFavoritesAddReal(TRUE);
	}

	int nTimeWaited = 0;

	while(m_iMediaLoadState == MLS_LOADING)
	{
	

		m_fOpeningAborted = true;

		if(pGB) pGB->Abort(); // TODO: lock on graph objects somehow, this is not thread safe

		if(nTimeWaited > 5*1000 && m_pGraphThread)
		{
			MessageBeep(MB_ICONEXCLAMATION);
			TRACE(_T("CRITICAL ERROR: !!! Must kill opener thread !!!"));
			TerminateThread(m_pGraphThread->m_hThread, -1);
			m_pGraphThread = (CGraphThread*)AfxBeginThread(RUNTIME_CLASS(CGraphThread));
			s_fOpenedThruThread = false;
			break;
		}

		Sleep(50);

		nTimeWaited += 50;
	}


	m_fOpeningAborted = false;

	m_closingmsg.Empty();

	m_iMediaLoadState = MLS_CLOSING;

	OnFilePostClosemedia();

	if(m_pGraphThread && s_fOpenedThruThread)
	{
		CAMEvent e;
		m_pGraphThread->PostThreadMessage(CGraphThread::TM_CLOSE, 0, (LPARAM)&e);
		e.Wait(); // either opening or closing has to be blocked to prevent reentering them, closing is the better choice
	}
	else
	{
		CloseMediaPrivate();
	}
	if(m_pMC){
		m_pMC = NULL;
	}

	UnloadExternalObjects();
}

//
// CGraphThread
//

IMPLEMENT_DYNCREATE(CGraphThread, CWinThread)

BOOL CGraphThread::InitInstance()
{
	AfxSocketInit();
	return SUCCEEDED(CoInitialize(0)) ? TRUE : FALSE;
}

int CGraphThread::ExitInstance()
{
	CoUninitialize();
	return __super::ExitInstance();
}

BEGIN_MESSAGE_MAP(CGraphThread, CWinThread)
	ON_THREAD_MESSAGE(TM_EXIT, OnExit)
	ON_THREAD_MESSAGE(TM_OPEN, OnOpen)
	ON_THREAD_MESSAGE(TM_CLOSE, OnClose)
END_MESSAGE_MAP()

void CGraphThread::OnExit(WPARAM wParam, LPARAM lParam)
{
	PostQuitMessage(0);
	if(CAMEvent* e = (CAMEvent*)lParam) e->Set();
}

void CGraphThread::OnOpen(WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame)
	{
		CAutoPtr<OpenMediaData> pOMD((OpenMediaData*)lParam);
		m_pMainFrame->OpenMediaPrivate(pOMD);
	}
}

void CGraphThread::OnClose(WPARAM wParam, LPARAM lParam)
{
	if(m_pMainFrame) m_pMainFrame->CloseMediaPrivate();
	if(CAMEvent* e = (CAMEvent*)lParam) e->Set();
}

afx_msg void CMainFrame::OnSubtitleDelay(UINT nID)
{
	if(m_pCAP) {
		int newDelay;
		int oldDelay = m_pCAP->GetSubtitleDelay();

		if(nID == ID_SUB_DELAY_DOWN)
			newDelay = oldDelay-AfxGetAppSettings().nSubDelayInterval;
		else
			newDelay = oldDelay+AfxGetAppSettings().nSubDelayInterval;

		CString str;
		str.Format(_T("主字幕延时已经设为： %d ms"), newDelay);
		SendStatusMessage(str, 5000);
		SetSubtitleDelay(newDelay);
	}
}

afx_msg void CMainFrame::OnSubtitleDelay2(UINT nID)
{
	if(m_pCAP) {
		int newDelay;
		int oldDelay = m_pCAP->GetSubtitleDelay2();

		if(nID == ID_SUB_DELAY_DOWN)
			newDelay = oldDelay-AfxGetAppSettings().nSubDelayInterval;
		else
			newDelay = oldDelay+AfxGetAppSettings().nSubDelayInterval;

		CString str;
		str.Format(_T("第二字幕延时已经设为： %d ms"), newDelay);
		SendStatusMessage(str, 5000);
		SetSubtitleDelay2(newDelay);
	}
}
afx_msg void CMainFrame::OnSubtitleMove(UINT nID)
{
	if(m_pCAP)
	{
		BOOL bSubChg1 = FALSE;
		BOOL bSubChg2 = FALSE;
		AppSettings& s = AfxGetAppSettings();
	
		if(nID >= ID_SUBMOVEUP && nID <= ID_SUBMOVERIGHT)
			bSubChg1 = TRUE;
		if(nID >= ID_SUB2MOVEUP && nID <= ID_SUB2MOVERIGHT)
			bSubChg2 = TRUE;

		CString str;

		switch(nID){
			case ID_SUBMOVEUP:
				s.nVerPos -= 2;
				if(s.nVerPos < 2){s.nVerPos = 2;}
				s.fOverridePlacement = true;
				str.Format(_T("主字幕高度已经设为：%d %%"), s.nVerPos);
				break;
			case  ID_SUBMOVEDOWN:
				s.nVerPos += 2;
				if(s.nVerPos > 98){s.nVerPos = 98;}
				s.fOverridePlacement = true;
				str.Format(_T("主字幕高度已经设为：%d %%"), s.nVerPos);
				break;
			case  ID_SUBMOVELEFT:
				break;
			case  ID_SUBMOVERIGHT:
				break;
			case  ID_SUB2MOVEUP:
				s.nVerPos2 -= 2;
				if(s.nVerPos2 < 2){s.nVerPos2 = 2;}
				s.fOverridePlacement2 = true;
				str.Format(_T("第二字幕高度已经设为：%d %%"), s.nVerPos2);
				break;
			case  ID_SUB2MOVEDOWN:
				s.nVerPos2 += 2;
				if(s.nVerPos2 > 98){s.nVerPos2 = 98;}
				s.fOverridePlacement2 = true;
				str.Format(_T("第二字幕高度已经设为：%d %%"), s.nVerPos2);
				break;
			case  ID_SUB2MOVELEFT:
				break;
			case  ID_SUB2MOVERIGHT:
				break;
		}
		

		if(bSubChg1)
			UpdateSubtitle(true); 
		if(bSubChg2)
			UpdateSubtitle2(true);
		
		//SendStatusMessage(str, 5000);
		
// 		if(bSubChg1 || bSubChg2)
// 			m_pCAP->Invalidate();
	}
}

afx_msg void CMainFrame::OnSubtitleFontChange(UINT nID)
{
	if(m_pCAP)
	{
		BOOL bSubChg1 = FALSE;
		BOOL bSubChg2 = FALSE;
		AppSettings& s = AfxGetAppSettings();
		CString str, str2;

		if(nID >= ID_SUBFONTUPBOTH && nID <= ID_SUBFONTDOWNBOTH)
			bSubChg1 = TRUE; bSubChg2 = TRUE;
		if(nID >= ID_SUB1FONTUP && nID <= ID_SUB1FONTDOWN)
			bSubChg1 = TRUE;
		if(nID >= ID_SUB2MOVEUP && nID <= ID_SUB2MOVERIGHT)
			bSubChg2 = TRUE;

		switch(nID){
			case ID_SUBFONTUPBOTH:
				
			case ID_SUB1FONTUP:
				s.subdefstyle.fontSize += 1;
				if(nID != ID_SUBFONTUPBOTH)
					break;
			case ID_SUB2FONTUP:
				s.subdefstyle2.fontSize += 1;
				
				break;
			case ID_SUBFONTDOWNBOTH:
				
			case ID_SUB1FONTDOWN:
				s.subdefstyle.fontSize -= 1;
				if(s.subdefstyle.fontSize < 3)
					s.subdefstyle.fontSize = 3;
				if(nID != ID_SUBFONTDOWNBOTH)
					break;
			case ID_SUB2FONTDOWN:
				s.subdefstyle2.fontSize -= 1;
				if(s.subdefstyle2.fontSize < 3)
					s.subdefstyle2.fontSize = 3;
				break;
		}

		if(bSubChg1){
			str.Format(_T("主字幕字体已经设为：%0.0f  "), s.subdefstyle.fontSize);
			UpdateSubtitle(true); 
		}
		if(bSubChg2){
			str2.Format(_T("第二字幕字体已经设为：%0.0f  "), s.subdefstyle.fontSize);
			UpdateSubtitle2(true);
		}
		SendStatusMessage(str + str2, 5000);

		// 		if(bSubChg1 || bSubChg2)
		// 			m_pCAP->Invalidate();
	}
}
void CMainFrame::OnSvpsubMenuenable()
{
	AppSettings& s = AfxGetAppSettings();
	s.autoDownloadSVPSub = !s.autoDownloadSVPSub;
}

void CMainFrame::OnUpdateSvpsubMenuenable(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!!(AfxGetAppSettings().autoDownloadSVPSub ));
}


void CMainFrame::OnVisitbbs()
{
	//Visit BBS
	ShellExecute(m_hWnd, _T("open"), _T("https://bbs.shooter.cn/forumdisplay.php?fid=6"), NULL, NULL, SW_SHOWDEFAULT);
}
#include "SearchSubDlg.h"
void CMainFrame::OnFileISDBSearch()
{
	CSearchSubDlg ssub;
	CSVPToolBox svpTool;
	CStringArray szaPathArr;
	svpTool.getVideoFileBasename(m_fnCurPlayingFile, &szaPathArr);
	if(szaPathArr.GetCount() >= 4){
		ssub.m_skeywords = szaPathArr.GetAt(3);
		
	}
	ssub.DoModal();
}
void CMainFrame::OnCheckDefaultPlayer(){
	CChkDefPlayer dlg_chkdefplayer;
	dlg_chkdefplayer.b_isDefaultPlayer();
	dlg_chkdefplayer.DoModal();
	
}
void CMainFrame::OnSendemail()
{
	ShellExecute(m_hWnd, _T("open"), _T("mailto:tomasen@gmail.com"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnVisitcontactinfo()
{
	
	ShellExecute(m_hWnd, _T("open"), _T("http://shooter.cn/splayer/contact.html"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnDonate()
{
	
	ShellExecute(m_hWnd, _T("open"), _T("http://shooter.cn/donate/"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnJointeam()
{
	ShellExecute(m_hWnd, _T("open"), _T("http://shooter.cn/splayer/join.html"), NULL, NULL, SW_SHOWDEFAULT);
}

void CMainFrame::OnSetsnapshotpath()
{
	TCHAR buff[MAX_PATH];
	AppSettings& s = AfxGetAppSettings();

	BROWSEINFO bi;
	bi.hwndOwner = m_hWnd;
	bi.pidlRoot = NULL;
	bi.pszDisplayName = buff;
	bi.lpszTitle = _T("选择截图默认保存文件夹");
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_VALIDATE | BIF_USENEWUI;
	bi.lpfn = BrowseCtrlCallback;
	bi.lParam = (LPARAM)(LPCTSTR)s.SnapShotPath;
	bi.iImage = 0; 

	LPITEMIDLIST iil;
	if(iil = SHBrowseForFolder(&bi))
	{
		if( SHGetPathFromIDList(iil, buff) )
			s.SnapShotPath = buff;
		return ;
	}

}

void CMainFrame::OnKillFocus(CWnd* pNewWnd)
{
	__super::OnKillFocus(pNewWnd);

	// TODO: Add your message handler code here
	RedrawNonClientArea();
}

void CMainFrame::OnChangebackground()
{
	// TODO: Add your command handler code here
	CAutoPtr<CPPageLogo> page(new CPPageLogo());
	CPropertySheet dlg(_T("界面背景设置..."), this);
	dlg.AddPage(page);
	dlg.DoModal() ;
}

void CMainFrame::OnSubsetfontboth()
{
	// TODO: Add your command handler code here
	LOGFONT lf;
	AppSettings& s = AfxGetAppSettings();
	
	lf <<= s.subdefstyle;
	int iCharset1 = s.subdefstyle.charSet;
	int iCharset2 = s.subdefstyle2.charSet;

	CFontDialog dlg(&lf, CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_FORCEFONTEXIST|CF_SCALABLEONLY|CF_EFFECTS);
	if(dlg.DoModal() == IDOK)
	{
		CString str(lf.lfFaceName);
		
		s.subdefstyle = lf;
		s.subdefstyle2 = lf;

		s.subdefstyle.charSet = iCharset1;
		s.subdefstyle2.charSet = iCharset2;
	
		UpdateSubtitle(true);
		UpdateSubtitle2(true);	
	}
}

void CMainFrame::OnDeletecurfile()
{
	// TODO: Add your command handler code here
	CString szMsg;
	CSVPToolBox svpTool;
	szMsg.Format(_T("删除当前文件：\r\n%s ?"),  m_fnCurPlayingFile);
	if(IDYES == AfxMessageBox(szMsg, MB_YESNO)){
		PostMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fnDelPending = m_fnCurPlayingFile;
		SetTimer(TIMER_DELETE_CUR_FILE, 1000, NULL);

		if(m_fFullScreen){
			ToggleFullscreen(true, false);
		}
	}
}

void CMainFrame::OnDelcurfolder()
{
	// TODO: Add your command handler code here
	CString szMsg;
	CSVPToolBox svpTool;
	CString szPath = svpTool.GetDirFromPath(m_fnCurPlayingFile);
	szMsg.Format(_T("删除当前文件夹：\r\n%s ?"), szPath);
	if(IDYES == AfxMessageBox(szMsg, MB_YESNO)){
		PostMessage(WM_COMMAND, ID_FILE_CLOSEMEDIA);
		fnDelPending = szPath;
		SetTimer(TIMER_DELETE_CUR_FOLDER, 1000, NULL);
		
		if(m_fFullScreen){
			ToggleFullscreen(true, false);
		}
	}
}

void CMainFrame::OnUpdateDeleteCurs(CCmdUI *pCmdUI)
{
	if( !IsSomethingLoaded()){
		pCmdUI->Enable(FALSE);
		switch(pCmdUI->m_nID){
			case ID_DELETECURFILE:
				pCmdUI->SetText(_T("当前文件"));
			break;
			case ID_DELCURFOLDER:
				pCmdUI->SetText(_T("当前文件夹"));
			break;
		}
	}else{
		pCmdUI->Enable(TRUE);
		CString szMText ;
		switch(pCmdUI->m_nID){
			case ID_DELETECURFILE:
				szMText.Format(_T("当前文件(%s)"), m_fnCurPlayingFile);
				break;
			case ID_DELCURFOLDER:
				CSVPToolBox svpTool;
				szMText.Format(_T("当前文件夹(%s)"), svpTool.GetDirFromPath(m_fnCurPlayingFile));
				break;
		}
		pCmdUI->SetText(szMText);
	}
}

void CMainFrame::OnToggleSPDIF(){
	AppSettings& s = AfxGetAppSettings();
	
	s.fbUseSPDIF = !s.fbUseSPDIF;
	if(IsSomethingLoaded()){
		ReRenderOrLoadMedia();
	}
	
}
void CMainFrame::OnUpdateToggleSPDIF(CCmdUI *pCmdUI){

	AppSettings& s = AfxGetAppSettings();
	pCmdUI->SetCheck(s.fbUseSPDIF);
}
void CMainFrame::OnSetAutoLoadSubtitle(){
	AppSettings& s = AfxGetAppSettings();
	s.fAutoloadSubtitles2 = !s.fAutoloadSubtitles2;
}
void CMainFrame::OnUpdateSetAutoLoadSubtitle(CCmdUI *pCmdUI){
	AppSettings& s = AfxGetAppSettings();
	pCmdUI->SetCheck(s.fAutoloadSubtitles2 );
}
void CMainFrame::OnDebugreport()
{
	CStringArray szaReport;

	if(m_iMediaLoadState == MLS_LOADED)
	{
		CSVPToolBox svpTool;
		svpTool.GetGPUString(&szaReport);
		
		BeginEnumFilters(pGB, pEF, pBF)
		{
			CString name(GetFilterName(pBF));
		
			CLSID clsid = GetCLSID(pBF);
			if(clsid == CLSID_AVIDec)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					DWORD c = ((VIDEOINFOHEADER*)mt.pbFormat)->bmiHeader.biCompression;
					switch(c)
					{
					case BI_RGB: name += _T(" (RGB)"); break;
					case BI_RLE4: name += _T(" (RLE4)"); break;
					case BI_RLE8: name += _T(" (RLE8)"); break;
					case BI_BITFIELDS: name += _T(" (BITF)"); break;
					default: name.Format(_T("%s (%c%c%c%c)"), 
								 CString(name), (TCHAR)((c>>0)&0xff), (TCHAR)((c>>8)&0xff), (TCHAR)((c>>16)&0xff), (TCHAR)((c>>24)&0xff)); break;
					}
				}
			}
			else if(clsid == CLSID_ACMWrapper)
			{
				CComPtr<IPin> pPin = GetFirstPin(pBF);
				AM_MEDIA_TYPE mt;
				if(pPin && SUCCEEDED(pPin->ConnectionMediaType(&mt)))
				{
					WORD c = ((WAVEFORMATEX*)mt.pbFormat)->wFormatTag;
					name.Format(_T("%s (0x%04x)"), CString(name), (int)c);
				}
			}
			else if(clsid == __uuidof(CTextPassThruFilter) || clsid == __uuidof(CNullTextRenderer)
				|| clsid == GUIDFromCString(_T("{48025243-2D39-11CE-875D-00608CB78066}"))) // ISCR
			{
				// hide these
				continue;
			}
			szaReport.Add(name + _T(" ") + CStringFromGUID(clsid));
			CComQIPtr<ISpecifyPropertyPages> pSPP = pBF;
			BeginEnumPins(pBF, pEP, pPin)
			{
				CString name = GetPinName(pPin);
				name.Replace(_T("&"), _T("&&"));

				if(pSPP = pPin)
				{
					CAUUID caGUID;
					caGUID.pElems = NULL;
					if(SUCCEEDED(pSPP->GetPages(&caGUID)) && caGUID.cElems > 0)
					{
						m_pparray.Add(pPin);
						
						if(caGUID.pElems) CoTaskMemFree(caGUID.pElems);

						
					}
				}
				szaReport.Add(CString(_T(">>")) +name);
			}
			EndEnumPins

			CComQIPtr<IAMStreamSelect> pSS = pBF;
			if(pSS)
			{
				DWORD nStreams = 0, flags, group, prevgroup = -1;
				LCID lcid;
				WCHAR* wname = NULL;
				CComPtr<IUnknown> pObj, pUnk;

				pSS->Count(&nStreams);



				for(DWORD i = 0; i < nStreams; i++, pObj = NULL, pUnk = NULL)
				{
					m_ssarray.Add(pSS);

					flags = group = 0;
					wname = NULL;
					pSS->Info(i, NULL, &flags, &lcid, &group, &wname, &pObj, &pUnk);


					if(!wname) 
					{
						CStringW stream(L"Unknown Stream");
						wname = (WCHAR*)CoTaskMemAlloc((stream.GetLength()+3+1)*sizeof(WCHAR));
						swprintf(wname, L"%s %d", stream, min(i+1,999));
					}

					CString name(wname);
					name.Replace(_T("&"), _T("&&"));

					szaReport.Add(CString(_T(">>>")) + name);

					CoTaskMemFree(wname);
				}

				if(nStreams == 0) pSS.Release();
			}

			
		}
		EndEnumFilters

		CString szBuf;
		szBuf.Format(_T("SPlayer Build %s ") , SVP_REV_STR);
		szaReport.Add(szBuf);

		OSVERSIONINFO osver;

		osver.dwOSVersionInfoSize = sizeof( OSVERSIONINFO );

		::GetVersionEx( &osver ) ;
		szBuf.Format(_T("OS Ver %s %d.%d ")  , osver.szCSDVersion, osver.dwMajorVersion , osver.dwMinorVersion );
		szaReport.Add(szBuf);

		szBuf = _T("CPU ");
		if(g_cpuid.m_flags & CCpuID::mmx)
			szBuf.Append(_T(" MMX"));
		
		if(g_cpuid.m_flags & CCpuID::sse2)
			szBuf.Append(_T(" SSE2"));

		if(g_cpuid.m_flags & CCpuID::ssemmx)
			szBuf.Append(_T(" SSEMMX"));

		szaReport.Add(szBuf);

		CString szReport = svpTool.Implode(_T("\r\n-------------\r\n"), &szaReport);
		
		CInfoReport CIR;
		CIR.m_rptText2 = szReport;
		CIR.DoModal();

		//AfxMessageBox(szReport);
	}
}

void CMainFrame::OnUpdateDebugreport(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsSomethingLoaded());
}
