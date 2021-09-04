#include "BaseClasses.h"

#include "wxmemdbg.h"

ReadOnlyRTC::ReadOnlyRTC(wxWindow* parent,
	wxWindowID id,
	const wxString& value,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxRichTextCtrl(parent, id, value, pos, size, style)
{
	SetCursor(wxCURSOR_DEFAULT);
	SetTextCursor(wxCURSOR_DEFAULT);
	Bind(wxEVT_SET_FOCUS, [](wxFocusEvent&) {});
	Bind(wxEVT_LEFT_DOWN, [](wxMouseEvent&) {});
	Bind(wxEVT_RIGHT_UP, [](wxMouseEvent&) {});
	Bind(wxEVT_CHAR, [](wxKeyEvent&) {});
	Bind(wxEVT_KEY_DOWN, [](wxKeyEvent&) {});

	wxRichTextAttr attr;
	attr.SetFont(wxFontInfo(12).FaceName("Times New Roman"));
	attr.SetAlignment(wxTEXT_ALIGNMENT_JUSTIFIED);
	attr.SetLeftIndent(64);
	attr.SetRightIndent(64);
	attr.SetLineSpacing(10);
	attr.SetTextColour(wxColour(210, 210, 210));

	SetBasicStyle(attr);
	SetBackgroundColour(wxColour(0, 0, 0));
	SetFontScale(1.13);

	m_shadowBitmap.LoadFile("Assets\\Scroll Shadow\\Large@2x.png", wxBITMAP_TYPE_PNG);
}

void ReadOnlyRTC::PaintAboveContent(wxDC& dc)
{
	int yo;
	CalcUnscrolledPosition(0, 0, nullptr, &yo);
	wxSize size = GetClientSize();

	double scale = (double)size.x / m_shadowBitmap.GetWidth();
	dc.SetUserScale(scale, scale);
	dc.DrawBitmap(m_shadowBitmap, wxPoint(0, (size.y + yo - ((double)m_shadowBitmap.GetHeight() * scale)) / scale), true);
	dc.SetUserScale(1.0, 1.0);
}


//////////////////////////////////////////////////////////////////
////////////////////// BackgroundImageCanvas /////////////////////
//////////////////////////////////////////////////////////////////


BackgroundImageCanvas::BackgroundImageCanvas(wxSFDiagramManager* manager,
	wxWindow* parent,
	wxWindowID id,
	const wxPoint& pos,
	const wxSize& size,
	long style) : wxSFShapeCanvas(manager, parent, id, pos, size, style)
{
	EnableGC(true);
	SetHoverColour(wxColour(255, 255, 255));

	RemoveStyle(sfsMULTI_SELECTION);
	RemoveStyle(sfsPROCESS_MOUSEWHEEL);

	Bind(wxEVT_SIZE, &BackgroundImageCanvas::_OnSize, this);
	Bind(wxEVT_ENTER_WINDOW, &BackgroundImageCanvas::OnEnterWindow, this);
	Bind(wxEVT_LEAVE_WINDOW, &BackgroundImageCanvas::OnLeaveWindow, this);

	m_bgAnimTimer.SetOwner(&m_bgAnimTimer, TIMER_Background);
	m_bgAnimTimer.Bind(wxEVT_TIMER, &BackgroundImageCanvas::OnBgAnimTimer, this);
}

void BackgroundImageCanvas::OnSize(wxSizeEvent& event)
{
	wxSize size = GetSize();
	double curRatio = (double)size.x / size.y;
	int overallOffset = MAX_BG_OFFSET * 2;

	if ( curRatio > m_bgRatio )
	{
		m_bgScale = (double)(size.x + overallOffset) / m_background.GetWidth();
		m_bgy = (double)(((size.y - overallOffset) - ((double)m_background.GetHeight() * m_bgScale)) / 2) / m_bgScale;
		m_bgx = -MAX_BG_OFFSET;
	} else
	{
		m_bgScale = (double)(size.y + overallOffset) / m_background.GetHeight();
		m_bgx = (double)(((size.x - overallOffset) - ((double)m_background.GetWidth() * m_bgScale)) / 2) / m_bgScale;
		m_bgy = -MAX_BG_OFFSET;
	}

	InvalidateVisibleRect();
}

void BackgroundImageCanvas::DrawBackground(wxDC& dc, bool fromPaint)
{
	dc.SetUserScale(m_bgScale, m_bgScale);
	dc.DrawBitmap(m_background, m_bgx + m_bgxoffset, m_bgy + m_bgyoffset);
	dc.SetUserScale(1.0, 1.0);
}

void BackgroundImageCanvas::OnUpdateVirtualSize(wxRect& rect)
{
	rect.SetTopLeft(wxPoint(0, 0));
	rect.SetSize(GetClientSize());
}

void BackgroundImageCanvas::OnMouseMove(wxMouseEvent& event)
{
	wxSize size = GetClientSize();
	wxPoint pos = event.GetPosition();

	m_bgxoffset = (double)((m_mousePosOnEnter.x - pos.x) * MAX_BG_OFFSET) / size.x;
	m_bgyoffset = (double)((m_mousePosOnEnter.y - pos.y) * MAX_BG_OFFSET) / size.y;

	InvalidateVisibleRect();
	wxSFShapeCanvas::OnMouseMove(event);
}

void BackgroundImageCanvas::OnEnterWindow(wxMouseEvent& event)
{
	m_bgAnimTimer.Stop();
	m_mousePosOnEnter = event.GetPosition();
}

void BackgroundImageCanvas::OnLeaveWindow(wxMouseEvent& event)
{
	// Do this so that the canvas unhovers any shape that it
	// thinks is still under the mouse
	wxPoint posCache = event.GetPosition();
	event.SetPosition(wxPoint(0, 0));
	wxSFShapeCanvas::OnMouseMove(event);
	event.SetPosition(posCache);

	m_bgAnimTimer.Start(1);
}

void BackgroundImageCanvas::OnBgAnimTimer(wxTimerEvent& event)
{
	if ( m_bgxoffset > 1 )
		m_bgxoffset -= 2;
	else if ( m_bgxoffset < -1 )
		m_bgxoffset += 2;

	if ( m_bgyoffset > 1 )
		m_bgyoffset -= 2;
	else if ( m_bgyoffset < -1 )
		m_bgyoffset += 2;

	if ( (m_bgxoffset > -2 && m_bgxoffset < 2) && (m_bgyoffset > -2 && m_bgyoffset < 2) )
	{
		m_bgxoffset = 0;
		m_bgyoffset = 0;
		m_bgAnimTimer.Stop();
	}

	Refresh();
	Update();
}

void BackgroundImageCanvas::_OnSize(wxSizeEvent& event)
{
	this->OnSize(event);
	event.Skip();
}

RTCFileLoader::RTCFileLoader(wxEvtHandler* evtHandler) : m_loadTimer(evtHandler, 12345)
{
	m_evtHandler = evtHandler;

	m_evtHandler->Bind(wxEVT_WEBREQUEST_STATE, [&](wxWebRequestEvent& evt) { OnWebRequestChanged(evt); });
	m_evtHandler->Bind(wxEVT_TIMER, [&](wxTimerEvent& evt) { OnLoadTimer(evt); });
}

bool RTCFileLoader::StartLoadLoop()
{
	if ( !m_rtc )
		return false;

	if ( m_nLoadAttempts >= 5 )
	{
		SetMessage(m_sFinalError);
		m_nLoadAttempts = 0;
		return false;
	}

	SetMessage(m_sPlaceholder);

	m_webRequest = wxWebSession::GetDefault().CreateRequest(m_evtHandler, m_sUrl);
	if ( !m_webRequest.IsOk() )
		return false;

	m_webRequest.Start();
	return true;
}

void RTCFileLoader::OnWebRequestChanged(wxWebRequestEvent& event)
{
	if ( !m_rtc )
		return;

	switch ( event.GetState() )
	{
	case wxWebRequest::State_Completed:
	{
		wxRichTextBuffer buffer;
		buffer.LoadFile(*event.GetResponse().GetStream());

		if ( !buffer.IsEmpty() )
		{
			buffer.SetBasicStyle(m_rtc->GetBasicStyle());

			m_rtc->GetBuffer() = buffer;
			m_rtc->LayoutContent();
		}

		OnFileLoaded();
		break;
	}
	case wxWebRequest::State_Unauthorized:
	case wxWebRequest::State_Failed:
	case wxWebRequest::State_Cancelled:
		m_nLoadAttempts++;
		m_loadTimer.Start(1000);
		break;
	}
}

void RTCFileLoader::OnLoadTimer(wxTimerEvent& event)
{
	if ( event.GetId() != 12345 )
	{
		event.Skip();
		return;
	}

	if ( m_nLoadRetryCountdown == 0 || !m_rtc )
	{
		m_nLoadRetryCountdown = 10;
		m_loadTimer.Stop();
		StartLoadLoop();
	}
	else
	{
		SetMessage(
			"\nCouldn't reach the servers.\nPlease check that you have a stable and working internet connection. "
			"Trying again in " + std::to_string(m_nLoadRetryCountdown--) + " seconds..."
		);
	}
}

void RTCFileLoader::SetMessage(const wxString& message)
{
	m_rtc->SetValue(message);
	m_rtc->Refresh();
	m_rtc->Update();
}