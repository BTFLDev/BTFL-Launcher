#ifndef MAINPANEL_H_
#define MAINPANEL_H_
#pragma once

#include <wx\wx.h>
#include <wx\filename.h>
#include <wx\wxsf\wxShapeFramework.h>

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
	TIMER_Gauge
};

enum GaugeResult
{
	GAUGE_VerifyValid,
	GAUGE_VerifyInvalid,

	GAUGE_Invalid
};

class MainPanel : public BackgroundImageCanvas
{
private:
	MainFrame* m_mainFrame = nullptr;

	wxBitmap m_logo;
	int m_logox = 0, m_logoy = 0;

	wxString m_fileLabel{ "No ISO selected..." }, m_fileDesc{ "View Installation Guide" };
	wxFont m_fileLabelFont{ wxFontInfo(12).FaceName("Times New Roman") },
		m_fileDescFont{ wxFontInfo(10).FaceName("Times New Roman") };
	wxRect m_fileDescRect;
	bool m_isHoveringViewGuide = false;
	bool m_isHoveringFileCont = false;
	wxColour m_fileDescColour{ 52, 199, 226 };
	wxBitmap m_fileContainer, m_fileBmp;

	double m_fileBmpScale = 1.0;
	int m_xFLabel = -1, m_yFLabel = -1,
		m_xFDesc = -1, m_yFDesc = -1,
		m_xFCont = -1, m_yFCont = -1,
		m_xFBmp = -1, m_yFBmp = -1;

	TransparentButton* m_mainButton = nullptr,
		* m_configButton = nullptr;
	FrameButtons* m_frameButtons = nullptr;

	CustomGauge* m_gauge = nullptr;
	wxSFTextShape* m_gaugeLabel = nullptr,
		* m_gaugeProgress = nullptr;
	wxTimer m_gaugeTimer;
	std::atomic<int> m_nextGaugeValue;
	wxString m_nextGaugeLabel;
	GaugeResult m_gaugeResult = GAUGE_Invalid;

public:
	MainPanel(wxSFDiagramManager* manager,
		MainFrame* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = wxBORDER_NONE);

	void SetState(btfl::LauncherState state);

	// Put everything in place, be it buttons, labels, bitmaps, etc.
	void RepositionAll();

	// Start the iso verification process. Only works if m_iso is valid.
	void VerifyIso();

	void CreateGauge();
	void DestroyGauge();

	// Updates the gauge and refreshes the screen. "Message" is assiged to m_gaugeLabel.
	void UpdateGauge(int currentUnit, const wxString& message);

	// Called every 1ms so that the program checks whether it needs to update the gauge
	// with the value specified in m_nextGaugeValue.
	void OnGaugeTimer(wxTimerEvent& event);

	// Called when OnGaugeTimer receives a m_nextGaugeValue that results in a 100 percent
	// completion in the gauge. It then takes into account the current value in m_gaugeResult
	// to know what action should be taken as a result of the completion.
	void OnGaugeFinished();

	void DoSelectIso();

	// wxSF event handlers.
	void OnFrameButtons(wxSFShapeMouseEvent& event);
	void OnSelectIso(wxSFShapeMouseEvent& event);
	void OnVerifyIso(wxSFShapeMouseEvent& event);
	void OnSettings(wxSFShapeMouseEvent& event);

	virtual void OnSize(wxSizeEvent& event) override;
	virtual void OnMouseMove(wxMouseEvent& event) override;
	virtual void OnLeftDown(wxMouseEvent& event) override;

	virtual void DrawForeground(wxDC& dc, bool fromPaint) override;

	wxDECLARE_EVENT_TABLE();
};

#endif