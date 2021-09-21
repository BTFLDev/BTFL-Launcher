#include "TransparentButton.h"

#include "wxmemdbg.h"

XS_IMPLEMENT_CLONABLE_CLASS(TransparentButton, wxSFRoundRectShape);

TransparentButton::TransparentButton(const wxString& label, const wxRealPoint& pos, const wxRealPoint& size,
	double radius, wxSFDiagramManager* manager) :
	wxSFRoundRectShape(pos, size, radius, manager)
{
	SetFill(wxBrush(wxColour(255, 255, 255, 50)));
	SetBorder(*wxTRANSPARENT_PEN);

	AddStyle(sfsEMIT_EVENTS);

	RemoveStyle(sfsHIGHLIGHTING);
	RemoveStyle(sfsPOSITION_CHANGE);
	RemoveStyle(sfsSIZE_CHANGE);
	RemoveStyle(sfsSHOW_HANDLES);

	m_label = label;
}

TransparentButton::TransparentButton(const TransparentButton& other)
{

}

TransparentButton::~TransparentButton()
{
	if ( m_bitmap )
		delete m_bitmap;
}

void TransparentButton::Enable(bool enable)
{
	if ( enable )
	{
		AddStyle(sfsEMIT_EVENTS);
		AddStyle(sfsHOVERING);
	} else
	{
		RemoveStyle(sfsEMIT_EVENTS);
		RemoveStyle(sfsHOVERING);
	}

	m_isEnabled = enable;
}

void TransparentButton::SetLabel(const wxString& label)
{
	m_label = label;
	RecalculateSelf();
}

void TransparentButton::SetFont(const wxFont& font)
{
	m_font = font;
	RecalculateSelf();
}

void TransparentButton::SetBitmap(const wxBitmap& bmp)
{
	if ( !bmp.IsOk() )
	{
		if ( m_bitmap )
		{
			delete m_bitmap;
			m_bitmap = nullptr;
		}

		return;
	}

	if ( !m_bitmap )
		m_bitmap = new wxBitmap(bmp);
	else
		*m_bitmap = bmp;

	m_bmpRatio = (double)bmp.GetWidth() / bmp.GetHeight();
}

void TransparentButton::SetPadding(int x, int y)
{
	m_xPadding = x;
	m_yPadding = y;
}

void TransparentButton::GetPadding(int* x, int* y)
{
	if ( x ) *x = m_xPadding;
	if ( y ) *y = m_yPadding;
}

void TransparentButton::RecalculateSelf(const wxSize& soloBmpSize)
{
	wxSFShapeCanvas* pCanvas = GetParentCanvas();
	if ( !pCanvas )
		return;

	wxClientDC dc(pCanvas);
	dc.SetFont(m_font);
	wxSize size = dc.GetMultiLineTextExtent(m_label);
	size.y -= 5;
	m_xLabel = m_xPadding;

	if ( m_bitmap )
	{
		if ( m_label.IsEmpty() )
		{
			if ( soloBmpSize != wxDefaultSize )
			{
				double curRatio = (double)soloBmpSize.x / soloBmpSize.y;
				if ( curRatio > m_bmpRatio )
				{
					m_bmpScale = (double)soloBmpSize.y / m_bitmap->GetHeight();
					m_xBitmap = (soloBmpSize.x / 2) - ((double)(m_bitmap->GetWidth() * m_bmpScale) / 2) + m_xPadding;
					m_yBitmap = m_yPadding;
				} else
				{
					m_bmpScale = (double)soloBmpSize.x / m_bitmap->GetWidth();
					m_yBitmap = (soloBmpSize.y / 2) - (((double)m_bitmap->GetHeight() * m_bmpScale) / 2) + m_yPadding;
					m_xBitmap = m_xPadding;
				}

				size.x = (double)m_bitmap->GetWidth() * m_bmpScale;
				size.y = (double)m_bitmap->GetHeight() * m_bmpScale;
			}
		} 
		else
		{
			m_bmpScale = double(size.y - 4) / (m_bitmap->GetHeight());
			m_yBitmap = m_yPadding + 2;
			m_xBitmap = m_xPadding;

			int bmpx = (double)m_bitmap->GetWidth() * m_bmpScale;
			size.x += bmpx + 10;
			m_xLabel += bmpx + 10;
		}
	}

	SetRectSize(size.x + (m_xPadding * 2), size.y + (m_yPadding * 2));
}

void TransparentButton::DrawContent(wxDC& dc, bool isHovering)
{
	dc.SetFont(m_font);
	dc.SetTextForeground(m_textColour);
	wxRealPoint absolutePos = GetAbsolutePosition();

	dc.DrawText(m_label, absolutePos + wxRealPoint(m_xLabel, m_yPadding));

	if ( m_bitmap )
	{
		dc.SetUserScale(m_bmpScale, m_bmpScale);
		wxRealPoint bmpPos = (absolutePos + wxRealPoint(m_xBitmap, m_yBitmap));
		bmpPos.x /= m_bmpScale;
		bmpPos.y /= m_bmpScale;

		if ( isHovering )
			dc.DrawBitmap(m_bitmap->ConvertToDisabled(0), bmpPos, true);
		else
			dc.DrawBitmap(*m_bitmap, bmpPos, true);

		dc.SetUserScale(1.0, 1.0);
	}
}

void TransparentButton::DrawNormal(wxDC& dc)
{
	wxSFRoundRectShape::DrawNormal(dc);
	DrawContent(dc, false);
}

void TransparentButton::DrawHover(wxDC& dc)
{
	wxBrush fillCache = GetFill();
	wxColour textColourCache = m_textColour;

	SetFill(wxBrush(wxColour(230, 230, 230)));
	m_textColour = { 0, 0, 0 };

	wxSFRoundRectShape::DrawHover(dc);
	DrawContent(dc, true);

	SetFill(fillCache);
	m_textColour = textColourCache;
}

