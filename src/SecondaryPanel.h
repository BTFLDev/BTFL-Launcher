#ifndef SECONDARYPANEL_H_
#define SECONDARYPANEL_H_
#pragma once

#include "BaseClasses.h"
#include "PatchNotes.h"
#include "FrameButtons.h"
#include "TransparentButton.h"
#include "StateManaging.h"

#include <wx\wxsf\wxShapeFramework.h>
#include <wx\richtext\richtextctrl.h>

class SecondaryPanel;
class MainFrame;

class DisclaimerPanel : public ReadOnlyRTC
{
private:
	SecondaryPanel* m_seconPanel = nullptr;
	int m_bgx = 0, m_bgy = 0;

public:
	DisclaimerPanel(SecondaryPanel* parent,
		wxWindowID id = -1,
		const wxString& value = wxEmptyString,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE);

	inline void SetBackgroundX(int x) { m_bgx = x; }
	inline void SetBackgroundY(int y) { m_bgy = y; }

	virtual void PaintBackground(wxDC& dc) override;
};

enum
{
	BUTTON_Back = wxID_HIGHEST,

	BUTTON_DisclaimerAgree,
	BUTTON_DisclaimerAgreeVerify,

	BUTTON_AutoUpdate,
	BUTTON_Uninstall
};

class CheckboxShape : public wxSFRoundRectShape
{
private:
	bool m_bIsChecked = false;

public:
	XS_DECLARE_CLONABLE_CLASS(CheckboxShape);
	CheckboxShape();

	bool IsChecked() { return m_bIsChecked; }
	void SetState(bool isChecked);

	virtual void OnLeftClick(const wxPoint& pos) override;
	virtual void OnMouseEnter(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_CLOSED_HAND); }
	virtual void OnMouseLeave(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_DEFAULT); }
};

class SecondaryPanel : public BackgroundImageCanvas
{
private:
	MainFrame* m_mainFrame = nullptr;

	FrameButtons* m_frameButtons = nullptr;
	wxSFTextShape* m_title = nullptr;
	wxSFBitmapShape* m_backArrow = nullptr;

	wxBitmap m_topSeparator;

	const int TOP_SPACE = 50;
	const int BOTTOM_SPACE = 100;

	wxBoxSizer* m_verSizer = nullptr;

	DisclaimerPanel* m_disclaimer = nullptr;
	TransparentButton* m_disDecline = nullptr,
		* m_disAgree = nullptr;

	/////////////////// Settings ///////////////////////
	btfl::Settings m_settings;

	wxSFFlexGridShape* m_mainSettingsGrid = nullptr;
	wxSFTextShape* m_installPath = nullptr;
	CheckboxShape* m_autoUpdate = nullptr;
	TransparentButton* m_uninstallButton = nullptr;

	bool m_bIsHoveringInstallPath = false;

	wxBitmap m_installPathBmp{ "Assets\\Icon\\Folder@2x.png", wxBITMAP_TYPE_PNG };
	float m_fInstallPathBmpScale = 1.0;
	wxPoint m_installPathBmpPos;

public:
	SecondaryPanel(wxSFDiagramManager* manager, MainFrame* parent);

	void ShowDisclaimer();
	void ShowSettings();

	void SelectInstallPath();

	void SetSettings(const btfl::Settings& settings);
	btfl::Settings GetSettings() { return m_settings;  }

	void OnFrameButtons(wxSFShapeMouseEvent& event);
	void OnAcceptDisclaimer(wxSFShapeMouseEvent& event);

	void OnAutoUpdateChange(wxSFShapeMouseEvent& event);

	void RepositionAll();
	void DeleteSettingsShapes();
	void SetShapeStyle(wxSFShapeBase* shape);
	virtual void DrawForeground(wxDC& dc, bool fromPaint) override;

	virtual void OnSize(wxSizeEvent& event) override;
	virtual void OnLeftDown(wxMouseEvent& event) override;
	virtual void OnMouseMove(wxMouseEvent& event) override;

	wxDECLARE_EVENT_TABLE();

private:
	void ReloadSettings();
	void DoSaveSettings() { btfl::SaveSettings(m_settings); }
};
#endif