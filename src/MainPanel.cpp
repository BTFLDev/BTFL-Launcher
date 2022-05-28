#include "MainPanel.h"

#include "MainFrame.h"
#include "MyApp.h"

#include <thread>
#include <wx/zipstrm.h>
#include <wx/sstream.h>
#include <wx/mimetype.h>
#include <wx/textdlg.h>

#include "IsoChecking.h"
#include "wxmemdbg.h"

bool CustomGauge::UpdateValue(int currentUnit)
{
	if ( currentUnit > m_units )
		currentUnit = m_units;

	m_currentUnit = currentUnit;

	return m_currentUnit >= m_units;
}

void CustomGauge::DrawBar(wxDC& dc)
{
	wxRect rect = GetBoundingBox();
	rect.Deflate(2);
	rect.SetWidth((m_currentUnit * rect.GetWidth()) / m_units);

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(m_barColour));

	dc.DrawRoundedRectangle(rect, GetRadius());
}

void CustomGauge::DrawNormal(wxDC& dc)
{
	wxSFRoundRectShape::DrawNormal(dc);
	DrawBar(dc);

}

void CustomGauge::DrawHover(wxDC & dc)
{
	wxSFRoundRectShape::DrawHover(dc);
	DrawBar(dc);
}

wxBEGIN_EVENT_TABLE(MainPanel, wxSFShapeCanvas)

EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Help, MainPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Minimize, MainPanel::OnFrameButtons)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Close, MainPanel::OnFrameButtons)

EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_ToSelectIso, MainPanel::OnSelectIso)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_ToVerifyIso, MainPanel::OnVerifyIso)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_VerificationFailed, MainPanel::OnSelectIso)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_ToInstallGame, MainPanel::OnInstall)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_ToUpdateGame, MainPanel::OnInstall)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherState::STATE_ToPlayGame, MainPanel::OnPlay)
EVT_SF_SHAPE_LEFT_DOWN(BUTTON_Settings, MainPanel::OnSettings)

EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherEssenceState::STATE_ToInstallGameEssence, MainPanel::OnInstallEssence)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherEssenceState::STATE_ToUpdateGameEssence, MainPanel::OnInstallEssence)
EVT_SF_SHAPE_LEFT_DOWN(btfl::LauncherEssenceState::STATE_ToPlayGameEssence, MainPanel::OnPlayEssence)

EVT_TIMER(TIMER_Gauge, MainPanel::OnGaugeTimer)

EVT_MOUSE_CAPTURE_LOST(MainPanel::OnMouseCaptureLost)

wxEND_EVENT_TABLE()

MainPanel::MainPanel(wxSFDiagramManager* manager,
	MainFrame* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style) : BackgroundImageCanvas(manager, parent, id, pos, size, style), m_gaugeTimer(this, TIMER_Gauge)
{
	m_mainFrame = parent;

	Bind(wxEVT_WEBREQUEST_STATE, &MainPanel::OnWebRequestState, this);

	ChangeToRandomBgImage();
	m_logo.LoadFile("Assets/LauncherLogo.png", wxBITMAP_TYPE_PNG);

	m_fileContainer.LoadFile("Assets/Containers/FilePath@2x.png", wxBITMAP_TYPE_PNG);
	m_fileBmp.LoadFile("Assets/Icon/Browse@2x.png", wxBITMAP_TYPE_PNG);
	m_fileBmpScale = (double)256 / m_fileContainer.GetWidth();

	manager->AcceptShape("TransparentButton");

	m_mainButton = new TransparentButton("SELECT ISO", wxDefaultPosition, wxDefaultPosition, 3.0, manager);
	m_mainButton->SetFont(wxFontInfo(20).FaceName("Lora"));
	m_mainButton->SetPadding(35, 12);
	manager->AddShape(m_mainButton, nullptr, wxDefaultPosition, true, false);

	m_configButton = new TransparentButton("", wxDefaultPosition, wxDefaultPosition, 3.0, manager);
	m_configButton->SetId(BUTTON_Settings);
	m_configButton->SetBitmap(wxBitmap("Assets/Icon/Settings@2x.png", wxBITMAP_TYPE_PNG));
	m_configButton->SetPadding(16, 16);
	manager->AddShape(m_configButton, nullptr, wxDefaultPosition, true, false);

	m_frameButtons = (FrameButtons*)manager->AddShape(CLASSINFO(FrameButtons), false);
	m_frameButtons->Init();

	m_bgChangeTimer.SetOwner(&m_bgChangeTimer, TIMER_BgChange);
	m_bgChangeTimer.Bind(wxEVT_TIMER, &MainPanel::OnBgChangeTimer, this);

	m_bgChangeTimer.StartOnce(7000);
}

void MainPanel::SetState(btfl::LauncherState state)
{
	m_mainButton->SetId(state);
	m_fileDescColour = { 180, 180, 180 };

	switch ( state )
	{
	case btfl::LauncherState::STATE_ToSelectIso:
		m_mainButton->SetBitmap(wxBitmap("Assets/Icon/Verify@2x.png", wxBITMAP_TYPE_PNG));
		m_mainButton->SetLabel("SELECT ISO");
		m_mainButton->Enable(true);
		m_mainButton->SetState(Idle);
		
		m_fileBmp.LoadFile("Assets/Icon/Browse@2x.png", wxBITMAP_TYPE_PNG); 
		m_fileDescColour = { 52, 199, 226 };
		m_fileLabel = "No ISO selected...";
		m_fileDesc = "View Installation Guide";

		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_ToVerifyIso:
		m_mainButton->SetBitmap(wxBitmap("Assets/Icon/Verify@2x.png", wxBITMAP_TYPE_PNG));
		m_mainButton->SetLabel("VERIFY ISO");
		m_mainButton->Enable(true);
		m_mainButton->SetState(Idle);

		m_fileBmp.LoadFile("Assets/Icon/Folder@2x.png", wxBITMAP_TYPE_PNG);
		m_fileLabel = btfl::GetIsoFileName().GetName();
		m_fileDesc = "ISO File";
		
		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_VerifyingIso:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("VERIFYING...");
		m_mainButton->Enable(false);
		m_mainButton->SetState(Processing);

		m_fileBmp.LoadFile("Assets/Icon/Folder@2x.png", wxBITMAP_TYPE_PNG);
		m_fileLabel = btfl::GetIsoFileName().GetName();
		m_fileDesc = "ISO File";

		CreateGauge();
		break;

	case btfl::LauncherState::STATE_VerificationFailed:
		m_mainButton->SetBitmap(wxBitmap("Assets/Icon/Verify@2x.png", wxBITMAP_TYPE_PNG));
		m_mainButton->SetLabel("SELECT ISO");
		m_mainButton->Enable(true);
		m_mainButton->SetState(Idle);

		m_fileBmp.LoadFile("Assets/Icon/Failed@2x.png", wxBITMAP_TYPE_PNG);
		m_fileDescColour = { 52, 199, 226 };
		m_fileLabel = "Unrecognized ISO";
		m_fileDesc = "View Installation Guide";

		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_ToInstallGame:
		m_mainButton->SetBitmap(wxBitmap("Assets/Icon/Download@2x.png", wxBITMAP_TYPE_PNG));
		m_mainButton->SetLabel("INSTALL");
		m_mainButton->Enable(true);
		m_mainButton->SetState(Idle);

		m_fileBmp.LoadFile("Assets/Icon/Check@2x.png", wxBITMAP_TYPE_PNG);
		m_fileLabel = iso::GetGameNameFromRegion(btfl::GetIsoRegion());
		m_fileDesc = iso::GetRegionAcronym(btfl::GetIsoRegion());
		
		if ( m_essenceMainButton ) m_essenceMainButton->Enable(true);
		if ( m_essenceUpdateButton ) m_essenceUpdateButton->Enable(true);
		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_InstallingGame:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("INSTALLING...");
		m_mainButton->Enable(false);
		m_mainButton->SetState(Processing);

		if ( m_essenceMainButton ) m_essenceMainButton->Enable(false);
		if ( m_essenceUpdateButton ) m_essenceUpdateButton->Enable(false);
		CreateGauge();
		break;

	case btfl::LauncherState::STATE_ToPlayGame:
	case btfl::LauncherState::STATE_ToUpdateGame:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("PLAY");
		m_mainButton->Enable(true);
		m_mainButton->SetState(Special);
		m_mainButton->SetId(btfl::STATE_ToPlayGame);

		m_fileBmp.LoadFile("Assets/Icon/Check@2x.png", wxBITMAP_TYPE_PNG);
		m_fileLabel = iso::GetGameNameFromRegion(btfl::GetIsoRegion());
		m_fileDesc = iso::GetRegionAcronym(btfl::GetIsoRegion());

		if ( m_essenceMainButton ) m_essenceMainButton->Enable(true);
		if ( m_essenceUpdateButton ) m_essenceUpdateButton->Enable(true);
		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_UpdatingGame:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("UPDATING...");
		m_mainButton->Enable(false);
		m_mainButton->SetState(Processing);

		if ( m_essenceMainButton ) m_essenceMainButton->Enable(false);
		if ( m_essenceUpdateButton ) m_essenceUpdateButton->Enable(false);
		CreateGauge();
		break;
	}

	if ( state == btfl::STATE_ToUpdateGame )
		CreateUpdateButton();
	else
		m_bShouldDeleteUpdateBtn = true;

	RepositionAll();
	Refresh();
}

void MainPanel::SetEssenceState(btfl::LauncherEssenceState state)
{
	if ( state != btfl::STATE_NoneEssence )
		CreateEssencesButtonsIfNeeded();

	m_essenceMainButton->SetId(state);

	switch ( state )
	{
	case btfl::LauncherEssenceState::STATE_NoneEssence:
		DestroyEssencesButtonsIfNeeded();

		m_mainButton->Enable(true);
		if ( m_updateButton ) m_updateButton->Enable(true);
		DestroyGauge();
		break;

	case btfl::LauncherEssenceState::STATE_ToInstallGameEssence:
		m_essenceMainButton->SetBitmap(wxBitmap("Assets/Icon/Download@2x.png", wxBITMAP_TYPE_PNG));
		m_essenceMainButton->SetLabel("INSTALL PREVIEW");
		m_essenceMainButton->Enable(true);
		m_essenceMainButton->SetState(Idle);

		m_mainButton->Enable(true);
		if ( m_updateButton ) m_updateButton->Enable(true);
		DestroyGauge();
		break;

	case btfl::LauncherEssenceState::STATE_InstallingGameEssence:
		m_essenceMainButton->SetBitmap(wxNullBitmap);
		m_essenceMainButton->SetLabel("INSTALLING PREVIEW...");
		m_essenceMainButton->Enable(false);
		m_essenceMainButton->SetState(Processing);

		m_mainButton->Enable(false);
		if ( m_updateButton ) m_updateButton->Enable(false);
		CreateGauge();
		break;

	case btfl::LauncherEssenceState::STATE_ToPlayGameEssence:
	case btfl::LauncherEssenceState::STATE_ToUpdateGameEssence:
		m_essenceMainButton->SetBitmap(wxNullBitmap);
		m_essenceMainButton->SetLabel("PLAY PREVIEW");
		m_essenceMainButton->Enable(true);
		m_essenceMainButton->SetState(Special);

		m_mainButton->Enable(true);
		if ( m_updateButton ) m_updateButton->Enable(true);
		DestroyGauge();
		break;

	case btfl::LauncherEssenceState::STATE_UpdatingGameEssence:
		m_essenceMainButton->SetBitmap(wxNullBitmap);
		m_essenceMainButton->SetLabel("UPDATING PREVIEW...");
		m_essenceMainButton->Enable(false);
		m_essenceMainButton->SetState(Processing);

		m_mainButton->Enable(false);
		if ( m_updateButton ) m_updateButton->Enable(false);
		CreateGauge();
		break;
	}

	if ( state == btfl::LauncherEssenceState::STATE_ToUpdateGameEssence )
		CreateEssenceUpdateButton();
	else
		m_bShouldDeleteEssenceUpdateBtn = true;

	RepositionAll();
	Refresh();
}

void MainPanel::CreateEssencesButtonsIfNeeded()
{
	if ( btfl::GetEssenceState() != btfl::LauncherEssenceState::STATE_NoneEssence &&  !m_essenceMainButton )
	{
		wxSFDiagramManager* pManager = GetDiagramManager();

		m_essenceMainButton = new TransparentButton("", wxDefaultPosition, wxDefaultPosition, 3.0, pManager);
		m_essenceMainButton->SetId(btfl::LauncherEssenceState::STATE_ToInstallGameEssence);
		m_essenceMainButton->SetFont(wxFontInfo(20).FaceName("Lora"));
		m_essenceMainButton->SetPadding(35, 12);
		pManager->AddShape(m_essenceMainButton, nullptr, wxDefaultPosition, true, false);
	}
}

void MainPanel::DestroyEssencesButtonsIfNeeded()
{
	m_bShouldDeleteEssenceUpdateBtn = true;

	if ( !m_essenceMainButton )
		return;

	GetDiagramManager()->RemoveShape(m_essenceMainButton, false);
	m_essenceMainButton = nullptr;
}

void MainPanel::RepositionAll()
{
	wxSize size = GetClientSize();

	m_mainButton->RecalculateSelf();
	wxRect shapeRect = m_mainButton->GetBoundingBox();

	m_mainButton->MoveTo(40, size.y - shapeRect.height - 40);
	wxRect mainButtonRect = m_mainButton->GetBoundingBox();
	shapeRect = mainButtonRect;

	if ( m_updateButton )
	{
		m_updateButton->RecalculateSelf();
		m_updateButton->MoveTo(mainButtonRect.GetTopRight() + wxPoint(10, 0));
		shapeRect = m_updateButton->GetBoundingBox();
	}

	int xPadding, yPadding;
	m_configButton->GetPadding(&xPadding, &yPadding);
	m_configButton->RecalculateSelf(wxSize(mainButtonRect.height - (xPadding * 2), mainButtonRect.height - (yPadding * 2)));
	m_configButton->MoveTo(shapeRect.GetTopRight() + wxPoint(10, 0));

	shapeRect = m_frameButtons->GetBoundingBox();
	m_frameButtons->MoveTo(size.x - shapeRect.width - 10, 10);

	m_fileContPos.x = (double)mainButtonRect.GetLeft() / m_fileBmpScale;
	m_fileContPos.y = (double)(mainButtonRect.GetTop() - ((double)m_fileContainer.GetHeight() * m_fileBmpScale) - 10) / m_fileBmpScale;

	m_fileBmpPos.x = m_fileContPos.x + 20;
	m_fileBmpPos.y = m_fileContPos.y + (((double)m_fileBmp.GetHeight() * m_fileBmpScale) / 2) - 4;

	m_fileLabelPos.x = (double)(m_fileBmpPos.x + m_fileBmp.GetWidth() + 10) * m_fileBmpScale;
	m_fileLabelPos.y = ((double)m_fileContPos.y * m_fileBmpScale) + 6;

	wxClientDC dc(this);
	dc.SetFont(m_fileDescFont);
	wxSize textSize = dc.GetTextExtent(m_fileDesc);

	m_fileDescPos.x = m_fileLabelPos.x;
	m_fileDescPos.y = ((double)(m_fileContPos.y + m_fileContainer.GetHeight()) * m_fileBmpScale) - textSize.y - 4;
	if ( btfl::GetState() == btfl::STATE_ToSelectIso  || btfl::GetState() == btfl::STATE_VerificationFailed )
		m_viewGuideRect = { m_fileDescPos, textSize };
	else
		m_viewGuideRect = { -1,-1,-1,-1 };

	dc.SetFont(m_versionFont);
	textSize = dc.GetTextExtent(btfl::GetInstalledLauncherVersion());
	m_versionLabelPos = wxPoint(size.x - textSize.x - 10, size.y - textSize.y - 5);

	m_dragSafeArea = wxRect(wxPoint(0, 0), m_frameButtons->GetBoundingBox().GetBottomLeft());

	if ( m_gauge )
	{
		m_gauge->SetRectSize(350, 8);
		m_gauge->MoveTo(size.x - 350 - 20, size.y - 48);
		shapeRect = m_gauge->GetBoundingBox();

		m_gaugeLabel->MoveTo(shapeRect.x, shapeRect.y - m_gaugeLabel->GetRectSize().y - 10);
		m_gaugeProgress->MoveTo(shapeRect.GetRight() - m_gaugeProgress->GetRectSize().x,
			shapeRect.y - m_gaugeProgress->GetRectSize().y - 10);
	}

	if ( m_essenceMainButton )
	{
		m_essenceMainButton->RecalculateSelf();
		m_essenceMainButton->MoveTo(m_fileContPos.x * m_fileBmpScale, (m_fileContPos.y * m_fileBmpScale) - 10 - m_essenceMainButton->GetRectSize().y);

		if ( m_essenceUpdateButton )
		{
			m_essenceUpdateButton->RecalculateSelf();
			m_essenceUpdateButton->MoveTo(m_essenceMainButton->GetBoundingBox().GetTopRight() + wxPoint(10, 0));
		}
	}
}

void MainPanel::VerifyIso()
{
	if ( !btfl::GetIsoFileName().Exists() )
		return;

	btfl::SetState(btfl::LauncherState::STATE_VerifyingIso);

	// Start a new thread so we don't hog the program when verifying the iso.
	std::thread thread(
		[&]()
		{
			m_gaugeResult = GAUGE_VerifyInvalid;
			m_nextGaugeLabel = "Verifying ISO validity...";
			srand(time(0));

			wxString fileHash = iso::GetFileHash(btfl::GetIsoFileName().GetFullPath(), m_nextGaugeValue, m_nextGaugeValueMutex);
			
			m_nextGaugeLabel = "Processing results...";
			iso::ISO_Region region = iso::GetIsoRegion(fileHash);

			btfl::SetIsoRegion(region);

			// Change m_gaugeResult so that the function OnGaugeFinished() knows what to
			// do next.
			if ( region != iso::ISO_Invalid )
				m_gaugeResult = GAUGE_VerifyValid;

			// And set the gauge to 100 to trigger the function call.
			m_nextGaugeValue = 100;
		}
	);

	thread.detach();
}

void MainPanel::CreateGauge()
{
	if ( m_gauge )
		return;

	wxSFDiagramManager* manager = GetDiagramManager();

	m_gauge = new CustomGauge(wxDefaultPosition, wxDefaultPosition, 100, 2.0, manager);
	m_gauge->SetBorder(wxPen(wxColour(50, 50, 50)));
	m_gauge->SetFill(wxBrush(wxColour(50, 50, 50)));
	manager->AddShape(m_gauge, nullptr, wxDefaultPosition, true);

	m_gaugeLabel = (wxSFTextShape*)manager->AddShape(CLASSINFO(wxSFTextShape), false);
	m_gaugeLabel->SetBorder(*wxTRANSPARENT_PEN);
	m_gaugeLabel->SetFill(*wxTRANSPARENT_BRUSH);
	m_gaugeLabel->SetTextColour(wxColour(255, 255, 255, 150));
	m_gaugeLabel->SetFont(wxFontInfo(12).FaceName("Lora"));
	m_gaugeLabel->SetText("...");
	m_gaugeLabel->SetStyle(0);

	m_gaugeProgress = (wxSFTextShape*)manager->AddShape(CLASSINFO(wxSFTextShape), false);
	m_gaugeProgress->SetBorder(*wxTRANSPARENT_PEN);
	m_gaugeProgress->SetFill(*wxTRANSPARENT_BRUSH);
	m_gaugeProgress->SetTextColour(wxColour(255, 255, 255, 255));
	m_gaugeProgress->SetFont(wxFontInfo(12).FaceName("Lora"));
	m_gaugeProgress->SetText("0% Complete");
	m_gaugeProgress->SetStyle(0);

	RepositionAll();

	m_gaugeTimer.Start(100);
}

void MainPanel::DestroyGauge()
{
	if ( !m_gauge )
		return;

	m_gaugeTimer.Stop();
	
	wxSFDiagramManager* manager = GetDiagramManager();
	manager->RemoveShape(m_gauge, false);
	manager->RemoveShape(m_gaugeLabel, false);
	manager->RemoveShape(m_gaugeProgress, false);

	m_gauge = nullptr;
	m_gaugeLabel = nullptr;
	m_gaugeProgress = nullptr;
}

void MainPanel::UpdateGauge(int currentUnit, const wxString& message, bool refresh)
{
	if ( !m_gauge )
		return;

	m_gauge->UpdateValue(currentUnit);
	m_gaugeLabel->SetText(message);
	m_gaugeProgress->SetText(std::to_string(m_gauge->GetCurrentPercent()) + "% Complete");
	m_gauge->Refresh(true);

	if ( refresh )
	{
		Refresh();
		Update();
	}
}

void MainPanel::CreateUpdateButton()
{
	if ( m_updateButton )
		return;

	wxSFDiagramManager* pManager = GetDiagramManager();

	m_updateButton = new TransparentButton("UPDATE", wxDefaultPosition, wxDefaultPosition, 3.0, pManager);
	m_updateButton->SetId(btfl::STATE_ToUpdateGame);
	m_updateButton->SetFont(wxFontInfo(20).FaceName("Lora"));
	m_updateButton->SetPadding(35, 12);
	m_updateButton->SetBitmap(wxBitmap("Assets/Icon/Update@2x.png", wxBITMAP_TYPE_PNG));
	pManager->AddShape(m_updateButton, nullptr, wxDefaultPosition, true, false);
}

void MainPanel::CreateEssenceUpdateButton()
{
	if ( m_essenceUpdateButton )
		return;

	wxSFDiagramManager* pManager = GetDiagramManager();

	m_essenceUpdateButton = new TransparentButton("UPDATE PREVIEW", wxDefaultPosition, wxDefaultPosition, 3.0, pManager);
	m_essenceUpdateButton->SetId(btfl::STATE_ToUpdateGameEssence);
	m_essenceUpdateButton->SetFont(wxFontInfo(20).FaceName("Lora"));
	m_essenceUpdateButton->SetPadding(35, 12);
	m_essenceUpdateButton->SetBitmap(wxBitmap("Assets/Icon/Update@2x.png", wxBITMAP_TYPE_PNG));
	pManager->AddShape(m_essenceUpdateButton, nullptr, wxDefaultPosition, true, false);
}

void MainPanel::DestroyButton(TransparentButton*& pButton)
{
	if ( !pButton )
		return;

	GetDiagramManager()->RemoveShape(pButton, false);
	pButton = nullptr;
}

void MainPanel::DoCheckGauge(bool refresh)
{
	wxMutexLocker locker(m_nextGaugeValueMutex);

	// This isn't what I wanted but the OnInstallWebRequestData event doesn't seem to work
	btfl::LauncherState currentState = btfl::GetState();
	btfl::LauncherEssenceState currentEssenceState = btfl::GetEssenceState();
	if ( currentState == btfl::STATE_InstallingGame || currentState == btfl::STATE_UpdatingGame )
	{
		int nValue = (m_webRequest.GetBytesReceived() * 90) /
			m_webRequest.GetBytesExpectedToReceive();

		if ( nValue != 90 )
			m_nextGaugeValue = nValue;
	}
	else if ( currentEssenceState == btfl::STATE_InstallingGameEssence || currentEssenceState == btfl::STATE_UpdatingGameEssence )
	{
		int nValue = (m_webRequestEssence.GetBytesReceived() * 90) /
			m_webRequestEssence.GetBytesExpectedToReceive();

		if ( nValue != 90 )
			m_nextGaugeValue = nValue;
	}

	if ( m_nextGaugeValue != -1 )
	{
		UpdateGauge(m_nextGaugeValue, m_nextGaugeLabel, refresh);
		m_nextGaugeValue = -1;

		if ( m_gauge->GetCurrentPercent() >= 100 )
		{
			m_gaugeTimer.Stop();
			OnGaugeFinished();
		}
	}
}

void MainPanel::OnGaugeFinished()
{
	DestroyGauge();

	switch ( m_gaugeResult )
	{
	case GAUGE_VerifyValid:
		// If the iso is valid, change current state to "STATE_ToInstallGame"
		btfl::SetState(btfl::LauncherState::STATE_ToInstallGame);
		break;

	case GAUGE_VerifyInvalid:
		// Else, change state back to what it should be.
		btfl::SetIsoFileName(wxFileName());
		btfl::SetState(btfl::LauncherState::STATE_VerificationFailed);
		break;

	case GAUGE_InstallationSuccessful:
	{
		btfl::SQLEntry entry("launcher_data");
		entry.strings["installed_game_version"] = btfl::GetLatestGameVersion();
		btfl::UpdateDatabase(entry);

		btfl::SetState(btfl::LauncherState::STATE_ToPlayGame);
		break;
	}
	case GAUGE_InstallationUnsuccessful:
		btfl::RestoreLastState();
		break;
	case GAUGE_EssenceInstallationSuccessful:
	{
		btfl::SQLEntry entry("launcher_data");
		entry.strings["installed_essence_game_version"] = btfl::GetLatestEssenceGameVersion();
		btfl::UpdateDatabase(entry);

		btfl::SetEssenceState(btfl::LauncherEssenceState::STATE_ToPlayGameEssence);
		break;
	}
	case GAUGE_EssenceInstallationUnsuccessful:
		btfl::RestoreLastEssenceState();
		break;
	}
}

void MainPanel::DoSelectIso()
{
	// Attempting to read a file when this dialog is open causes an exception, probably due to Working Directory
	m_bCanChangeBackground = false;

	wxFileDialog fileDialog(nullptr, _("Please select an original Shadow Of The Colossus ISO file..."),
		"", "", "ISO files (*.iso;*.ISO)|*.iso;*.ISO", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	if ( fileDialog.ShowModal() == wxID_CANCEL )
		return;

	wxFileName fileName(fileDialog.GetPath());

	// Double check to see if the file really is an iso file, in case the
	// wxFileDialog didn't take care of it.
	wxString ext = fileName.GetExt();
	if ( ext != "iso" && ext != "ISO" )
	{
		wxMessageBox("You need to select a .iso file.");
		return;
	}

	btfl::SetIsoFileName(fileName);
	btfl::SetState(btfl::LauncherState::STATE_ToVerifyIso);
	m_fileLabel = btfl::GetIsoFileName().GetName();

	// Show disclaimer before doing everything else if the user hasn't agreed to it yet.
	if ( !btfl::HasUserAgreedToDisclaimer() )
		m_mainFrame->ShowDisclaimer();

	m_bCanChangeBackground = true;
}

void MainPanel::DoInstallGame()
{
	btfl::LauncherState currentState = btfl::GetState();
	if ( currentState != btfl::STATE_ToInstallGame && currentState != btfl::STATE_ToUpdateGame )
		return;

	wxFileName installFileName = btfl::GetInstallFileName();
	if ( !installFileName.Exists() )
		wxMkdir(installFileName.GetFullPath(), wxS_DIR_DEFAULT);

	PerformWebRequest(
		m_webRequest,
		"game/BTFL.zip",
		WEB_Install,
		wxWebRequest::Storage_File,
		installFileName.GetFullPath(),
		"Unable to start installation. Please check your internet connection and try again."
	);
	
	m_nextGaugeLabel = "Downloading files...";
	switch ( currentState )
	{
	case btfl::STATE_ToInstallGame:
		btfl::SetState(btfl::STATE_InstallingGame);
		break;

	case btfl::STATE_ToUpdateGame:
		btfl::SetState(btfl::STATE_UpdatingGame);
		break;
	}
}

void MainPanel::DoInstallEssenceGame()
{
	btfl::LauncherEssenceState currentState = btfl::GetEssenceState();
	if ( currentState != btfl::STATE_ToInstallGameEssence && currentState != btfl::STATE_ToUpdateGameEssence )
		return;

	wxFileName essenceInstallFileName = btfl::GetEssenceInstallFileName();
	if ( !essenceInstallFileName.Exists() )
		wxMkdir(essenceInstallFileName.GetFullPath(), wxS_DIR_DEFAULT);

	PerformWebRequest(
		m_webRequestEssence,
		"game/BTFL_Preview.zip",
		WEB_InstallEssence,
		wxWebRequest::Storage_File,
		essenceInstallFileName.GetFullPath(),
		"Unable to start installation. Please check your internet connection and try again."
	);

	m_nextGaugeLabel = "Downloading files...";
	switch ( currentState )
	{
	case btfl::STATE_ToInstallGameEssence:
		btfl::SetEssenceState(btfl::STATE_InstallingGameEssence);
		break;

	case btfl::STATE_ToUpdateGameEssence:
		btfl::SetEssenceState(btfl::STATE_UpdatingGameEssence);
		break;
	}
}

void MainPanel::DoLookForLauncherUpdates()
{
	PerformWebRequest(
		m_latestLauncherVersionRequest,
		"launcher/version.txt",
		WEB_LookForLauncherUpdate,
		wxWebRequest::Storage_Memory
	);
}

void MainPanel::DoLookForGameUpdates()
{
	PerformWebRequest(
		m_latestGameVersionRequest,
		"game/version.txt",
		WEB_LookForGameUpdate,
		wxWebRequest::Storage_Memory
	);
}

void MainPanel::DoLookForEssenceGameUpdates()
{
	PerformWebRequest(
		m_latestEssenceGameVersionRequest,
		"game/essence_version.txt",
		WEB_LookForEssenceGameUpdate,
		wxWebRequest::Storage_Memory
	);
}

void MainPanel::DoCheckEssencePassword()
{
	PerformWebRequest(
		m_essencePasswordRequest,
		"launcher/essence_password.txt",
		WEB_GetEssencePassword,
		wxWebRequest::Storage_Memory
	);
}

void MainPanel::PerformWebRequest(
	wxWebRequest& webRequestObj,
	const wxString& file,
	long id,
	wxWebRequest::Storage storage,
	const wxString& tempDir,
	const wxString& failMessage
)
{
	webRequestObj = wxWebSession::GetDefault().CreateRequest(
		this,
		btfl::GetStorageURL() + file,
		id
	);
	webRequestObj.SetStorage(storage);
	if ( !tempDir.IsEmpty() )
		wxWebSession::GetDefault().SetTempDir(tempDir);

	if ( !webRequestObj.IsOk() )
	{
		if ( !failMessage.IsEmpty() )
			wxMessageBox(failMessage, "Web request failed!");

		return;
	}
	webRequestObj.SetHeader(
		utils::crypto::GetDecryptedString("Bxujpuj}bvjro"),
		utils::crypto::GetDecryptedString("urlgo#hkqaofv[mKr7ijR|GhH\\Spm48ygpzY4nKT4m6MhN"));
	webRequestObj.Start();
}

void MainPanel::OnWebRequestState(wxWebRequestEvent& event)
{
	switch ( event.GetId() )
	{
	case WEB_Install:
	case WEB_InstallEssence:
		HandleInstallWebRequest(event);
		break;

	case WEB_LookForLauncherUpdate:
		HandleLauncherUpdateWebRequest(event);
		break;
	case WEB_LookForGameUpdate:
		HandleGameUpdateWebRequest(event);
		break;
	case WEB_LookForEssenceGameUpdate:
		HandleEssenceGameUpdateWebRequest(event);
		break;
	case WEB_GetEssencePassword:
		HandleEssencePasswordWebRequest(event);
		break;
	}
}

void MainPanel::HandleInstallWebRequest(wxWebRequestEvent& event)
{
	bool bIsInstallingPreview = btfl::GetEssenceState() == btfl::STATE_InstallingGameEssence || btfl::GetEssenceState() == btfl::STATE_UpdatingGameEssence;
	
	switch ( event.GetState() )
	{
	case wxWebRequest::State_Completed:
	{
		m_nextGaugeValue = 95;
		switch ( btfl::GetState() )
		{
		case btfl::STATE_InstallingGame:
			m_nextGaugeLabel = "Installing game...";
			break;
		case btfl::STATE_UpdatingGame:
			m_nextGaugeLabel = "Updating game...";
			break;
		}

		switch ( btfl::GetEssenceState() )
		{
		case btfl::STATE_InstallingGameEssence:
			m_nextGaugeLabel = "Installing preview...";
			break;
		case btfl::STATE_UpdatingGameEssence:
			m_nextGaugeLabel = "Updating game...";
			break;
		}

		wxFileName fileName(event.GetDataFile());
		fileName.SetName("BTFL" + bIsInstallingPreview ? "_Preview" : "");
		fileName.SetExt("zip");

		if ( wxRenameFile(event.GetDataFile(), fileName.GetFullPath()) )
		{
			UnzipGameFiles(
				fileName.	GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR),
				fileName.GetFullName(),
				bIsInstallingPreview ? GaugeResult::GAUGE_EssenceInstallationSuccessful : GaugeResult::GAUGE_InstallationSuccessful,
				bIsInstallingPreview ? GaugeResult::GAUGE_EssenceInstallationUnsuccessful : GaugeResult::GAUGE_InstallationUnsuccessful
			);
		}
		else
		{
			wxMessageBox(wxString("There was a problem when installing the ") << (bIsInstallingPreview ? "preview" : "game") <<
				".\nPlease try again and, if the problem persists, contact the team at the Discord Server.");
			btfl::RestoreLastState();
		}

		break;
	}
	case wxWebRequest::State_Idle:
		m_nextGaugeLabel = "Connecting...";
		break;
	case wxWebRequest::State_Active:
		m_nextGaugeLabel = "Downloading files...";
		break;
	case wxWebRequest::State_Failed:
	case wxWebRequest::State_Cancelled:
		wxMessageBox(wxString("There was a problem when downloading the ") << (bIsInstallingPreview ? "preview" : "game") << ".\nPlease check your internet connection and try again.");

		if ( bIsInstallingPreview )
			btfl::RestoreLastEssenceState();
		else
			btfl::RestoreLastState();
	
		break;
	}
}

void MainPanel::HandleLauncherUpdateWebRequest(wxWebRequestEvent& event)
{
	if ( event.GetState() == wxWebRequest::State_Completed )
	{
		wxStringInputStream* sstream = (wxStringInputStream*)event.GetResponse().GetStream();
		wxStringOutputStream outStream;
		sstream->Read(outStream);

		if ( !outStream.GetString().IsEmpty() && btfl::GetInstalledLauncherVersion() != outStream.GetString() )
		{
			wxMessageDialog dialog(
				nullptr,
				"There's an update available for the Launcher. Press okay to continue with the installation.",
				wxString::FromAscii(wxMessageBoxCaptionStr),
				wxOK | wxCANCEL
			);

			if ( dialog.ShowModal() == wxID_OK )
				btfl::DoUpdateLauncher();
		}
	}
}

void MainPanel::HandleGameUpdateWebRequest(wxWebRequestEvent& event)
{
	if ( event.GetState() == wxWebRequest::State_Completed )
	{
		wxStringInputStream* sstream = (wxStringInputStream*)event.GetResponse().GetStream();
		wxStringOutputStream outStream;
		sstream->Read(outStream);

		if ( !outStream.GetString().IsEmpty() )
		{
			btfl::SetLatestGameVersion(outStream.GetString());
			if ( btfl::GetInstalledGameVersion() != outStream.GetString() )
			{
				if ( btfl::GetState() == btfl::STATE_ToPlayGame && btfl::ShouldAutoUpdateGame() )
					btfl::SetState(btfl::STATE_ToUpdateGame);
			}
		}
	}
}

void MainPanel::HandleEssenceGameUpdateWebRequest(wxWebRequestEvent& event)
{
	if ( event.GetState() == wxWebRequest::State_Completed )
	{
		wxStringInputStream* sstream = (wxStringInputStream*)event.GetResponse().GetStream();
		wxStringOutputStream outStream;
		sstream->Read(outStream);

		if ( !outStream.GetString().IsEmpty() )
		{
			btfl::SetLatestEssenceGameVersion(outStream.GetString());
			if ( btfl::GetInstalledEssenceGameVersion() != outStream.GetString() )
			{
				if ( btfl::GetEssenceState() == btfl::STATE_ToPlayGameEssence )
					btfl::SetEssenceState(btfl::STATE_ToUpdateGameEssence);
			}
		}
	}
}

void MainPanel::HandleEssencePasswordWebRequest(wxWebRequestEvent& event)
{
	if ( event.GetState() == wxWebRequest::State_Completed )
	{
		wxStringInputStream* sstream = (wxStringInputStream*)event.GetResponse().GetStream();
		wxStringOutputStream outStream;
		sstream->Read(outStream);

		if ( !outStream.GetString().IsEmpty() )
		{
			btfl::SetEncryptedEssencePassword(outStream.GetString());
			if ( btfl::GetUserEssencePassword() != utils::crypto::GetDecryptedString(outStream.GetString()) )
			{
				btfl::UninstallEssenceGame(false);
				btfl::SetEssenceState(btfl::STATE_NoneEssence);
			}
		}
	}
}

void MainPanel::UnzipGameFiles(const wxString& filePath, const wxString& fileName, GaugeResult onSuccess, GaugeResult onFailure)
{
	if ( !wxFileName::Exists(filePath) )
	{
		m_gaugeResult = onFailure;
		m_nextGaugeValue = 100;
		return;
	}

	std::thread thread([&, filePath, fileName, onSuccess, onFailure]()
		{
			{
				wxFFileInputStream inputStream(filePath + fileName);
				if ( !inputStream.IsOk() )
				{
					wxMutexLocker locker(m_nextGaugeValueMutex);
					m_gaugeResult = onFailure;
					m_nextGaugeValue = 100;
					return;
				}

				wxZipEntry* pZIPEntry = nullptr;
				wxZipInputStream ZIPStream(inputStream);
				int nFileCount = ZIPStream.GetTotalEntries();
				for ( int i = 0; i < nFileCount; i++ )
				{
					pZIPEntry = ZIPStream.GetNextEntry();

					if ( !ZIPStream.OpenEntry(*pZIPEntry) || !ZIPStream.CanRead() )
					{
						wxMutexLocker locker(m_nextGaugeValueMutex);
						m_gaugeResult = onFailure;
						m_nextGaugeValue = 100;
						return;
					}

					wxString sOutputFile = filePath + pZIPEntry->GetName();

					if ( pZIPEntry->IsDir() ) {
						int perm = pZIPEntry->GetMode();
						wxFileName::Mkdir(sOutputFile, perm, wxPATH_MKDIR_FULL);
					}
					else {
						wxFileOutputStream outputFileStream(sOutputFile);
						if ( !outputFileStream.IsOk() )
						{
							wxMutexLocker locker(m_nextGaugeValueMutex);
							m_gaugeResult = onFailure;
							m_nextGaugeValue = 100;
							delete pZIPEntry;
							return;
						}

						ZIPStream.Read(outputFileStream);
					}

					wxMutexLocker locker(m_nextGaugeValueMutex);
					m_nextGaugeValue = (double(i * 10) / nFileCount) + 90;
					delete pZIPEntry;
				}
			}

			wxRemoveFile(filePath + fileName);
			wxMutexLocker locker(m_nextGaugeValueMutex);
			m_gaugeResult = onSuccess;
			m_nextGaugeValue = 100;
		}
	);
	thread.detach();
}

void MainPanel::ChangeToRandomBgImage()
{
	if ( !m_bCanChangeBackground )
		return;

	srand(time(0));
	int nCurrentBackground = !m_vShownBackgrounds.IsEmpty() ? m_vShownBackgrounds.Last() : -1;
	int nNewBackground = -1;

	if ( m_vShownBackgrounds.Count() == 6 )
		m_vShownBackgrounds.Clear();

	do { nNewBackground = rand() % 6 + 1; } while ( m_vShownBackgrounds.Index(nNewBackground) != wxNOT_FOUND  || nNewBackground == nCurrentBackground);
	m_vShownBackgrounds.Add(nNewBackground);
	
	switch ( nNewBackground )
	{
	case 1:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Roc.jpg");
		break;
	case 2:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Cauldron.jpg");
		break;
	case 3:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Phoenix.jpg");
		break;
	case 4:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Sirius.jpg");
		break;
	case 5:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Spider.jpg");
		break;
	case 6:
		m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Worm.jpg");
		break;
	default: m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Roc.jpg");
	}

	m_bgRatio = (double)m_background.GetWidth() / m_background.GetHeight();
}

void MainPanel::PromptForBecomingEssence()
{
	if ( btfl::GetState() < btfl::LauncherState::STATE_ToInstallGame )
	{
		wxMessageBox("You must verify your ISO before becoming an Essence!");
		return;
	}
	else if ( btfl::GetEssenceState() != btfl::LauncherEssenceState::STATE_NoneEssence )
	{
		wxMessageBox("Already an Essence.");
		return;
	}

	wxPasswordEntryDialog passwordDialog(nullptr, "Please, as an Essence, input your special password.", "Welcome, Essence Of Dormin.");
	
	if ( passwordDialog.ShowModal() == wxID_OK )
	{
		if ( passwordDialog.GetValue() == utils::crypto::GetDecryptedString(btfl::GetEncryptedEssencePassword()) )
		{
			btfl::SetUserEssencePassword(passwordDialog.GetValue());
			btfl::SetEssenceState(btfl::LauncherEssenceState::STATE_ToInstallGameEssence);
			wxMessageBox("Welcome, Essence");
		}
		else
			wxMessageBox("You are not worthy.");
	}
}

void MainPanel::OnFrameButtons(wxSFShapeMouseEvent& event)
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

	default:
		break;
	}
}

void MainPanel::OnSelectIso(wxSFShapeMouseEvent& event)
{
	DoSelectIso();
}

void MainPanel::OnVerifyIso(wxSFShapeMouseEvent& event)
{
	// If the user still hasn't agreed to the disclaimer, make them agree to it again.
	// VerifyIso() will only be called once they do.
	if ( !btfl::HasUserAgreedToDisclaimer() )
	{
		m_mainFrame->ShowDisclaimer();
		return;
	}

	VerifyIso();
}

void MainPanel::OnInstall(wxSFShapeMouseEvent& event)
{
	DoInstallGame();
}

void MainPanel::OnPlay(wxSFShapeMouseEvent& event)
{
	if ( wxFileName::Exists(btfl::GetInstallFileName().GetFullPath() + "BTFL.exe") )
	{
		wxFileType* pFileType = wxTheMimeTypesManager->GetFileTypeFromExtension("exe");
		wxString sCommand = pFileType->GetOpenCommand(btfl::GetInstallFileName().GetFullPath() + "BTFL.exe");
		delete pFileType;

		if ( wxExecute(sCommand + " \"" + utils::crypto::GetEncryptedString("BTFL is cool") + "\"") != -1 )
		{
			if ( btfl::GetSettings().bCloseSelfOnGameLaunch )
				m_mainFrame->Close();
		}
		else
			wxMessageBox("There was a problem when launching the game."
				" Please make sure the download isn't corrupted and try again");
	}
}

void MainPanel::OnSettings(wxSFShapeMouseEvent& event)
{
	m_mainFrame->ShowSettings();
}

void MainPanel::OnInstallEssence(wxSFShapeMouseEvent& event)
{
	DoInstallEssenceGame();
}

void MainPanel::OnPlayEssence(wxSFShapeMouseEvent& event)
{
	wxString fullName(btfl::GetEssenceInstallFileName().GetFullPath() + "BTFL_Preview.exe");
	if ( wxFileName::Exists(fullName) )
	{
		wxFileType* pFileType = wxTheMimeTypesManager->GetFileTypeFromExtension("exe");
		wxString sCommand = pFileType->GetOpenCommand(fullName);
		delete pFileType;

		if ( wxExecute(sCommand + " \"" + utils::crypto::GetEncryptedString("BTFL is cool") + "\"") != -1 )
		{
			if ( btfl::GetSettings().bCloseSelfOnGameLaunch )
				m_mainFrame->Close();
		}
		else
			wxMessageBox("There was a problem when launching the preview."
				" Please make sure the download isn't corrupted and try again");
	}
}

void MainPanel::DrawBackground(wxDC& dc, bool fromPaint)
{
	BackgroundImageCanvas::DrawBackground(dc, fromPaint);

	if ( m_bgFadeColour.Alpha() != 0 )
	{
		dc.SetBrush(wxBrush(m_bgFadeColour));
		dc.DrawRectangle(wxPoint(0, 0), GetSize());
		dc.SetBrush(wxNullBrush);
	}
}

void MainPanel::DrawForeground(wxDC& dc, bool fromPaint)
{
	// Draw the logo independent of the background image.
	dc.DrawBitmap(m_logo, m_logox, m_logoy);

	dc.SetTextForeground(wxColour(255, 255, 255));
	dc.SetFont(m_fileLabelFont);
	dc.DrawText(m_fileLabel, m_fileLabelPos);

	dc.SetTextForeground(m_fileDescColour);
	dc.SetFont(m_fileDescFont);
	dc.DrawText(m_fileDesc, m_fileDescPos);

	dc.SetUserScale(m_fileBmpScale, m_fileBmpScale);
	dc.DrawBitmap(m_fileContainer, m_fileContPos.x, m_fileContPos.y, true);
	dc.DrawBitmap(m_fileBmp, m_fileBmpPos.x, m_fileBmpPos.y, true);
	dc.SetUserScale(1.0, 1.0);

	dc.SetTextForeground(wxColour(255, 255, 255, 95));
	dc.DrawText(btfl::GetInstalledLauncherVersion(), m_versionLabelPos);

	if ( btfl::GetEssenceState() != btfl::STATE_NoneEssence )
	{
		dc.SetTextForeground(wxColour(52, 199, 226));
		dc.DrawText("Essence Of Dormin", 5, 5);
	}
}

void MainPanel::OnSize(wxSizeEvent& event)
{
	BackgroundImageCanvas::OnSize(event);
	m_logox = (event.GetSize().x / 2) - (m_logo.GetWidth() / 2);
	m_logoy = 70;

	RepositionAll();
}

void MainPanel::OnMouseMove(wxMouseEvent& event)
{
	if ( m_bIsDraggingFrame )
	{
		wxPoint toMove = wxGetMousePosition() - m_dragStartMousePos;
		m_mainFrame->Move(m_dragStartFramePos + toMove);
		return;
	}

	if ( m_gauge )
		DoCheckGauge(false);

	DoAnimateBackground(false);
	BackgroundImageCanvas::OnMouseMove(event);

	bool bIsHoveringViewGuide = m_viewGuideRect.Contains(event.GetPosition());
	if ( bIsHoveringViewGuide != m_isHoveringViewGuide )
	{
		SetCursor((wxStockCursor)((wxCURSOR_DEFAULT * !bIsHoveringViewGuide) + (wxCURSOR_CLOSED_HAND * bIsHoveringViewGuide)));
		m_isHoveringViewGuide = bIsHoveringViewGuide;
	}
	else
	{
		if ( !bIsHoveringViewGuide && !m_gauge )
		{
			bool bIsHoveringFileCont = m_mainButton->GetId() == btfl::LauncherState::STATE_ToVerifyIso
				&& wxRect(
					m_fileBmpPos * m_fileBmpScale,
					m_fileBmp.GetSize() * m_fileBmpScale
				).Contains(event.GetPosition());

			if ( bIsHoveringFileCont != m_isHoveringFileCont )
			{
				SetCursor((wxStockCursor)((wxCURSOR_DEFAULT * !bIsHoveringFileCont) + (wxCURSOR_CLOSED_HAND * bIsHoveringFileCont)));
				m_isHoveringFileCont = bIsHoveringFileCont;
			}
		}
	}
}

void MainPanel::OnLeftDown(wxMouseEvent& event) {
	BackgroundImageCanvas::OnLeftDown(event);
	
	wxRect eRect(wxPoint(479, 140), wxPoint(489, 154));
	if ( eRect.Contains(event.GetPosition()) )
	{
		PromptForBecomingEssence();
	}

	if ( m_bShouldDeleteUpdateBtn )
	{
		DestroyButton(m_updateButton);
		RepositionAll();
		Refresh();

		m_bShouldDeleteUpdateBtn = false;
		SetCursor(wxCURSOR_DEFAULT);
		return;
	}

	if ( m_bShouldDeleteEssenceUpdateBtn )
	{
		DestroyButton(m_essenceUpdateButton);
		RepositionAll();
		Refresh();

		m_bShouldDeleteEssenceUpdateBtn = false;
		SetCursor(wxCURSOR_DEFAULT);
		return;
	}

	if ( m_dragSafeArea.Contains(event.GetPosition()) )
	{
		m_bIsDraggingFrame = true;
		m_dragStartMousePos = wxGetMousePosition();
		m_dragStartFramePos = m_mainFrame->GetPosition();
		CaptureMouse();
		return;
	}

	if ( m_isHoveringFileCont )
	{
		DoSelectIso();
	}
	else if ( m_isHoveringViewGuide )
	{
		wxLaunchDefaultBrowser("https://www.techwalla.com/articles/how-to-make-an-iso-from-a-ps2-game-disc");
	}
}

void MainPanel::OnLeftUp(wxMouseEvent& event)
{
	BackgroundImageCanvas::OnLeftUp(event);

	if ( HasCapture() )
	{
		m_bIsDraggingFrame = false;
		ReleaseCapture();
	}
}

void MainPanel::OnBgChangeTimer(wxTimerEvent& event)
{
	if ( !m_bCanChangeBackground )
	{
		m_bgChangeTimer.StartOnce(7000);
		return;
	}

	m_bDoFadeOut = true;
	m_bgAnimTimer.Start(8);
}

void MainPanel::DoAnimateBackground(bool refresh)
{
	if ( m_bDoFadeOut )
	{
		char currentAlpha = m_bgFadeColour.Alpha();
		m_bgFadeColour = wxColour(0, 0, 0, currentAlpha + 2U);

		if ( m_bgFadeColour.Alpha() >= 254U )
		{
			m_bgFadeColour = wxColour(0, 0, 0, 255U);
			m_bDoFadeOut = false;

			ChangeToRandomBgImage();
			m_bDoFadeIn = true;
		}
	}
	else if ( m_bDoFadeIn )
	{
		char currentAlpha = m_bgFadeColour.Alpha();
		m_bgFadeColour = wxColour(0, 0, 0, currentAlpha - 2U);

		if ( m_bgFadeColour.Alpha() <= 1U )
		{
			m_bgFadeColour = wxColour(0, 0, 0, 0);
			m_bDoFadeIn = false;
			m_bgChangeTimer.StartOnce(7000);
		}
	}

	BackgroundImageCanvas::DoAnimateBackground(refresh);
}