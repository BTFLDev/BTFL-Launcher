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
	BUTTON_CloseOnGameLaunch,
	BUTTON_Uninstall
};

class CheckboxShape : public wxSFCircleShape
{
private:
	bool m_bIsChecked = false;

	static wxBitmap m_colossusEyeBlue;

	double m_dBitmapScaleX = 1.0;
	double m_dBitmapScaleY = 1.0;

	static bool m_bIsInitialized;

public:
	XS_DECLARE_CLONABLE_CLASS(CheckboxShape);
	CheckboxShape();

	static void Initialize();

	bool IsChecked() { return m_bIsChecked; }
	void SetState(bool isChecked);

	void UpdateBitmapScale();

	virtual void Draw(wxDC& dc, bool children = true) override;

	virtual void OnLeftClick(const wxPoint& pos) override;
	virtual void OnMouseEnter(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_CLOSED_HAND); }
	virtual void OnMouseLeave(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_DEFAULT); }
};

class SecondaryPanel : public BackgroundImageCanvas, public RTCFileLoader
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

	TransparentButton* m_disDecline = nullptr,
		* m_disAgree = nullptr;

	/////////////////// Settings ///////////////////////
	btfl::Settings m_settings;

	wxSFFlexGridShape* m_mainSettingsGrid = nullptr;
	wxSFTextShape* m_installPath = nullptr;
	CheckboxShape* m_autoUpdate = nullptr,
		* m_closeSelfOnGameLaunch = nullptr;
	TransparentButton* m_uninstallButton = nullptr;

	bool m_bIsHoveringInstallPath = false;

	wxBitmap m_installPathBmp{ "Assets/Icon/Folder@2x.png", wxBITMAP_TYPE_PNG };
	float m_fInstallPathBmpScale = 1.0;
	wxPoint m_installPathBmpPos;

	bool m_bIsDraggingFrame = false;
	wxRect m_dragSafeArea;
	wxPoint m_dragStartMousePos, m_dragStartFramePos;

public:
	SecondaryPanel(wxSFDiagramManager* manager, MainFrame* parent);

	void ShowDisclaimer();
	void ShowSettings();

	virtual void OnFileLoaded() override;

	void SelectInstallPath();

	void SetSettings(const btfl::Settings& settings);
	btfl::Settings GetSettings() { return m_settings;  }

	void OnFrameButtons(wxSFShapeMouseEvent& event);
	void OnAcceptDisclaimer(wxSFShapeMouseEvent& event);

	void OnAutoUpdateChange(wxSFShapeMouseEvent& event);
	void OnCloseOnGameLaunchChange(wxSFShapeMouseEvent& event);

	void OnUninstall(wxSFShapeMouseEvent& event);

	void RepositionAll();
	void DeleteSettingsShapes();
	void SetShapeStyle(wxSFShapeBase* shape);
	void LayoutSelf();

	virtual void DrawForeground(wxDC& dc, bool fromPaint) override;

	virtual void OnSize(wxSizeEvent& event) override;
	virtual void OnLeftDown(wxMouseEvent& event) override;
	virtual void OnLeftUp(wxMouseEvent& event) override;
	virtual void OnMouseMove(wxMouseEvent& event) override;

	inline void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
	{
		m_bIsDraggingFrame = false;
	}

	wxDECLARE_EVENT_TABLE();

private:
	void ReloadSettings();
	void DoSaveSettings() { btfl::SaveSettings(m_settings); }
};
#endif