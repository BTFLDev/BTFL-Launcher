#ifndef PATCHNOTESWINODW_H_
#define PATCHNOTESWINODW_H_
#pragma once

#include <wx\wx.h>
#include <wx\scrolwin.h>
#include <wx\richtext\richtextctrl.h>

#include "BaseClasses.h"
#include "Scrollbar.h"
#include "StateManaging.h"

class MainFrame;

///////////////////////////////////////////////////////////////////////
/////////////////////////// HyperlinkPanel ////////////////////////////
///////////////////////////////////////////////////////////////////////


class HyperlinkPanel : public wxPanel
{
	wxBitmap m_bitmap;
	wxString m_url;

	int m_padding = 1;
	int m_bgx = 0, m_bgy = 0;
	double m_bgScale = 0;
	double m_bgRatio = 0;

	bool m_isHovering = false;

public:
	HyperlinkPanel(wxWindow* parent, const wxString& url, const wxBitmap& bmp, const wxSize& size = wxDefaultSize,
		const wxPoint& pos = wxDefaultPosition, long style = wxBORDER_NONE);

	void RecalculateSelf();

	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent& event);

	void OnLeftDown(wxMouseEvent& event);
	void OnEnterWindow(wxMouseEvent& event);
	void OnLeaveWindow(wxMouseEvent& event);

	wxDECLARE_EVENT_TABLE();
};


///////////////////////////////////////////////////////////////////////
///////////////////////////// LeftSidebar /////////////////////////////
///////////////////////////////////////////////////////////////////////


class LeftSidebarRTC : public wxPanel, public RTCFileLoader {
public:
	LeftSidebarRTC(wxWindow* parent, const wxString& fileToLoad);
};


class LeftSidebar : public wxPanel
{
	MainFrame* m_mainFrame = nullptr;
	LeftSidebarRTC* m_welcomePage = nullptr,
		* m_patchNotesPage = nullptr;

	wxStaticText* m_welcomeLabel = nullptr,
		* m_patchNotesLabel = nullptr;

	bool m_bIsWelcomeShown = true;
	
public:
	LeftSidebar(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE);

	void SetState(btfl::LauncherState state);

	void ShowWelcome();
	void ShowPatchNotes();

	void OnPaint(wxPaintEvent& event);
	void OnMove(wxMouseEvent& event);

	DECLARE_EVENT_TABLE()
};

#endif