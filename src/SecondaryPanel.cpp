#include "SecondaryPanel.h"
#include "Scrollbar.h"
#include "MainFrame.h"

#include "wxmemdbg.h"

DisclaimerPanel::DisclaimerPanel(SecondaryPanel* parent,
	wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style) : ReadOnlyRTC(parent, id, value, pos, size, style)
{
	m_seconPanel = parent;
}

void DisclaimerPanel::PaintBackground(wxDC& dc)
{
	double scale = m_seconPanel->GetBackgroundScale();
	int yo;
	CalcUnscrolledPosition(0, 0, nullptr, &yo);

	dc.SetUserScale(scale, scale);
	dc.DrawBitmap(m_seconPanel->GetBackgroundBitmap(), m_bgx, m_bgy + ((double)yo / scale));
	dc.SetUserScale(1.0, 1.0);

	wxSize virtualSize = GetVirtualSize();
	int halfx = virtualSize.x / 2;

	wxColour grey(150, 150, 150);
	dc.GradientFillLinear(wxRect(0, virtualSize.y - 1, halfx, 1), *wxBLACK, grey);
	dc.GradientFillLinear(wxRect(halfx, virtualSize.y - 1, halfx, 1), grey, *wxBLACK);
}


/////////////////////////////////////////////////////////////////////////
//////////////////////////// SecondaryPanel /////////////////////////////
/////////////////////////////////////////////////////////////////////////

XS_IMPLEMENT_CLONABLE_CLASS(CheckboxShape, wxSFRoundRectShape);

CheckboxShape::CheckboxShape() : wxSFRoundRectShape()
{
	SetState(false);
	SetRadius(2.0);

	AddStyle(sfsEMIT_EVENTS);

	RemoveStyle(sfsHIGHLIGHTING);
	RemoveStyle(sfsPOSITION_CHANGE);
	RemoveStyle(sfsPARENT_CHANGE);
	RemoveStyle(sfsSIZE_CHANGE);
	RemoveStyle(sfsSHOW_HANDLES);
}

void CheckboxShape::SetState(bool isChecked)
{
	if ( isChecked )
	{
		SetFill(wxBrush(*wxWHITE));
		SetBorder(wxPen(wxColour(50, 180, 220), 2));
	}
	else
	{
		SetFill(*wxTRANSPARENT_BRUSH);
		SetBorder(wxPen(*wxWHITE, 2 ));
	}

	m_bIsChecked = isChecked;
	Refresh();
}

void CheckboxShape::OnLeftClick(const wxPoint& pos)
{
	SetState(!m_bIsChecked);
	wxSFRoundRectShape::OnLeftClick(pos);
}

wxBEGIN_EVENT_TABLE(SecondaryPanel, BackgroundImageCanvas)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Help, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Minimize, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Close, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Back, SecondaryPanel::OnFrameButtons)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_DisclaimerAgree, SecondaryPanel::OnAcceptDisclaimer)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_DisclaimerAgreeVerify, SecondaryPanel::OnAcceptDisclaimer)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_AutoUpdate, SecondaryPanel::OnAutoUpdateChange)

wxEND_EVENT_TABLE()

SecondaryPanel::SecondaryPanel(wxSFDiagramManager* manager, MainFrame* parent) :
	BackgroundImageCanvas(manager, parent, -1), RTCFileLoader(this)
{
	m_mainFrame = parent;
	m_sFileToLoad = "disclaimer.xml";

	MAX_BG_OFFSET = 0;
	m_background.LoadFile("Assets/Background/Subpage@2x.png", wxBITMAP_TYPE_PNG);
	m_bgRatio = (double)m_background.GetWidth() / m_background.GetHeight();

	manager->AcceptShape("All");

	m_topSeparator.LoadFile("Assets/Spacer/Full Window Width@2x.png", wxBITMAP_TYPE_PNG);

	m_backArrow = (wxSFBitmapShape*)manager->AddShape(CLASSINFO(wxSFBitmapShape), false);
	m_backArrow->CreateFromFile("Assets/Icon/Arrow Left.png", wxBITMAP_TYPE_PNG);
	m_backArrow->SetId(BUTTON_Back);
	m_backArrow->SetStyle(
		wxSFShapeBase::STYLE::sfsHOVERING |
		wxSFShapeBase::STYLE::sfsEMIT_EVENTS
	);

	m_title = (wxSFTextShape*)manager->AddShape(CLASSINFO(wxSFTextShape), false);
	m_title->SetTextColour(wxColour(255, 255, 255));
	m_title->SetFont(wxFontInfo(15).Bold().FaceName("Times New Roman"));
	m_title->SetStyle(0);

	m_frameButtons = (FrameButtons*)manager->AddShape(CLASSINFO(FrameButtons), false);
	m_frameButtons->Init();

	m_verSizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(m_verSizer);

}

void SecondaryPanel::ShowDisclaimer() 
{
	m_title->SetText("DISCLAIMER AGREEMENT");
	
	if ( !m_rtc )
	{
		DeleteSettingsShapes();

		m_rtc = new DisclaimerPanel(this, -1, "", wxDefaultPosition, wxSize(700, -1));
		m_scrollbar = new CustomRTCScrollbar(this, m_rtc, -1);

		wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
		sizer->AddStretchSpacer(1);
		sizer->Add(m_rtc, wxSizerFlags(0).Expand());
		sizer->AddStretchSpacer(1);
		sizer->Add(m_scrollbar, wxSizerFlags(0).Expand());
		sizer->AddSpacer(20);

		m_verSizer->Clear(true);
		m_verSizer->AddSpacer(TOP_SPACE);
		m_verSizer->Add(sizer, wxSizerFlags(1).Expand());

		StartLoadLoop();
	}

	LayoutSelf();
}

void SecondaryPanel::ShowSettings()
{
	m_title->SetText("SETTINGS");
	wxSFDiagramManager* pManager = GetDiagramManager();

	if ( m_rtc )
	{
		m_verSizer->Clear(true);
		m_rtc = nullptr;
		m_scrollbar = nullptr;

		if ( m_disDecline )
		{
			pManager->RemoveShape(m_disDecline, false);
			pManager->RemoveShape(m_disAgree, true);
			m_disDecline = nullptr;
			m_disAgree = nullptr;
		}
	}

	if ( !m_mainSettingsGrid )
	{
		m_mainSettingsGrid = (wxSFFlexGridShape*)pManager->AddShape(CLASSINFO(wxSFFlexGridShape), false);
		m_mainSettingsGrid->AcceptChild(wxT("All"));
		m_mainSettingsGrid->SetFill(*wxTRANSPARENT_BRUSH);
		m_mainSettingsGrid->SetBorder(*wxTRANSPARENT_PEN);
		m_mainSettingsGrid->SetDimensions(5, 2);
		m_mainSettingsGrid->SetCellSpace(15);
		m_mainSettingsGrid->SetStyle(0);

		wxSFTextShape* pInstallPathLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pInstallPathLabel->SetTextColour(*wxWHITE);
		pInstallPathLabel->SetFont(wxFontInfo(14).Bold().FaceName("Times New Roman"));
		SetShapeStyle(pInstallPathLabel);
		pInstallPathLabel->SetText("Install Path                                            ");

		m_installPath = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		m_installPath->SetTextColour(*wxWHITE);
		m_installPath->SetFont(wxFontInfo(14).FaceName("Times New Roman"));
		SetShapeStyle(m_installPath);

		wxSFTextShape* pAutoUpdateLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pAutoUpdateLabel->SetTextColour(*wxWHITE);
		pAutoUpdateLabel->SetFont(wxFontInfo(14).Bold().FaceName("Times New Roman"));
		SetShapeStyle(pAutoUpdateLabel);
		pAutoUpdateLabel->SetText("Automatically look for and install updates                   ");

		m_autoUpdate = (CheckboxShape*)pManager->AddShape(CLASSINFO(CheckboxShape), false);
		m_autoUpdate->SetRectSize(25, 25);
		m_autoUpdate->SetId(BUTTON_AutoUpdate);

		m_uninstallButton = new TransparentButton("UNINSTALL", wxDefaultPosition, wxDefaultPosition, 3.0, pManager);
		m_uninstallButton->SetId(BUTTON_Uninstall);
		m_uninstallButton->SetFont(wxFontInfo(20).FaceName("Times New Roman"));
		m_uninstallButton->AddStyle(wxSFShapeBase::STYLE::sfsSIZE_CHANGE);
		pManager->AddShape(m_uninstallButton, nullptr, wxDefaultPosition, true, false);

		m_mainSettingsGrid->AppendToGrid(pInstallPathLabel);
		m_mainSettingsGrid->AppendToGrid(m_installPath);
		m_mainSettingsGrid->AppendToGrid(pAutoUpdateLabel);
		m_mainSettingsGrid->AppendToGrid(m_autoUpdate);
		m_mainSettingsGrid->AppendToGrid(m_uninstallButton);

		ReloadSettings();
	}

	LayoutSelf();
}

void SecondaryPanel::OnFileLoaded()
{
	if ( !m_rtc )
		return;

	wxSFDiagramManager* manager = GetDiagramManager();
	if ( !m_disDecline && !btfl::HasUserAgreedToDisclaimer() )
	{
		m_verSizer->AddSpacer(BOTTOM_SPACE);

		m_disDecline = new TransparentButton("DECLINE", wxDefaultPosition, wxDefaultPosition, 3.0, manager);
		m_disDecline->SetId(BUTTON_Back);
		m_disDecline->SetFont(wxFontInfo(20).FaceName("Times New Roman"));
		manager->AddShape(m_disDecline, nullptr, wxDefaultPosition, true, false);

		m_disAgree = new TransparentButton("", wxDefaultPosition, wxDefaultPosition, 3.0, manager);
		m_disAgree->SetFill(wxBrush(wxColour(255, 255, 255)));
		m_disAgree->SetTextColour(wxColour(0, 0, 0));
		m_disAgree->SetFont(wxFontInfo(20).FaceName("Times New Roman"));
		manager->AddShape(m_disAgree, nullptr, wxDefaultPosition, true, false);
	}
	else if ( m_disDecline && btfl::HasUserAgreedToDisclaimer() )
	{
		m_verSizer->Remove(2);

		manager->RemoveShape(m_disDecline, false);
		manager->RemoveShape(m_disAgree, true);
		m_disDecline = nullptr;
		m_disAgree = nullptr;
	}

	if ( m_disAgree )
	{
		if ( m_mainFrame->IsIsoSelected() )
		{
			m_disAgree->SetLabel("ACCEPT & VERIFY");
			m_disAgree->SetId(BUTTON_DisclaimerAgreeVerify);
		}
		else
		{
			m_disAgree->SetLabel("ACCEPT");
			m_disAgree->SetId(BUTTON_DisclaimerAgree);
		}
	}

	LayoutSelf();
}

void SecondaryPanel::SelectInstallPath()
{
	wxDirDialog dirDialog(nullptr, _("Please select a folder..."),
		"./", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	
	if ( dirDialog.ShowModal() == wxID_OK )
	{
		m_installPath->SetText(dirDialog.GetPath());
		btfl::SetInstallPath(dirDialog.GetPath());
		RepositionAll();
		Refresh();
	}
}

void SecondaryPanel::SetSettings(const btfl::Settings& settings)
{
	m_settings.bLookForUpdates = settings.bLookForUpdates;
	if ( m_mainSettingsGrid )
		ReloadSettings();
}

void SecondaryPanel::OnFrameButtons(wxSFShapeMouseEvent& event)
{
	switch ( event.GetId() )
	{
	case BUTTON_Help:
		break;

	case BUTTON_Minimize:
		m_mainFrame->Iconize();
		break;

	case BUTTON_Close:
		m_mainFrame->Close();
		break;

	case BUTTON_Back:
		m_mainFrame->ShowMainPanel();
		SetCursor(wxCURSOR_DEFAULT);
		break;
	}
}

void SecondaryPanel::OnAcceptDisclaimer(wxSFShapeMouseEvent& event)
{
	btfl::AgreeToDisclaimer();
	m_mainFrame->ShowMainPanel();

	if ( event.GetId() == BUTTON_DisclaimerAgreeVerify )
		m_mainFrame->VerifyIso();

	// If the mouse isn't moved out of a button and the button is destroyed, the
	// cursor doesn't set itself back to default, so I do it manually.
	SetCursor(wxCURSOR_DEFAULT);
}

void SecondaryPanel::OnAutoUpdateChange(wxSFShapeMouseEvent& event)
{
	m_settings.bLookForUpdates = m_autoUpdate->IsChecked();
	DoSaveSettings();
}

void SecondaryPanel::RepositionAll()
{
	wxSize size = GetClientSize();
	wxRealPoint shapeSize;

	shapeSize = m_frameButtons->GetRectSize();
	m_frameButtons->MoveTo(size.x - shapeSize.x - 10, 10);

	shapeSize = m_title->GetRectSize();
	m_title->MoveTo((size.x / 2) - (shapeSize.x / 2), (TOP_SPACE / 2) - (shapeSize.y / 2));

	shapeSize = m_backArrow->GetRectSize();
	m_backArrow->MoveTo(10, 10);

	if ( m_rtc )
	{
		wxRect disclaimerRect = m_rtc->GetRect();

		if ( m_disDecline )
		{
			int yButtonHalf = disclaimerRect.GetBottom() + ((size.y - disclaimerRect.GetBottom()) / 2);

			m_disDecline->RecalculateSelf();
			shapeSize = m_disDecline->GetRectSize();
			m_disDecline->MoveTo(disclaimerRect.x, yButtonHalf - (shapeSize.y / 2));

			m_disAgree->RecalculateSelf();
			shapeSize = m_disAgree->GetRectSize();
			m_disAgree->MoveTo(disclaimerRect.GetRight() - shapeSize.x, yButtonHalf - (shapeSize.y / 2));
		}
	}
	else if ( m_mainSettingsGrid )
	{
		m_mainSettingsGrid->Update();
		m_mainSettingsGrid->SetRelativePosition(
			(GetSize().x - m_mainSettingsGrid->GetRectSize().x) / 2,
			70.0
		);

		m_fInstallPathBmpScale = double(m_installPath->GetRectSize().y) / m_installPathBmp.GetHeight();
		m_installPathBmpPos = wxPoint(
			(m_installPath->GetAbsolutePosition().x - (m_installPathBmp.GetWidth() * m_fInstallPathBmpScale) - 5) / m_fInstallPathBmpScale,
			m_installPath->GetAbsolutePosition().y / m_fInstallPathBmpScale
		);
	}
}

void SecondaryPanel::DeleteSettingsShapes()
{
	if ( !m_mainSettingsGrid )
		return;

	wxSFDiagramManager* pManager = GetDiagramManager();
	pManager->RemoveShape(m_mainSettingsGrid, false);
	
	m_installPath = nullptr;
	m_autoUpdate = nullptr;
	m_uninstallButton = nullptr;
	m_mainSettingsGrid = nullptr;
}

void SecondaryPanel::SetShapeStyle(wxSFShapeBase* shape)
{
	shape->SetStyle(
		wxSFShapeBase::STYLE::sfsALWAYS_INSIDE |
		wxSFShapeBase::STYLE::sfsSIZE_CHANGE
	);
}

void SecondaryPanel::LayoutSelf()
{
	Layout();
	SendSizeEvent();
	RepositionAll();
	Refresh();
}

void SecondaryPanel::DrawForeground(wxDC& dc, bool fromPaint)
{
	dc.SetUserScale(m_bgScale, m_bgScale);
	dc.DrawBitmap(m_topSeparator, 0, 49.0 / m_bgScale, true);
	dc.SetUserScale(1.0, 1.0);

	if ( m_mainSettingsGrid )
	{
		dc.SetUserScale(m_fInstallPathBmpScale, m_fInstallPathBmpScale);
		dc.DrawBitmap(m_installPathBmp, m_installPathBmpPos.x, m_installPathBmpPos.y, true);
		dc.SetUserScale(1.0, 1.0);
	}
}

void SecondaryPanel::OnSize(wxSizeEvent& event)
{
	BackgroundImageCanvas::OnSize(event);

	if ( m_rtc )
	{
		wxPoint disclaimerPos = m_rtc->GetPosition();

		// For some reason I need to offset the x component by 15 so it statys aligned.
		// Might be a quirk with how wxRTC draws itself
		((DisclaimerPanel*)m_rtc)->SetBackgroundX(m_bgx - ((double)disclaimerPos.x / m_bgScale));
		((DisclaimerPanel*)m_rtc)->SetBackgroundY(m_bgy - ((double)disclaimerPos.y / m_bgScale));
	}

	RepositionAll();
}

void SecondaryPanel::OnLeftDown(wxMouseEvent& event)
{
	BackgroundImageCanvas::OnLeftDown(event);

	if ( m_bIsHoveringInstallPath )
	{
		SelectInstallPath();
	}
}

void SecondaryPanel::OnMouseMove(wxMouseEvent& event)
{
	BackgroundImageCanvas::OnMouseMove(event);

	if ( m_mainSettingsGrid )
	{
		wxRect installPathBmpRect(
			m_installPathBmpPos * m_fInstallPathBmpScale, 
			m_installPathBmp.GetSize() * m_fInstallPathBmpScale
		);

		bool bIsHoveringInstallPath = installPathBmpRect.Contains(event.GetPosition());
		if ( bIsHoveringInstallPath != m_bIsHoveringInstallPath )
		{
			SetCursor((wxStockCursor)((wxCURSOR_DEFAULT * !bIsHoveringInstallPath) + (wxCURSOR_CLOSED_HAND * bIsHoveringInstallPath)));
			m_bIsHoveringInstallPath = bIsHoveringInstallPath;
		}
	}
}

void SecondaryPanel::ReloadSettings()
{
	m_installPath->SetText(btfl::GetInstallFileName().GetFullPath());
	m_autoUpdate->SetState(m_settings.bLookForUpdates);
}
