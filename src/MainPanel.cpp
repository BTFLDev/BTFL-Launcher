#include "MainPanel.h"

#include "MainFrame.h"
#include "MyApp.h"

#include <thread>
#include <wx/zipstrm.h>
#include <wx/sstream.h>
#include <wx/mimetype.h>

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

	m_background = utils::crypto::GetDecryptedImage("Assets/Background/Main Page Roc.png");
	m_logo.LoadFile("Assets/LauncherLogo.png", wxBITMAP_TYPE_PNG);
	m_bgRatio = (double)m_background.GetWidth() / m_background.GetHeight();

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
		
		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_InstallingGame:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("INSTALLING...");
		m_mainButton->Enable(false);
		m_mainButton->SetState(Processing);

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

		DestroyGauge();
		break;

	case btfl::LauncherState::STATE_UpdatingGame:
		m_mainButton->SetBitmap(wxNullBitmap);
		m_mainButton->SetLabel("UPDATING...");
		m_mainButton->Enable(false);
		m_mainButton->SetState(Processing);

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

	m_xFCont = (double)mainButtonRect.GetLeft() / m_fileBmpScale;
	m_yFCont = (double)(mainButtonRect.GetTop() - ((double)m_fileContainer.GetHeight() * m_fileBmpScale) - 10) / m_fileBmpScale;

	m_xFBmp = m_xFCont + 20;
	m_yFBmp = m_yFCont + (((double)m_fileBmp.GetHeight() * m_fileBmpScale) / 2) - 4;

	m_xFLabel = (double)(m_xFBmp + m_fileBmp.GetWidth() + 10) * m_fileBmpScale;
	m_yFLabel = ((double)m_yFCont * m_fileBmpScale) + 6;

	wxClientDC dc(this);
	dc.SetFont(m_fileDescFont);
	wxSize textSize = dc.GetTextExtent(m_fileDesc);

	m_xFDesc = m_xFLabel;
	m_yFDesc = ((double)(m_yFCont + m_fileContainer.GetHeight()) * m_fileBmpScale) - textSize.y - 4;
	if ( btfl::GetState() == btfl::STATE_ToSelectIso  || btfl::GetState() == btfl::STATE_VerificationFailed )
		m_viewGuideRect = { wxPoint(m_xFDesc, m_yFDesc), textSize };
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

			wxString fileHash = iso::GetFileHash(btfl::GetIsoFileName().GetFullPath(), m_nextGaugeValue);
			
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

void MainPanel::UpdateGauge(int currentUnit, const wxString & message)
{
	if ( !m_gauge )
		return;

	m_gauge->UpdateValue(currentUnit);
	m_gaugeLabel->SetText(message);
	m_gaugeProgress->SetText(std::to_string(m_gauge->GetCurrentPercent()) + "% Complete");
	m_gauge->Refresh(true);

	RepositionAll();
	Refresh();
	Update();
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

void MainPanel::DestroyUpdateButton()
{
	if ( !m_updateButton )
		return;

	GetDiagramManager()->RemoveShape(m_updateButton, false);
	m_updateButton = nullptr;
}

void MainPanel::OnGaugeTimer(wxTimerEvent& event)
{
	wxMutexLocker locker(m_nextGaugeValueMutex);

	// This isn't what I wanted but the OnInstallWebRequestData event doesn't seem to work
	btfl::LauncherState currentState = btfl::GetState();
	if ( currentState == btfl::STATE_InstallingGame || currentState == btfl::STATE_UpdatingGame )
	{
		int nValue = (m_webRequest.GetBytesReceived() * 90) /
			m_webRequest.GetBytesExpectedToReceive();
	
		if ( nValue != 90 )
			m_nextGaugeValue = nValue;
	}

	if ( m_nextGaugeValue != -1 )
	{
		UpdateGauge(m_nextGaugeValue, m_nextGaugeLabel);
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
	}
}

void MainPanel::DoSelectIso()
{
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

}

void MainPanel::DoInstallGame()
{
	btfl::LauncherState currentState = btfl::GetState();
	if ( currentState != btfl::STATE_ToInstallGame && currentState != btfl::STATE_ToUpdateGame )
		return;

	wxFileName installFileName = btfl::GetInstallFileName();
	if ( !installFileName.Exists() )
		wxMkdir(installFileName.GetFullPath(), wxS_DIR_DEFAULT);

	m_webRequest = wxWebSession::GetDefault().CreateRequest(
		this,
		btfl::GetStorageURL() + "game/BTFL.zip",
		WEB_Install
	);
	m_webRequest.SetStorage(wxWebRequest::Storage_File);
	wxWebSession::GetDefault().SetTempDir(installFileName.GetFullPath());

	if ( !m_webRequest.IsOk() )
	{
		wxMessageBox("Unable to start installation. Please check your internet connection and try again.");
		return;
	}
	
	m_nextGaugeLabel = "...";
	switch ( currentState )
	{
	case btfl::STATE_ToInstallGame:
		btfl::SetState(btfl::STATE_InstallingGame);
		break;

	case btfl::STATE_ToUpdateGame:
		btfl::SetState(btfl::STATE_UpdatingGame);
		break;
	}

	m_webRequest.SetHeader(
		utils::crypto::GetDecryptedString("Bxujpuj}bvjro"),
		utils::crypto::GetDecryptedString("urlgo#hkqaofv[mKr7ijR|GhH\\Spm48ygpzY4nKT4m6MhN"));
	m_webRequest.Start();
}

void MainPanel::DoLookForLauncherUpdates()
{
	m_webRequest = wxWebSession::GetDefault().CreateRequest(
		this,
		btfl::GetStorageURL() + "launcher/version.txt",
		WEB_LookForLauncherUpdate
	);
	m_webRequest.SetStorage(wxWebRequest::Storage_Memory);

	if ( !m_webRequest.IsOk() )
		return;

	m_webRequest.SetHeader(
		utils::crypto::GetDecryptedString("Bxujpuj}bvjro"),
		utils::crypto::GetDecryptedString("urlgo#hkqaofv[mKr7ijR|GhH\\Spm48ygpzY4nKT4m6MhN"));
	m_webRequest.Start();
}

void MainPanel::DoLookForGameUpdates()
{
	m_webRequest = wxWebSession::GetDefault().CreateRequest(
		this,
		btfl::GetStorageURL() + "game/version.txt",
		WEB_LookForGameUpdate
	);
	m_webRequest.SetStorage(wxWebRequest::Storage_Memory);

	if ( !m_webRequest.IsOk() )
		return;

	m_webRequest.SetHeader(
		utils::crypto::GetDecryptedString("Bxujpuj}bvjro"),
		utils::crypto::GetDecryptedString("urlgo#hkqaofv[mKr7ijR|GhH\\Spm48ygpzY4nKT4m6MhN"));
	m_webRequest.Start();
}

void MainPanel::OnWebRequestState(wxWebRequestEvent& event)
{
	switch ( event.GetId() )
	{
	case WEB_Install:
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

			wxFileName fileName(event.GetDataFile());
			fileName.SetName("BTFL");
			fileName.SetExt("zip");

			if ( wxRenameFile(event.GetDataFile(), fileName.GetFullPath()) )
			{
				UnzipGameFiles();
			}
			else
			{
				wxMessageBox("There was a problem when installing the game.\n"
					"Please try again and, if the problem persists, contact the team at the Discord Server.");
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
			wxMessageBox("There was a problem when downloading the game. Please check your internet connection and try again.");
			btfl::RestoreLastState();
			break;
		}

		break;

	case WEB_LookForLauncherUpdate:
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

			break;
		}

		break;
	case WEB_LookForGameUpdate:
		if ( event.GetState() == wxWebRequest::State_Completed )
		{
			wxStringInputStream* sstream = (wxStringInputStream*)event.GetResponse().GetStream();
			wxStringOutputStream outStream;
			sstream->Read(outStream);

			if ( !outStream.GetString().IsEmpty() )
			{
				btfl::SetLatestGameVerstion(outStream.GetString());
				if ( btfl::GetInstalledGameVersion() != outStream.GetString() )
				{
					if ( btfl::GetState() == btfl::STATE_ToPlayGame && btfl::ShouldAutoUpdateGame() )
						btfl::SetState(btfl::STATE_ToUpdateGame);
				}
			}

			break;
		}
		break;
	}
	
}

void MainPanel::UnzipGameFiles()
{
	if ( !wxFileName::Exists(btfl::GetInstallFileName().GetFullPath()) )
	{
		m_gaugeResult = GAUGE_InstallationUnsuccessful;
		m_nextGaugeValue = 100;
		return;
	}

	std::thread thread([&]()
		{
			wxString sPath(btfl::GetInstallFileName().GetFullPath());

			{
				wxFFileInputStream inputStream(sPath + "BTFL.zip");
				if ( !inputStream.IsOk() )
				{
					wxMutexLocker locker(m_nextGaugeValueMutex);
					m_gaugeResult = GAUGE_InstallationUnsuccessful;
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
						m_gaugeResult = GAUGE_InstallationUnsuccessful;
						m_nextGaugeValue = 100;
						return;
					}

					wxString sOutputFile = sPath + pZIPEntry->GetName();

					if ( pZIPEntry->IsDir() ) {
						int perm = pZIPEntry->GetMode();
						wxFileName::Mkdir(sOutputFile, perm, wxPATH_MKDIR_FULL);
					}
					else {
						wxFileOutputStream outputFileStream(sOutputFile);
						if ( !outputFileStream.IsOk() )
						{
							wxMutexLocker locker(m_nextGaugeValueMutex);
							m_gaugeResult = GAUGE_InstallationUnsuccessful;
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

			wxRemoveFile(sPath + "BTFL.zip");
			wxMutexLocker locker(m_nextGaugeValueMutex);
			m_gaugeResult = GAUGE_InstallationSuccessful;
			m_nextGaugeValue = 100;
		}
	);
	thread.detach();
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

void MainPanel::DrawForeground(wxDC& dc, bool fromPaint)
{
	// Draw the logo independent of the background image.
	dc.DrawBitmap(m_logo, m_logox, m_logoy);

	dc.SetTextForeground(wxColour(255, 255, 255));
	dc.SetFont(m_fileLabelFont);
	dc.DrawText(m_fileLabel, m_xFLabel, m_yFLabel);

	dc.SetTextForeground(m_fileDescColour);
	dc.SetFont(m_fileDescFont);
	dc.DrawText(m_fileDesc, m_xFDesc, m_yFDesc);

	dc.SetUserScale(m_fileBmpScale, m_fileBmpScale);
	dc.DrawBitmap(m_fileContainer, m_xFCont, m_yFCont, true);
	dc.DrawBitmap(m_fileBmp, m_xFBmp, m_yFBmp, true);
	dc.SetUserScale(1.0, 1.0);

	dc.SetTextForeground(wxColour(255, 255, 255, 95));
	dc.DrawText(btfl::GetInstalledLauncherVersion(), m_versionLabelPos);
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
					wxPoint(m_xFBmp, m_yFBmp) * m_fileBmpScale,
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

	if ( m_bShouldDeleteUpdateBtn )
	{
		DestroyUpdateButton();
		RepositionAll();
		Refresh();

		m_bShouldDeleteUpdateBtn = false;
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
