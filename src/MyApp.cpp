#include "MyApp.h"
#include "StateManaging.h"

#include <wx/font.h>

#include "wxmemdbg.h"

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{	
	wxFont::AddPrivateFont("Assets/Fonts/Lora-Regular.ttf");

	wxImage::AddHandler(new wxPNGHandler);
	wxImage::AddHandler(new wxICOHandler);

	btfl::Init();
	return true;
}

int MyApp::OnExit()
{
	btfl::ShutDown();
	return wxApp::OnExit();
}