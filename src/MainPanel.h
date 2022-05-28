#ifndef MAINPANEL_H_
#define MAINPANEL_H_
#pragma once

#include <wx\wx.h>
#include <wx\filename.h>
#include <wx\wxsf\wxShapeFramework.h>
#include <wx/thread.h>

#include <atomic>

#include "BaseClasses.h"
#include "PatchNotes.h"
#include "FrameButtons.h"
#include "TransparentButton.h"
#include "StateManaging.h"

class MainFrame;
class CustomGauge;

class CustomGauge : public wxSFRoundRectShape
{
private:
	int m_units = 100;
	int m_currentUnit = 0;

	wxColour m_barColour = { 52, 199, 226 };

public:
	CustomGauge() = default;
	inline CustomGauge(const wxRealPoint& pos,
		const wxRealPoint& size, 
		int maxUnits,
		double radius,
		wxSFDiagramManager* manager) 
		: wxSFRoundRectShape(pos, size, radius, manager), m_units(maxUnits) 
	{
		SetStyle(sfsHOVERING);
	}

	bool UpdateValue(int currentUnit);

	inline int GetCurrentUnit() { return m_currentUnit; }
	inline int GetCurrentPercent() { return (m_currentUnit * 100) / m_units; }

	void DrawBar(wxDC& dc);

	virtual void DrawNormal(wxDC& dc) override;
	virtual void DrawHover(wxDC& dc) override;
};

enum
{
	BUTTON_Settings,
	TIMER_Gauge,

	WEB_Install,
	WEB_InstallEssence,
	WEB_LookForLauncherUpdate,
	WEB_LookForGameUpdate,
	WEB_LookForEssenceGameUpdate,
	WEB_GetEssencePassword,

	TIMER_BgChange,
};

enum GaugeResult
{
	GAUGE_VerifyValid,
	GAUGE_VerifyInvalid,

	GAUGE_InstallationSuccessful,
	GAUGE_InstallationUnsuccessful,

	GAUGE_EssenceInstallationSuccessful,
	GAUGE_EssenceInstallationUnsuccessful,

	GAUGE_Invalid
};

class MainPanel : public BackgroundImageCanvas
{
private:
	MainFrame* m_mainFrame = nullptr;

	wxArrayInt m_vShownBackgrounds;
	wxBitmap m_logo;
	int m_logox = 0, m_logoy = 0;

	wxString m_fileLabel{ "No ISO selected..." }, m_fileDesc{ "View Installation Guide" };
	wxFont m_fileLabelFont{ wxFontInfo(13).FaceName("Lora") },
		m_fileDescFont{ wxFontInfo(10).FaceName("Lora") };
	wxRect m_viewGuideRect;
	bool m_isHoveringViewGuide = false;
	bool m_isHoveringFileCont = false;
	wxColour m_fileDescColour{ 52, 199, 226 };
	wxBitmap m_fileContainer, m_fileBmp;

	double m_fileBmpScale = 1.0;
	wxPoint m_fileLabelPos{ -1, -1 },
		m_fileDescPos{ -1, -1 },
		m_fileContPos{ -1, -1 },
		m_fileBmpPos{ -1, -1 };

	TransparentButton* m_mainButton = nullptr,
		* m_configButton = nullptr,
		* m_updateButton = nullptr,
		* m_essenceMainButton = nullptr,
		* m_essenceUpdateButton = nullptr;
	FrameButtons* m_frameButtons = nullptr;

	bool m_bShouldDeleteUpdateBtn = false;
	bool m_bShouldDeleteEssenceUpdateBtn = false;

	CustomGauge* m_gauge = nullptr;
	wxSFTextShape* m_gaugeLabel = nullptr,
		* m_gaugeProgress = nullptr;
	wxTimer m_gaugeTimer;
	std::atomic<int> m_nextGaugeValue;
	wxMutex m_nextGaugeValueMutex;
	wxString m_nextGaugeLabel;
	std::atomic<GaugeResult> m_gaugeResult = GAUGE_Invalid;

	wxWebRequest m_webRequest,
		m_webRequestEssence,
		m_latestLauncherVersionRequest,
		m_latestGameVersionRequest,
		m_latestEssenceGameVersionRequest,
		m_essencePasswordRequest;

	bool m_bIsDraggingFrame = false;
	wxRect m_dragSafeArea;
	wxPoint m_dragStartMousePos, m_dragStartFramePos;

	wxPoint m_versionLabelPos;
	wxFont m_versionFont{ wxFontInfo(8).FaceName("Lora") };

	wxTimer m_bgChangeTimer;

	wxColour m_bgFadeColour{ 0,0,0,0 };
	bool m_bDoFadeOut = false, m_bDoFadeIn = false;
	bool m_bCanChangeBackground = true;

public:
	MainPanel(wxSFDiagramManager* manager,
		MainFrame* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE);

	void SetState(btfl::LauncherState state);
	void SetEssenceState(btfl::LauncherEssenceState state);
	
	void CreateEssencesButtonsIfNeeded();
	void DestroyEssencesButtonsIfNeeded();

	// Put everything in place, be it buttons, labels, bitmaps, etc.
	void RepositionAll();

	// Start the iso verification process. Only works if m_iso is valid.
	void VerifyIso();

	void CreateGauge();
	void DestroyGauge();

	void CreateUpdateButton();
	void CreateEssenceUpdateButton();

	void DestroyButton(TransparentButton*& pButton);

	// Updates the gauge and refreshes the screen. "Message" is assiged to m_gaugeLabel.
	void UpdateGauge(int currentUnit, const wxString& message, bool refresh);

	// Called every 1ms so that the program checks whether it needs to update the gauge
	// with the value specified in m_nextGaugeValue.
	void OnGaugeTimer(wxTimerEvent& event) { DoCheckGauge(true); }
	void DoCheckGauge(bool refresh);

	// Called when OnGaugeTimer receives a m_nextGaugeValue that results in a 100 percent
	// completion in the gauge. It then takes into account the current value in m_gaugeResult
	// to know what action should be taken as a result of the completion.
	void OnGaugeFinished();

	void DoSelectIso();
	void DoInstallGame();
	void DoInstallEssenceGame();

	void DoLookForLauncherUpdates();
	void DoLookForGameUpdates();
	void DoLookForEssenceGameUpdates();
	void DoCheckEssencePassword();

	void PerformWebRequest(
		wxWebRequest& webRequestObj,
		const wxString& file,
		long id,
		wxWebRequest::Storage storage,
		const wxString& tempDir = "",
		const wxString& failMessage = ""
	);
	void OnWebRequestState(wxWebRequestEvent& event);
	void HandleInstallWebRequest(wxWebRequestEvent& event);
	void HandleLauncherUpdateWebRequest(wxWebRequestEvent& event);
	void HandleGameUpdateWebRequest(wxWebRequestEvent& event);
	void HandleEssenceGameUpdateWebRequest(wxWebRequestEvent& event);
	void HandleEssencePasswordWebRequest(wxWebRequestEvent& event);

	void UnzipGameFiles(const wxString& filePath, const wxString& fileName, GaugeResult onSuccess, GaugeResult onFailure);

	void ChangeToRandomBgImage();

	void PromptForBecomingEssence();

	// wxSF event handlers.
	void OnFrameButtons(wxSFShapeMouseEvent& event);
	void OnSelectIso(wxSFShapeMouseEvent& event);
	void OnVerifyIso(wxSFShapeMouseEvent& event);
	void OnInstall(wxSFShapeMouseEvent& event);
	void OnPlay(wxSFShapeMouseEvent& event);
	void OnSettings(wxSFShapeMouseEvent& event);

	void OnInstallEssence(wxSFShapeMouseEvent& event);
	void OnPlayEssence(wxSFShapeMouseEvent& event);

	virtual void OnSize(wxSizeEvent& event) override;
	virtual void OnMouseMove(wxMouseEvent& event) override;
	virtual void OnLeftDown(wxMouseEvent& event) override;
	virtual void OnLeftUp(wxMouseEvent& event) override;

	void OnBgChangeTimer(wxTimerEvent& event);
	virtual void DoAnimateBackground(bool refresh) override;
	virtual bool ShouldStopAnimatingBackground() override { return !m_bIsBgPosResetting && !m_bDoFadeOut && !m_bDoFadeIn; }

	virtual void DrawBackground(wxDC& dc, bool fromPaint) override;
	virtual void DrawForeground(wxDC& dc, bool fromPaint) override;

	inline void OnMouseCaptureLost(wxMouseCaptureLostEvent& evt)
	{
		m_bIsDraggingFrame = false;
	}

	wxDECLARE_EVENT_TABLE();
};

#endif