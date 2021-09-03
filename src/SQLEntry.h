#ifndef DOCUMENT_H_
#define DOCUMENT_H_
#pragma once

#include <wx\wx.h>
#include <wx\memory.h>

#include <unordered_map>

namespace btfl
{
	struct SQLEntry
	{
		SQLEntry(const wxString& tableName) : tableName(tableName) {}

		wxString tableName{ "" };

		std::unordered_map<wxString, int> integers{};
		std::unordered_map<wxString, wxString> strings{};
	};
}

#endif