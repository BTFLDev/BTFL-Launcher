#include "MainFrame.h"

#include "wxmemdbg.h"

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
EVT_TIMER(TIMER_LoadPatchNotes, MainFrame::OnLoadPatchNotesTimer)
wxEND_EVENT_TABLE()


MainFrame::MainFrame(wxWindow* parent,
	wxWindowID id,
	const wxString& title,
	const wxPoint& pos,
	const wxSize& size,
	long style,
	const wxString& name) : wxFrame(parent, id, title, pos, size, style, name), m_loadPatchNotesTimer(this, TIMER_LoadPatchNotes)
{
	SetIcon(wxICON(aaaaBTFLIconNoText));
	SetBackgroundColour(wxColour(0, 0, 0));

	m_patchNotesWindow = new LeftSidebar(this, -1, wxDefaultPosition, wxSize(400, -1), wxBORDER_NONE);
	m_mainPanel = new MainPanel(&m_mainPanelManager, this, -1);

	m_seconPanel = new SecondaryPanel(&m_seconPanelManager, this);
	m_seconPanel->Hide();

	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(m_patchNotesWindow, wxSizerFlags(0).Expand());
	sizer->Add(m_mainPanel, wxSizerFlags(1).Expand());

	m_copyrightPanel = new wxPanel(this, -1, wxDefaultPosition, wxSize(-1, 40), wxBORDER_NONE);
	m_copyrightPanel->SetBackgroundColour(wxColour(0, 0, 0));

	wxStaticText* copyright = new wxStaticText(m_copyrightPanel, -1, "Beyond The Forbidden Lands is officialy classified as a modification of \"Shadow of the Colossus\" (2005) and ownership of the original game is required to play.");
	copyright->SetFont(wxFontInfo(11).FaceName("Times New Roman"));
	copyright->SetForegroundColour(wxColour(150, 150, 150));

	wxStaticText* readDisclaimers = new wxStaticText(m_copyrightPanel, -1, "Read Disclaimers");
	readDisclaimers->SetFont(wxFontInfo(11).FaceName("Times New Roman"));
	readDisclaimers->SetForegroundColour(wxColour(52, 199, 226));
	readDisclaimers->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnReadDisclaimer, this);
	readDisclaimers->SetCursor(wxCURSOR_CLOSED_HAND);

	wxBoxSizer* copyrightSizer = new wxBoxSizer(wxHORIZONTAL);
	copyrightSizer->AddSpacer(40);
	copyrightSizer->Add(copyright, wxSizerFlags(0).CenterVertical());
	copyrightSizer->AddStretchSpacer(1);
	copyrightSizer->Add(readDisclaimers, wxSizerFlags(0).CenterVertical());
	copyrightSizer->AddSpacer(40);

	m_copyrightPanel->SetSizer(copyrightSizer);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(m_seconPanel, wxSizerFlags(1).Expand());
	mainSizer->Add(sizer, wxSizerFlags(1).Expand());
	mainSizer->Add(m_copyrightPanel, wxSizerFlags(0).Expand());

	SetSizer(mainSizer);
	Bind(wxEVT_CLOSE_WINDOW, [&](wxCloseEvent& event)
		{
			m_mainPanelManager.SetShapeCanvas(nullptr);
			m_seconPanelManager.SetShapeCanvas(nullptr);

			event.Skip();
		});
}

void MainFrame::LoadPatchNotes()
{
	if ( m_nLoadPatchNotesAttempts < 5 ) {
		if ( !m_patchNotesWindow->Load() )
		{
			m_nLoadPatchNotesAttempts++;
			m_nLoadPatchNotesCountdown = 10;
			m_loadPatchNotesTimer.Start(1000);
		}
		else
		{
			m_loadPatchNotesTimer.Stop();
		}
	}
	else {
		m_patchNotesWindow->SetMessage(
			"\nCouldn't reach the servers.\nPlease check that you have a stable and working internet connection "
			"and restart the application."
		);
		m_loadPatchNotesTimer.Stop();
	}
}

void MainFrame::OnLoadPatchNotesTimer(wxTimerEvent& event) {
	if ( m_nLoadPatchNotesCountdown == 0 )
	{
		LoadPatchNotes();
	}
	else
	{
		m_patchNotesWindow->SetMessage(
			"\nCouldn't reach the servers.\nPlease check that you have a stable and working internet connection. "
			"Trying again in " + std::to_string(m_nLoadPatchNotesCountdown--) + " seconds..."
		);
	}
}

void MainFrame::SetState(btfl::LauncherState state)
{
	m_mainPanel->SetState(state);	
	if ( m_patchNotesWindow->SetState(state) )
		LoadPatchNotes();
}

void MainFrame::ShowMainPanel()
{
	Freeze();

	m_mainPanel->Show();
	m_patchNotesWindow->Show();
	m_copyrightPanel->Show();

	m_seconPanel->Hide();
	Layout();

	Thaw();
}

void MainFrame::ShowSecondaryPanel()
{
	Freeze();
	
	m_mainPanel->Hide();
	m_patchNotesWindow->Hide();
	m_copyrightPanel->Hide();

	m_seconPanel->Show();
	Layout();

	Thaw();
}

void MainFrame::ShowDisclaimer()
{
	Freeze();
	ShowSecondaryPanel();
	m_seconPanel->ShowDisclaimer();
	Thaw();
}

void MainFrame::ShowSettings()
{
	Freeze();
	ShowSecondaryPanel();
	m_seconPanel->ShowSettings();
	Thaw();
}

void MainFrame::OnReadDisclaimer(wxMouseEvent& event)
{
	wxBusyCursor cursor;
	ShowDisclaimer();
}