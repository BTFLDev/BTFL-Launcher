#include "SecondaryPanel.h"
#include "Scrollbar.h"
#include "MainFrame.h"

#include <wx/choicdlg.h>

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

XS_IMPLEMENT_CLONABLE_CLASS(CheckboxShape, wxSFCircleShape);

wxBitmap CheckboxShape::m_colossusEyeBlue = wxBitmap();
bool CheckboxShape::m_bIsInitialized = false;

CheckboxShape::CheckboxShape() : wxSFCircleShape()
{
	SetState(false);
	SetFill(wxBrush(*wxTRANSPARENT_BRUSH));

	AddStyle(sfsEMIT_EVENTS);

	RemoveStyle(sfsHIGHLIGHTING);
	RemoveStyle(sfsPOSITION_CHANGE);
	RemoveStyle(sfsPARENT_CHANGE);
	RemoveStyle(sfsSIZE_CHANGE);
	RemoveStyle(sfsSHOW_HANDLES);
}

void CheckboxShape::Initialize()
{
	if ( m_bIsInitialized )
		return;

	m_colossusEyeBlue.LoadFile("Assets/Icon/Colossus Eye Blue.png", wxBITMAP_TYPE_PNG);
	
	m_bIsInitialized = true;
}

void CheckboxShape::SetState(bool isChecked)
{
	if ( isChecked )
		SetBorder(wxPen(wxColour(50, 180, 220), 2));
	else
		SetBorder(wxPen(wxColour(160, 50, 50), 2));
	
	m_bIsChecked = isChecked;
	Refresh();
}

void CheckboxShape::UpdateBitmapScale()
{
	wxRealPoint szRectSize = GetRectSize();
	wxSize szBitmapSize = m_colossusEyeBlue.GetSize();

	m_dBitmapScaleX = szRectSize.x / szBitmapSize.x;
	m_dBitmapScaleY = szRectSize.y / szBitmapSize.y;
}

void CheckboxShape::Draw(wxDC& dc, bool children)
{
	wxSFCircleShape::Draw(dc, children);
	if ( !m_bIsChecked )
		return;

	wxRealPoint pos = GetAbsolutePosition();

	double dPrevScaleX, dPrevScaleY;
	dc.GetUserScale(&dPrevScaleX, &dPrevScaleY);

	dc.SetUserScale(m_dBitmapScaleX, m_dBitmapScaleY);
	dc.DrawBitmap(m_colossusEyeBlue, pos.x / m_dBitmapScaleX, pos.y / m_dBitmapScaleY, true);
	dc.SetUserScale(dPrevScaleX, dPrevScaleY);
}

void CheckboxShape::OnLeftClick(const wxPoint& pos)
{
	SetState(!m_bIsChecked);
	wxSFCircleShape::OnLeftClick(pos);
}

wxBEGIN_EVENT_TABLE(SecondaryPanel, BackgroundImageCanvas)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Help, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Minimize, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Close, SecondaryPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Back, SecondaryPanel::OnFrameButtons)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_DisclaimerAgree, SecondaryPanel::OnAcceptDisclaimer)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_DisclaimerAgreeVerify, SecondaryPanel::OnAcceptDisclaimer)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_AutoUpdate, SecondaryPanel::OnAutoUpdateChange)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_CloseOnGameLaunch, SecondaryPanel::OnCloseOnGameLaunchChange)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Uninstall, SecondaryPanel::OnUninstall)

EVT_MOUSE_CAPTURE_LOST(SecondaryPanel::OnMouseCaptureLost)

wxEND_EVENT_TABLE()

wxString GetSanitizedInstallPathForDisplay(const wxString& path)
{
	if ( path.size() < 75 )
		return path;

	wxString sNewString(path);
	sNewString.insert(50, " -\n- ");
	return sNewString;
}

SecondaryPanel::SecondaryPanel(wxSFDiagramManager* manager, MainFrame* parent) :
	BackgroundImageCanvas(manager, parent, -1), RTCFileLoader(this)
{
	m_mainFrame = parent;
	m_sFileToLoad = "disclaimer.xml";

	MAX_BG_OFFSET = 0;
	m_background = utils::crypto::GetDecryptedImage("Assets/Background/Subpage.png");
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
	m_title->SetFont(wxFontInfo(15).Bold().FaceName("Lora"));
	m_title->SetStyle(0);

	m_frameButtons = (FrameButtons*)manager->AddShape(CLASSINFO(FrameButtons), false);
	m_frameButtons->Init();

	CheckboxShape::Initialize();

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
	else
	{
		OnFileLoaded();
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
		m_mainSettingsGrid->SetDimensions(6, 2);
		m_mainSettingsGrid->SetCellSpace(15);
		m_mainSettingsGrid->SetStyle(0);

		wxSFTextShape* pInstallPathLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pInstallPathLabel->SetTextColour(*wxWHITE);
		pInstallPathLabel->SetFont(wxFontInfo(14).Bold().FaceName("Lora"));
		SetShapeStyle(pInstallPathLabel);
		pInstallPathLabel->SetText("Install Path                                            ");

		m_installPath = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		m_installPath->SetTextColour(*wxWHITE);
		m_installPath->SetFont(wxFontInfo(12).FaceName("Lora"));
		SetShapeStyle(m_installPath);

		wxSFTextShape* pAutoUpdateLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pAutoUpdateLabel->SetTextColour(*wxWHITE);
		pAutoUpdateLabel->SetFont(wxFontInfo(14).Bold().FaceName("Lora"));
		SetShapeStyle(pAutoUpdateLabel);
		pAutoUpdateLabel->SetText("Automatically  look  for  and  install  updates                   ");

		m_autoUpdate = (CheckboxShape*)pManager->AddShape(CLASSINFO(CheckboxShape), false);
		m_autoUpdate->SetRectSize(28, 28);
		m_autoUpdate->SetId(BUTTON_AutoUpdate);
		m_autoUpdate->UpdateBitmapScale();

		wxSFTextShape* pCloseOnGameLaunchLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pCloseOnGameLaunchLabel->SetTextColour(*wxWHITE);
		pCloseOnGameLaunchLabel->SetFont(wxFontInfo(14).Bold().FaceName("Lora"));
		SetShapeStyle(pCloseOnGameLaunchLabel);
		pCloseOnGameLaunchLabel->SetText("Close  Launcher  when  launching  game");

		m_closeSelfOnGameLaunch = (CheckboxShape*)pManager->AddShape(CLASSINFO(CheckboxShape), false);
		m_closeSelfOnGameLaunch->SetRectSize(28, 28);
		m_closeSelfOnGameLaunch->SetId(BUTTON_CloseOnGameLaunch);
		m_closeSelfOnGameLaunch->UpdateBitmapScale();

		m_uninstallButton = new TransparentButton("UNINSTALL", wxDefaultPosition, wxDefaultPosition, 3.0, pManager);
		m_uninstallButton->SetId(BUTTON_Uninstall);
		m_uninstallButton->SetFont(wxFontInfo(20).FaceName("Lora"));
		m_uninstallButton->AddStyle(wxSFShapeBase::STYLE::sfsSIZE_CHANGE);
		pManager->AddShape(m_uninstallButton, nullptr, wxDefaultPosition, true, false);

		m_mainSettingsGrid->AppendToGrid(pInstallPathLabel);
		m_mainSettingsGrid->AppendToGrid(m_installPath);
		m_mainSettingsGrid->AppendToGrid(pAutoUpdateLabel);
		m_mainSettingsGrid->AppendToGrid(m_autoUpdate);
		m_mainSettingsGrid->AppendToGrid(pCloseOnGameLaunchLabel);
		m_mainSettingsGrid->AppendToGrid(m_closeSelfOnGameLaunch);
		m_mainSettingsGrid->AppendToGrid(m_uninstallButton);

		ReloadSettings();
	}

	if ( btfl::IsUserAnEssence() && !m_essenceInstallPath )
	{
		wxSFTextShape* pEssenceInstallPathLabel = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		pEssenceInstallPathLabel->SetTextColour(*wxWHITE);
		pEssenceInstallPathLabel->SetFont(wxFontInfo(14).Bold().FaceName("Lora"));
		SetShapeStyle(pEssenceInstallPathLabel);
		pEssenceInstallPathLabel->SetText("Essence Install Path                                     ");

		m_essenceInstallPath = (wxSFTextShape*)pManager->AddShape(CLASSINFO(wxSFTextShape), false);
		m_essenceInstallPath->SetTextColour(*wxWHITE);
		m_essenceInstallPath->SetFont(wxFontInfo(12).FaceName("Lora"));
		SetShapeStyle(m_essenceInstallPath);

		m_mainSettingsGrid->InsertToGrid(2, pEssenceInstallPathLabel);
		m_mainSettingsGrid->InsertToGrid(3, m_essenceInstallPath);

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
		m_disDecline->SetFont(wxFontInfo(20).FaceName("Lora"));
		manager->AddShape(m_disDecline, nullptr, wxDefaultPosition, true, false);

		m_disAgree = new TransparentButton("", wxDefaultPosition, wxDefaultPosition, 3.0, manager);
		m_disAgree->SetState(Special);
		m_disAgree->SetFont(wxFontInfo(20).FaceName("Lora"));
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
		if ( btfl::GetState() == btfl::STATE_ToVerifyIso )
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
	btfl::LauncherState currentState = btfl::GetState();
	if ( currentState == btfl::STATE_InstallingGame || currentState== btfl::STATE_UpdatingGame )
	{
		wxMessageBox("You can't modify your installation path when an installation is in progress!");
		return;
	}

	wxDirDialog dirDialog(nullptr, _("Please select a folder..."),
		"./", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
	
	if ( dirDialog.ShowModal() == wxID_OK )
	{
		btfl::SetInstallPath(dirDialog.GetPath() + "/");
		m_installPath->SetText(GetSanitizedInstallPathForDisplay(btfl::GetInstallFileName().GetFullPath()));
		RepositionAll();
		Refresh();
	}
}

void SecondaryPanel::SelectEssenceInstallPath()
{
	btfl::LauncherEssenceState currentState = btfl::GetEssenceState();
	if ( currentState == btfl::STATE_InstallingGameEssence || currentState == btfl::STATE_UpdatingGameEssence )
	{
		wxMessageBox("You can't modify your installation path while an installation is in progress!");
		return;
	}

	wxDirDialog dirDialog(nullptr, _("Please select a folder..."),
		"./", wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);

	if ( dirDialog.ShowModal() == wxID_OK )
	{
		btfl::SetEssenceInstallPath(dirDialog.GetPath() + "/");
		m_essenceInstallPath->SetText(GetSanitizedInstallPathForDisplay(btfl::GetEssenceInstallFileName().GetFullPath()));
		RepositionAll();
		Refresh();
	}
}

void SecondaryPanel::SetSettings(const btfl::Settings& settings)
{
	m_settings.bLookForUpdates = settings.bLookForUpdates;
	m_settings.bCloseSelfOnGameLaunch = settings.bCloseSelfOnGameLaunch;
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
		if ( btfl::ShouldShutdown() )
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

void SecondaryPanel::OnCloseOnGameLaunchChange(wxSFShapeMouseEvent& event)
{
	m_settings.bCloseSelfOnGameLaunch = m_closeSelfOnGameLaunch->IsChecked();
	DoSaveSettings();
}

void SecondaryPanel::OnUninstall(wxSFShapeMouseEvent& event)
{
	btfl::LauncherState currentState = btfl::GetState();
	btfl::LauncherEssenceState essenceState = btfl::GetEssenceState();

	if ( currentState != btfl::STATE_ToPlayGame && currentState != btfl::STATE_ToUpdateGame &&
		essenceState != btfl::STATE_ToPlayGameEssence && essenceState != btfl::STATE_ToUpdateGameEssence)
		return;

	bool bIsUninstallingPreview = false;

	if ( essenceState == btfl::STATE_ToPlayGameEssence || essenceState == btfl::STATE_ToUpdateGameEssence )
	{
		wxArrayString choices;
		choices.Add("Preview");
		choices.Add("Standard release");

		wxSingleChoiceDialog choiceDialog(nullptr, "Which version of the game would you like to delete?", "Please choose the desired game to uninstall", choices);

		if ( choiceDialog.ShowModal() != wxID_OK )
			return;

		bIsUninstallingPreview = choiceDialog.GetStringSelection() == "Preview";
	}

	wxMessageDialog dialog(
		nullptr,
		"Do you want to remove all of the game files from your computer?",
		wxString::FromAscii(wxMessageBoxCaptionStr),
		wxOK | wxCANCEL
	);

	if ( dialog.ShowModal() != wxID_OK )
		return;

	if ( bIsUninstallingPreview )
		btfl::UninstallEssenceGame();
	else
		btfl::UninstallGame();
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

	m_dragSafeArea = wxRect(wxPoint(m_backArrow->GetBoundingBox().GetRight(), 0), m_frameButtons->GetBoundingBox().GetLeftBottom());

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

		if ( m_essenceInstallPath )
		{
			m_essenceInstallPathBmpPos = wxPoint(
				(m_essenceInstallPath->GetAbsolutePosition().x - (m_installPathBmp.GetWidth() * m_fInstallPathBmpScale) - 5) / m_fInstallPathBmpScale,
				m_essenceInstallPath->GetAbsolutePosition().y / m_fInstallPathBmpScale
			);
		}
		else
			m_essenceInstallPathBmpPos = wxPoint(-100, -100);
	}
}

void SecondaryPanel::DeleteSettingsShapes()
{
	if ( !m_mainSettingsGrid )
		return;

	wxSFDiagramManager* pManager = GetDiagramManager();
	pManager->RemoveShape(m_mainSettingsGrid, false);
	
	m_installPath = nullptr;
	m_essenceInstallPath = nullptr;
	m_autoUpdate = nullptr;
	m_closeSelfOnGameLaunch = nullptr;
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

		if ( btfl::GetEssenceState() != btfl::STATE_NoneEssence )
			dc.DrawBitmap(m_installPathBmp, m_essenceInstallPathBmpPos.x, m_essenceInstallPathBmpPos.y, true);
		
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

	if ( m_dragSafeArea.Contains(event.GetPosition()) )
	{
		m_bIsDraggingFrame = true;
		m_dragStartMousePos = wxGetMousePosition();
		m_dragStartFramePos = m_mainFrame->GetPosition();
		CaptureMouse();
		return;
	}

	if ( m_bIsHoveringInstallPath )
	{
		SelectInstallPath();
	}

	if ( m_bIsHoveringEssenceInstallPath )
	{
		SelectEssenceInstallPath();
	}
}

void SecondaryPanel::OnLeftUp(wxMouseEvent& event)
{
	BackgroundImageCanvas::OnLeftUp(event);

	if ( HasCapture() )
	{
		m_bIsDraggingFrame = false;
		ReleaseCapture();
	}
}

void SecondaryPanel::OnMouseMove(wxMouseEvent& event)
{
	if ( m_bIsDraggingFrame )
	{
		wxPoint toMove = wxGetMousePosition() - m_dragStartMousePos;
		m_mainFrame->Move(m_dragStartFramePos + toMove);
		return;
	}

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

		wxRect essenceInstallPathBmpRect(
			m_essenceInstallPathBmpPos * m_fInstallPathBmpScale,
			m_installPathBmp.GetSize() * m_fInstallPathBmpScale
		);

		bool bIsHoveringEssenceInstallPath = essenceInstallPathBmpRect.Contains(event.GetPosition());
		if ( bIsHoveringEssenceInstallPath != m_bIsHoveringEssenceInstallPath )
		{
			SetCursor((wxStockCursor)((wxCURSOR_DEFAULT * !bIsHoveringEssenceInstallPath) + (wxCURSOR_CLOSED_HAND * bIsHoveringEssenceInstallPath)));
			m_bIsHoveringEssenceInstallPath = bIsHoveringEssenceInstallPath;
		}
	}
}

void SecondaryPanel::ReloadSettings()
{
	m_installPath->SetText(GetSanitizedInstallPathForDisplay(btfl::GetInstallFileName().GetFullPath()));
	m_autoUpdate->SetState(m_settings.bLookForUpdates);
	m_closeSelfOnGameLaunch->SetState(m_settings.bCloseSelfOnGameLaunch);

	if ( m_essenceInstallPath )
		m_essenceInstallPath->SetText(GetSanitizedInstallPathForDisplay(btfl::GetEssenceInstallFileName().GetFullPath()));
}
