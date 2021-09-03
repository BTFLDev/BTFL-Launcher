#ifndef MYAPP_H_
#define MYAPP_H_
#pragma once

#include <wx\wx.h>

class MainFrame;

class MyApp : public wxApp
{
public:
	virtual bool OnInit() override;
	virtual int OnExit() override;
};

wxDECLARE_APP(MyApp);
#endif