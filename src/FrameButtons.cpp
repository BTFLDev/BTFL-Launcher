#include "FrameButtons.h"

#include "wxmemdbg.h"

XS_IMPLEMENT_CLONABLE_CLASS(FrameButtons, wxSFGridShape)

void FrameButtons::Init()
{
	AcceptChild("All");
	RemoveStyle(sfsHIGHLIGHTING);
	RemoveStyle(sfsPOSITION_CHANGE);
	RemoveStyle(sfsHOVERING);
	RemoveStyle(sfsSHOW_HANDLES);
	SetDimensions(1, 3);
	SetCellSpace(2);

	SetFill(*wxTRANSPARENT_BRUSH);
	SetBorder(wxPen(wxColour(255, 255, 255, 80), 1));

	wxSFBitmapShape* helpButton = (wxSFBitmapShape*)GetParentCanvas()->GetDiagramManager()->AddShape(CLASSINFO(wxSFBitmapShape), false);
	helpButton->CreateFromFile("Assets/Icon/Help.png", wxBITMAP_TYPE_PNG);
	helpButton->SetId(BUTTON_Help);
	helpButton->SetStyle(sfsHOVERING | sfsEMIT_EVENTS | sfsALWAYS_INSIDE);

	wxSFBitmapShape* minimizeButton = (wxSFBitmapShape*)GetParentCanvas()->GetDiagramManager()->AddShape(CLASSINFO(wxSFBitmapShape), false);
	minimizeButton->CreateFromFile("Assets/Icon/Minimize.png", wxBITMAP_TYPE_PNG);
	minimizeButton->SetId(BUTTON_Minimize);
	minimizeButton->SetStyle(sfsHOVERING | sfsEMIT_EVENTS | sfsALWAYS_INSIDE);

	wxSFBitmapShape* closeButton = (wxSFBitmapShape*)GetParentCanvas()->GetDiagramManager()->AddShape(CLASSINFO(wxSFBitmapShape), false);
	closeButton->CreateFromFile("Assets/Icon/Close.png", wxBITMAP_TYPE_PNG);
	closeButton->SetId(BUTTON_Close);
	closeButton->SetStyle(sfsHOVERING | sfsEMIT_EVENTS | sfsALWAYS_INSIDE);

	AppendToGrid(helpButton);
	AppendToGrid(minimizeButton);
	AppendToGrid(closeButton);

	Update();
}