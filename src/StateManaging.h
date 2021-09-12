#ifndef STATEMANAGING_H_
#define STATEMANAGING_H_
#pragma once

#include "SQLEntry.h"

#include <wx/wxsqlite3.h>
#include <wx/filename.h>
#include <wx/wxxmlserializer/XmlSerializer.h>

namespace btfl
{
	class SQLDatabase : public wxSQLite3Database
	{
	public:
		SQLDatabase() = default;
		SQLDatabase(wxFileName& path);

		bool Init();
		void CreateAllTables();

		int GetSQLEntryId(const SQLEntry& sqlEntry);

		bool CreateTable(const wxString& tableName, const wxArrayString& arguments, bool ifNotExists = true);

		bool InsertSQLEntry(const SQLEntry& sqlEntry);
		bool UpdateSQLEntry(const SQLEntry& sqlEntry);

		wxSQLite3Statement ConstructInsertStatement(const SQLEntry& sqlEntry);
		wxSQLite3Statement ConstructUpdateStatement(const SQLEntry& sqlEntry, int id);
	};


	////////////////////////////////////////////////////////////////
	/////////////////////// Separate Functions /////////////////////
	////////////////////////////////////////////////////////////////


	enum LauncherState
	{
		STATE_Initial,

		STATE_ToSelectIso,
		STATE_ToVerifyIso,
		STATE_VerifyingIso,
		STATE_ToInstallGame,
		STATE_InstallingGame,
		STATE_ToPlayGame,
		STATE_LaunchingGame,
		STATE_ToUpdateGame,
		STATE_UpdatingGame
	};

	struct Settings: public xsSerializable
	{
		bool bLookForUpdates = true;

		Settings()
		{
			XS_SERIALIZE_BOOL(bLookForUpdates, "look_for_updates");
		}
	};

	void Init();
	void ShutDown();
	void LoadLauncher(btfl::SQLDatabase* database);
	void SaveSettings(btfl::Settings& settings);

	void SetState(LauncherState state);

	inline wxString GetStorageURL() { return "https://btflgame.com/"; }

	void UpdateDatabase(const btfl::SQLEntry& sqlEntry);

	const wxFileName& GetIsoFileName();
	void SetIsoFileName(const wxFileName& fileName);

	const wxFileName& GetInstallFileName();
	void SetInstallPath(const wxString& installPath);

	bool HasUserAgreedToDisclaimer();
	void AgreeToDisclaimer();
}

#endif