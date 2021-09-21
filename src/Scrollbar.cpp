#include "Scrollbar.h"

#include <wx\dcbuffer.h>
#include <wx\dcgraph.h>

#include "wxmemdbg.h"

wxBEGIN_EVENT_TABLE(CustomRTCScrollbar, wxPanel)

EVT_SIZE(CustomRTCScrollbar::OnSize)
EVT_PAINT(CustomRTCScrollbar::OnPaint)

EVT_LEFT_DOWN(CustomRTCScrollbar::OnLeftDown)
EVT_LEFT_UP(CustomRTCScrollbar::OnLeftUp)

EVT_MOTION(CustomRTCScrollbar::OnMouseMove)
EVT_LEAVE_WINDOW(CustomRTCScrollbar::OnLeaveWindow)
EVT_MOUSE_CAPTURE_LOST(CustomRTCScrollbar::OnMouseCaptureLost)

EVT_TIMER(TIMER_SetOffOnHold, CustomRTCScrollbar::OnSetOffOnHold)
EVT_TIMER(TIMER_ScrollOnHold, CustomRTCScrollbar::OnScrollOnHold)

wxEND_EVENT_TABLE()

CustomRTCScrollbar::CustomRTCScrollbar(wxWindow* parent, wxRichTextCtrl* target, wxWindowID id, int orientation,
	const wxPoint& pos, const wxSize& size) : wxControl(parent, id, pos, size, wxBORDER_NONE),
	m_setOffOnHold(this, TIMER_SetOffOnHold), m_scrollOnHold(this, TIMER_ScrollOnHold)
{
	m_rtc = target;
	m_orientation = orientation;

	if ( size != wxDefaultSize )
		m_nMinWidth = size.y;

	SetMinSize(wxSize(m_nMinWidth, -1));

	SetBackgroundStyle(wxBG_STYLE_PAINT);
	Bind(wxEVT_ERASE_BACKGROUND, [](wxEraseEvent&) {});

	SetBackgroundColour(wxColour(0, 0, 0));
	SetForegroundColour(wxColour(255, 255, 255));

	m_rtc->ShowScrollbars(wxSHOW_SB_DEFAULT, wxSHOW_SB_NEVER);

	m_rtc->Bind(wxEVT_SIZE, &CustomRTCScrollbar::OnTargetSize, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_BOTTOM, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_TOP, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_LINEDOWN, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_LINEUP, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_PAGEDOWN, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_PAGEUP, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &CustomRTCScrollbar::OnTargetScroll, this);
	m_rtc->Bind(wxEVT_SCROLLWIN_THUMBTRACK, &CustomRTCScrollbar::OnTargetScroll, this);

	Hide();
}

void CustomRTCScrollbar::RecalculateSelf()
{
	wxSize size = GetSize();
	wxSize rtcSize = m_rtc->GetClientSize();
	wxSize rtcVirtualSize = m_rtc->GetVirtualSize();

	if ( rtcVirtualSize.y <= size.y )
	{
		if ( IsShown() )
			Hide();

		return;
	}
	else
	{
		if ( !IsShown() )
		{
			Show();
			SendSizeEventToParent();
		}
	}

	if ( m_orientation == wxVERTICAL )
	{
		m_dRatio = (double)size.y / rtcVirtualSize.y;

		int yo;
		m_rtc->CalcUnscrolledPosition(0, 0, nullptr, &yo);
		m_nCurrentPos = yo * m_dRatio;

		int nThumbHeight = int(rtcSize.y * m_dRatio);
		if ( nThumbHeight < 20 )
		{
			m_dPrivateScale = 20.0 / nThumbHeight;
			nThumbHeight = 20;
		}
		else
		{
			m_dPrivateScale = 1.0;
		}

		m_currentThumbRect = wxRect(0, m_nCurrentPos / m_dPrivateScale, size.x, nThumbHeight);
	}

	Refresh();
	Update();
}

void CustomRTCScrollbar::DoScroll(int pos)
{
	int max = GetSize().y - m_currentThumbRect.GetHeight();
	if ( pos < 0 )
		m_nCurrentPos = 0;
	else if ( pos > max )
		m_nCurrentPos = max;
	else
		m_nCurrentPos = pos;

	m_currentThumbRect.SetY(m_nCurrentPos);
	m_rtc->Freeze();
	m_rtc->Scroll(0, ((double)m_nCurrentPos / m_dRatio) / m_rtc->GetLineHeight());
	m_rtc->Thaw();
	m_rtc->Update();

	Refresh();
	Update();
}

void CustomRTCScrollbar::OnTargetScroll(wxScrollWinEvent& event)
{
	RecalculateSelf();
	m_rtc->Update();
	event.Skip();
}

void CustomRTCScrollbar::OnTargetSize(wxSizeEvent& event)
{
	RecalculateSelf();
	event.Skip();
}

void CustomRTCScrollbar::OnSetOffOnHold(wxTimerEvent& event)
{
	m_scrollOnHold.Start(100);
}

void CustomRTCScrollbar::OnScrollOnHold(wxTimerEvent& event)
{
	wxMouseEvent mouseEvent;
	mouseEvent.SetPosition(ScreenToClient(wxGetMousePosition()));

	if ( !m_currentThumbRect.Contains(mouseEvent.GetPosition()) )
		OnLeftDown(mouseEvent);
}

void CustomRTCScrollbar::OnSize(wxSizeEvent& event)
{
	RecalculateSelf();
	event.Skip();
}

void CustomRTCScrollbar::OnPaint(wxPaintEvent& event)
{
	wxAutoBufferedPaintDC dc(this);
	wxGCDC gdc(dc);
	wxSize size = GetSize();

	gdc.SetPen(*wxTRANSPARENT_PEN);
	gdc.SetBrush(wxBrush(GetBackgroundColour()));
	gdc.DrawRectangle(wxPoint(0, 0), size);

	if ( m_nBarWidth > 0 )
	{
		gdc.SetPen(wxPen(wxColour(255, 255, 255, 50), m_nBarWidth));
		int x = size.x / 2;
		gdc.DrawLine(wxPoint(x, 0), wxPoint(x, size.y));
	}

	gdc.SetPen(*wxTRANSPARENT_PEN);
	gdc.SetBrush(wxBrush(GetForegroundColour()));
	gdc.DrawRoundedRectangle(m_currentThumbRect, m_dBorderRadius);
}

void CustomRTCScrollbar::OnLeftDown(wxMouseEvent& event)
{
	wxPoint pos = event.GetPosition();
	if ( m_currentThumbRect.Contains(pos) )
	{
		m_bIsDragging = true;
		CaptureMouse();
		m_nDragOffset = pos.y - m_nCurrentPos;
	} else
	{
		int toScroll = m_nCurrentPos;

		if ( m_orientation == wxVERTICAL )
		{
			if ( pos.y > m_nCurrentPos )
				toScroll += m_currentThumbRect.GetHeight() - 20;
			else
				toScroll -= m_currentThumbRect.GetHeight() + 20;
		} else
		{
			if ( pos.x > m_nCurrentPos )
				toScroll += m_currentThumbRect.GetWidth() - 20;
			else
				toScroll -= m_currentThumbRect.GetWidth() + 20;
		}

		DoScroll(toScroll);
		m_setOffOnHold.Start(400, true);
	}
}

void CustomRTCScrollbar::OnLeftUp(wxMouseEvent& event)
{
	if ( HasCapture() )
		ReleaseMouse();

	m_bIsDragging = false;
	m_setOffOnHold.Stop();
	m_scrollOnHold.Stop();
	RecalculateSelf();
}

void CustomRTCScrollbar::OnMouseMove(wxMouseEvent& event)
{
	if ( m_bIsDragging )
	{
		if ( m_orientation == wxVERTICAL )
			DoScroll(event.GetY() - m_nDragOffset);
		else
			DoScroll(event.GetX() - m_nDragOffset);
	}
}

void CustomRTCScrollbar::OnLeaveWindow(wxMouseEvent& event)
{
	m_setOffOnHold.Stop();
	m_scrollOnHold.Stop();
}

void CustomRTCScrollbar::OnMouseCaptureLost(wxMouseCaptureLostEvent& event)
{
	m_bIsDragging = false;
	m_scrollOnHold.Stop();
	RecalculateSelf();
}