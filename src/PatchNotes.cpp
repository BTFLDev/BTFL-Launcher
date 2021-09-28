#include "PatchNotes.h"
#include "MainFrame.h"

#include <wx\richtext\richtextxml.h>
#include <wx\dcbuffer.h>
#include <wx\dcgraph.h>
#include <wx\sstream.h>

#include "wxmemdbg.h"

///////////////////////////////////////////////////////////////////////
/////////////////////////// HyperlinkPanel ////////////////////////////
///////////////////////////////////////////////////////////////////////


wxBEGIN_EVENT_TABLE(HyperlinkPanel, wxPanel)

EVT_SIZE(HyperlinkPanel::OnSize)
EVT_PAINT(HyperlinkPanel::OnPaint)

EVT_LEFT_DOWN(HyperlinkPanel::OnLeftDown)
EVT_ENTER_WINDOW(HyperlinkPanel::OnEnterWindow)
EVT_LEAVE_WINDOW(HyperlinkPanel::OnLeaveWindow)

wxEND_EVENT_TABLE()

HyperlinkPanel::HyperlinkPanel(wxWindow* parent, const wxString& url, const wxBitmap& bmp, const wxSize& size, const wxPoint& pos, long style) :
	wxPanel(parent, -1, pos, size, style)
{
	m_bitmap = bmp;
	m_url = url;
	m_bgRatio = (double)m_bitmap.GetWidth() / m_bitmap.GetHeight();

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

	SetCursor(wxCURSOR_CLOSED_HAND);
}

void HyperlinkPanel::RecalculateSelf()
{
	wxSize size = GetClientSize();
	double curRatio = (double)size.x / size.y;
	int paddingx2 = m_padding * 2;

	if ( curRatio > m_bgRatio )
	{
		m_bgScale = (double)(size.x - paddingx2) / m_bitmap.GetWidth();
		m_bgx = (((size.x + paddingx2) - ((double)m_bitmap.GetWidth() * m_bgScale)) / 2) / m_bgScale;
		m_bgy = m_padding;
	} else
	{
		m_bgScale = (double)(size.y - paddingx2) / m_bitmap.GetHeight();
		m_bgy = (((size.y + paddingx2) - ((double)m_bitmap.GetHeight() * m_bgScale)) / 2) / m_bgScale;
		m_bgx = m_padding;
	}

	Refresh();
}

void HyperlinkPanel::OnSize(wxSizeEvent& event)
{
	RecalculateSelf();
}

void HyperlinkPanel::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);

	dc.SetPen(*wxTRANSPARENT_PEN);
	dc.SetBrush(wxBrush(GetBackgroundColour()));
	dc.DrawRectangle(wxPoint(0, 0), GetSize());

	wxGCDC gdc(dc);
	gdc.GetGraphicsContext()->Scale(m_bgScale, m_bgScale);
	gdc.DrawBitmap(m_bitmap, m_bgx, m_bgy, true);
	gdc.GetGraphicsContext()->Scale(1.0 / m_bgScale, 1.0 / m_bgScale);

	if ( !m_isHovering )
	{
		wxSize size = GetSize();
		size.x += 20;
		size.y += 20;
		gdc.SetBrush(wxBrush(wxColour(0, 0, 0, 125)));
		gdc.DrawRectangle(wxPoint(-10, -10), size);
	}
}

void HyperlinkPanel::OnLeftDown(wxMouseEvent& event)
{
	wxLaunchDefaultBrowser(m_url);
}

void HyperlinkPanel::OnEnterWindow(wxMouseEvent& event)
{
	m_isHovering = true;
	Refresh();
}

void HyperlinkPanel::OnLeaveWindow(wxMouseEvent& event)
{
	m_isHovering = false;
	Refresh();
}


///////////////////////////////////////////////////////////////////////
///////////////////////////// LeftSidebar /////////////////////////////
///////////////////////////////////////////////////////////////////////


LeftSidebarRTC::LeftSidebarRTC(wxWindow* parent, const wxString& fileToLoad) :
	wxPanel(parent, -1, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE), RTCFileLoader(this)
{
	m_rtc = new ReadOnlyRTC(this, -1, "", wxDefaultPosition, wxDefaultSize, wxBORDER_NONE);
	m_rtc->ShowScrollbars(wxSHOW_SB_NEVER, wxSHOW_SB_NEVER);
	m_rtc->SetScrollRate(15, 15);

	m_scrollbar = new CustomRTCScrollbar(this, m_rtc, -1);

	m_sFileToLoad = fileToLoad;

	wxBoxSizer* pSizer = new wxBoxSizer(wxHORIZONTAL);
	pSizer->AddSpacer(5);
	pSizer->Add(m_scrollbar, wxSizerFlags(0).Expand());
	pSizer->Add(m_rtc, wxSizerFlags(1).Expand());

	SetSizer(pSizer);
}


BEGIN_EVENT_TABLE(LeftSidebar, wxScrolledWindow)
EVT_MOTION(LeftSidebar::OnMove)
EVT_PAINT(LeftSidebar::OnPaint)
END_EVENT_TABLE()

LeftSidebar::LeftSidebar(wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxPanel(parent, id, pos, size, style)
{
	m_mainFrame = (MainFrame*)parent;

	SetBackgroundColour(wxColour(0, 0, 0));

	wxRichTextBuffer::AddHandler(new wxRichTextXMLHandler);

	m_welcomePage = new LeftSidebarRTC(this, "welcome.xml");
	m_patchNotesPage = new LeftSidebarRTC(this, "notes.xml");
	m_welcomePage->Show();
	m_patchNotesPage->Hide();

	m_welcomeLabel = new wxStaticText(this, -1, "Welcome");
	m_welcomeLabel->SetForegroundColour(wxColour(250, 250, 250));
	m_welcomeLabel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { this->ShowWelcome(); });
	m_welcomeLabel->Bind(wxEVT_ENTER_WINDOW, [&](wxMouseEvent&)
		{
			m_welcomeLabel->SetForegroundColour(wxColour(200, 200, 200));
			m_welcomeLabel->Refresh();
			m_welcomeLabel->Update();
		}
	);
	m_welcomeLabel->Bind(wxEVT_LEAVE_WINDOW, [&](wxMouseEvent& event)
		{
			m_welcomeLabel->SetForegroundColour(wxColour(250, 250, 250));
			m_welcomeLabel->Refresh();
			m_welcomeLabel->Update();
		}
	);
	m_welcomeLabel->SetCursor(wxCURSOR_CLOSED_HAND);

	m_patchNotesLabel = new wxStaticText(this, -1, "Patch Notes");
	m_patchNotesLabel->SetFont(wxFontInfo(10).FaceName("Lora"));
	m_patchNotesLabel->SetForegroundColour(wxColour(250, 250, 250));
	m_patchNotesLabel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent&) { this->ShowPatchNotes(); });
	m_patchNotesLabel->Bind(wxEVT_ENTER_WINDOW, [&](wxMouseEvent&)
		{
			m_patchNotesLabel->SetForegroundColour(wxColour(200, 200, 200));
			m_patchNotesLabel->Refresh();
			m_patchNotesLabel->Update();
		}
	);
	m_patchNotesLabel->Bind(wxEVT_LEAVE_WINDOW, [&](wxMouseEvent& event)
		{
			m_patchNotesLabel->SetForegroundColour(wxColour(250, 250, 250));
			m_patchNotesLabel->Refresh();
			m_patchNotesLabel->Update();
		}
	);
	m_patchNotesLabel->SetCursor(wxCURSOR_CLOSED_HAND);

	wxBoxSizer* pLabelSizer = new wxBoxSizer(wxHORIZONTAL);
	pLabelSizer->Add(m_welcomeLabel);
	pLabelSizer->AddStretchSpacer(1);
	pLabelSizer->Add(m_patchNotesLabel);

	HyperlinkPanel* website = new HyperlinkPanel(this, "http://btflgame.com", wxBitmap("Assets/Icon/Website@2x.png", wxBITMAP_TYPE_PNG), wxSize(26, 26));
	website->SetBackgroundColour(wxColour(0, 0, 0));
	HyperlinkPanel* discord = new HyperlinkPanel(this, "https://discord.gg/PcjByKk", wxBitmap("Assets/Icon/Discord@2x.png", wxBITMAP_TYPE_PNG), wxSize(26, 26));
	discord->SetBackgroundColour(wxColour(0, 0, 0));
	HyperlinkPanel* reddit = new HyperlinkPanel(this, "https://reddit.com/r/BTFLgame", wxBitmap("Assets/Icon/Reddit@2x.png", wxBITMAP_TYPE_PNG), wxSize(26, 26));
	reddit->SetBackgroundColour(wxColour(0, 0, 0));
	HyperlinkPanel* twitter = new HyperlinkPanel(this, "http://twitter.com/btfl_game", wxBitmap("Assets/Icon/Twitter@2x.png", wxBITMAP_TYPE_PNG), wxSize(26, 26));
	twitter->SetBackgroundColour(wxColour(0, 0, 0));
	HyperlinkPanel* youtube = new HyperlinkPanel(this, "https://www.youtube.com/channel/UC4Z1YwJ0fAMt5MU0y2melOA/", wxBitmap("Assets/Icon/YouTube@2x.png", wxBITMAP_TYPE_PNG), wxSize(26, 26));
	youtube->SetBackgroundColour(wxColour(0, 0, 0));

	wxBoxSizer* socialSizer = new wxBoxSizer(wxHORIZONTAL);
	socialSizer->Add(website, wxSizerFlags(0).Border(wxALL, 10));
	socialSizer->Add(discord, wxSizerFlags(0).Border(wxALL, 10));
	socialSizer->Add(reddit, wxSizerFlags(0).Border(wxALL, 10));
	socialSizer->Add(twitter, wxSizerFlags(0).Border(wxALL, 10));
	socialSizer->Add(youtube, wxSizerFlags(0).Border(wxALL, 10));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->AddSpacer(5);
	sizer->Add(m_welcomePage, wxSizerFlags(1).Expand());
	sizer->Add(m_patchNotesPage, wxSizerFlags(1).Expand());
	sizer->AddSpacer(2);
	sizer->Add(pLabelSizer, wxSizerFlags(0).Expand().Border(wxALL, 10));
	sizer->Add(socialSizer, wxSizerFlags(0).CenterHorizontal());

	SetSizer(sizer);

	m_welcomePage->StartLoadLoop();
	m_patchNotesPage->StartLoadLoop();
}

void LeftSidebar::SetState(btfl::LauncherState state)
{
	switch ( state )
	{
	case btfl::LauncherState::STATE_ToSelectIso:
	case btfl::LauncherState::STATE_ToVerifyIso:
	case btfl::LauncherState::STATE_VerifyingIso:
	case btfl::LauncherState::STATE_VerificationFailed:
		ShowWelcome();

	case btfl::LauncherState::STATE_ToInstallGame:
	case btfl::LauncherState::STATE_InstallingGame:
	case btfl::LauncherState::STATE_ToPlayGame:
	case btfl::LauncherState::STATE_ToUpdateGame:
	case btfl::LauncherState::STATE_UpdatingGame:
		ShowPatchNotes();
	}
}

void LeftSidebar::ShowWelcome()
{
	if ( m_bIsWelcomeShown )
		return;

	Freeze();
	
	m_patchNotesPage->Hide();
	m_welcomePage->Show();

	m_patchNotesLabel->SetFont(wxFontInfo(10).FaceName("Lora"));
	m_welcomeLabel->SetFont(wxFontInfo(10).FaceName("Lora").Underlined());

	GetSizer()->Layout();
	m_welcomePage->LayoutRTC();
	Thaw();

	m_bIsWelcomeShown = true;
}

void LeftSidebar::ShowPatchNotes()
{
	if ( !m_bIsWelcomeShown )
		return;

	Freeze();

	m_welcomePage->Hide();
	m_patchNotesPage->Show();

	m_welcomeLabel->SetFont(wxFontInfo(10).FaceName("Lora"));
	m_patchNotesLabel->SetFont(wxFontInfo(10).FaceName("Lora").Underlined());

	GetSizer()->Layout();
	m_patchNotesPage->LayoutRTC();
	Thaw();

	m_bIsWelcomeShown = false;
}

void LeftSidebar::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);

	wxSize size = GetClientSize();
	wxRect rtcRect;
	if ( m_bIsWelcomeShown )
		rtcRect = m_welcomePage->GetRect();
	else
		rtcRect = m_patchNotesPage->GetRect();

	int middleX = size.x / 2;

	wxColour gray(100, 100, 100);
	wxColour black(0, 0, 0);

	wxSize gradientHalfSize(middleX, 1);

	dc.GradientFillLinear(wxRect(wxPoint(0, rtcRect.GetBottom() + 1), gradientHalfSize), black, gray);
	dc.GradientFillLinear(wxRect(wxPoint(middleX, rtcRect.GetBottom() + 1), gradientHalfSize), gray, black);

	dc.GradientFillLinear(wxRect(wxPoint(0, m_welcomeLabel->GetRect().GetBottom() + 10), gradientHalfSize), black, gray);
	dc.GradientFillLinear(wxRect(wxPoint(middleX, m_welcomeLabel->GetRect().GetBottom() + 10), gradientHalfSize), gray, black);

	dc.GradientFillLinear(wxRect(wxPoint(0, size.y - 1), gradientHalfSize), black, gray);
	dc.GradientFillLinear(wxRect(wxPoint(middleX, size.y - 1), gradientHalfSize), gray, black);
}

void LeftSidebar::OnMove(wxMouseEvent& event)
{

}
