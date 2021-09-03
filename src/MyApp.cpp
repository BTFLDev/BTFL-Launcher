#include "MyApp.h"
#include "StateManaging.h"

#include "wxmemdbg.h"

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
	wxInitAllImageHandlers();
	btfl::Init();
	return true;
}

int MyApp::OnExit()
{
	btfl::ShutDown();
	return wxApp::OnExit();
}