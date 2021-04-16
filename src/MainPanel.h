#ifndef MAINPANEL_H_
#define MAINPANEL_H_
#pragma once

#include <wx\wx.h>
#include <wx\wxsf\wxShapeFramework.h>

#include "PatchNotes.h"

class MainPanel;

class TransparentButton : public wxSFRoundRectShape {
protected:
	MainPanel* m_parent = nullptr;
	
	wxString m_label;
	wxFont m_font;
	wxColour m_textColour{ 255,255,255 };

	int m_xPadding = 30, m_yPadding = 15;
	int m_xLabel = 30;

	wxBitmap* m_bitmap = nullptr;
	double m_bmpScale = 1.0, m_bmpRatio = 1.0;
	int m_xBitmap = 30, m_yBitmap;

	bool m_isEnabled = true;

public:
	XS_DECLARE_CLONABLE_CLASS(TransparentButton);
	TransparentButton() = default;
	TransparentButton(const wxString& label, const wxRealPoint& pos, const wxRealPoint& size,
		double radius, wxSFDiagramManager* manager);
	TransparentButton(const TransparentButton& other);

	virtual ~TransparentButton();

	void Enable(bool enable);

	void SetLabel(const wxString& label);
	void SetFont(const wxFont& font);
	void SetBitmap(const wxBitmap& bmp);

	void SetPadding(int x, int y);
	void GetPadding(int* x, int* y);
	void RecalculateSelf(const wxSize& soloBmpSize = wxDefaultSize);

	void DrawContent(wxDC& dc, bool isHovering);

	virtual void DrawNormal(wxDC& dc);
	virtual void DrawHover(wxDC& dc);

	inline virtual void OnMouseEnter(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_CLOSED_HAND); }
	inline virtual void OnMouseLeave(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_DEFAULT); }
};


/////////////////////////////////////////////////////////////////////
//////////////////////////// MainPanel //////////////////////////////
/////////////////////////////////////////////////////////////////////

enum {
	BUTTON_Help,
	BUTTON_Minimize,
	BUTTON_Close,

	BUTTON_SelectIso,
	BUTTON_Install,
	BUTTON_Play
};

class MainPanel : public wxSFShapeCanvas {
private:
	wxBitmap m_background;
	wxBitmap m_logo;
	int m_bgx = 0, m_bgy = 0,
		m_bgxoffset = 0, m_bgyoffset = 0;
	int m_logox = 0, m_logoy = 0;
	double m_bgRatio, m_bgScale;
	const int MAX_BG_OFFSET = 20;

	wxString m_fileLabel{ "No ISO selected..." }, m_fileDesc{"View Installation Guide"};
	wxFont m_fileLabelFont{ wxFontInfo(12).FaceName("Times New Roman") },
		m_fileDescFont{ wxFontInfo(10).FaceName("Times New Roman") };
	wxRect m_fileDescRect;
	bool m_isHoveringFileDesc = false;
	wxColour m_fileDescColour{ 52, 199, 226 };
	wxBitmap m_fileContainer, m_fileBmp;

	double m_fileBmpScale = 1.0;
	int m_xFLabel = -1, m_yFLabel = -1,
		m_xFDesc = -1, m_yFDesc = -1,
		m_xFCont = -1, m_yFCont = -1,
		m_xFBmp = -1, m_yFBmp = -1;

	TransparentButton* m_mainButton = nullptr,
		* m_configButton = nullptr;
	wxSFGridShape* m_frameButtons = nullptr;

	wxPoint m_mousePosOnEnter{ 0,0 };
	wxTimer m_bgAnimTimer;

public:
	MainPanel(wxSFDiagramManager* manager, 
		wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE);

	inline const wxBitmap& GetBackgroundBitmap() { return m_background; }
	void RepositionAll();

	void OnFrameButtons(wxSFShapeMouseEvent& event);
	void OnSelectIso(wxSFShapeMouseEvent& event);

	void OnSize(wxSizeEvent& event);
	virtual void OnMouseMove(wxMouseEvent& event) override;

	virtual void OnUpdateVirtualSize(wxRect& rect) override;
	virtual void DrawBackground(wxDC& dc, bool fromPaint) override;
	virtual void DrawForeground(wxDC& dc, bool fromPaint) override;

	void OnEnterWindow(wxMouseEvent& event);
	void OnLeaveWindow(wxMouseEvent& event);

	void OnBgAnimTimer(wxTimerEvent& event);

	wxDECLARE_EVENT_TABLE();
};

#endif