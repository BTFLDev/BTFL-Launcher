#ifndef TRANSPARENTBUTTON_H_
#define TRANSPARENTBUTTON_H_
#pragma once

#include <wx\wxsf\wxShapeFramework.h>

class TransparentButton : public wxSFRoundRectShape
{
protected:
	wxString m_label;
	wxFont m_font;
	wxColour m_textColour{ 255,255,255 };

	int m_xPadding = 35, m_yPadding = 12;
	int m_xLabel = 30;

	wxBitmap* m_bitmap = nullptr;
	double m_bmpScale = 1.0, m_bmpRatio = 1.0;
	int m_xBitmap = 30, m_yBitmap;

	bool m_isEnabled = true;

public:
	XS_DECLARE_CLONABLE_CLASS(TransparentButton);
	TransparentButton() = default;
	TransparentButton(const wxString& label, const wxRealPoint& pos, const wxRealPoint& size,
		double radius, wxSFDiagramManager* manager);
	TransparentButton(const TransparentButton& other);

	virtual ~TransparentButton();

	void Enable(bool enable);

	void SetLabel(const wxString& label);
	void SetFont(const wxFont& font);
	void SetBitmap(const wxBitmap& bmp);

	inline void SetTextColour(const wxColour& colour) { m_textColour = colour; }

	void SetPadding(int x, int y);
	void GetPadding(int* x, int* y);
	void RecalculateSelf(const wxSize& soloBmpSize = wxDefaultSize);

	void DrawContent(wxDC& dc, bool isHovering);

	virtual void DrawNormal(wxDC& dc);
	virtual void DrawHover(wxDC& dc);

	inline virtual void OnMouseEnter(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_CLOSED_HAND); }
	inline virtual void OnMouseLeave(const wxPoint& pos) override { GetParentCanvas()->SetCursor(wxCURSOR_DEFAULT); }
};

#endif