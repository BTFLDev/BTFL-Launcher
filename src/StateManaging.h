#ifndef STATEMANAGING_H_
#define STATEMANAGING_H_
#pragma once

#include "SQLEntry.h"

#include <wx/wxsqlite3.h>
#include <wx/filename.h>
#include <wx/wxxmlserializer/XmlSerializer.h>

#include "IsoChecking.h"
#include "Utils.h"

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
		STATE_UpdatingGame,

		//These were added late to the party he
		STATE_VerificationFailed,
	};

	enum LauncherEssenceState
	{
		STATE_NoneEssence = 666,
		STATE_ToInstallGameEssence,
		STATE_InstallingGameEssence,
		STATE_ToPlayGameEssence,
		STATE_ToUpdateGameEssence,
		STATE_UpdatingGameEssence
	};

	struct Settings: public xsSerializable
	{
		bool bLookForUpdates = true;
		bool bCloseSelfOnGameLaunch = true;

		Settings()
		{
			XS_SERIALIZE(bLookForUpdates, "look_for_updates");
			XS_SERIALIZE(bCloseSelfOnGameLaunch, "close_on_game_launch");
		}
	};

	void Init();
	void ShutDown();
	bool ShouldShutdown();
	void LoadLauncher(btfl::SQLDatabase* database);
	void SaveSettings(btfl::Settings& settings);

	void SetState(LauncherState state);
	LauncherState GetState();
	void RestoreLastState();

	void SetEssenceState(LauncherEssenceState state);
	LauncherEssenceState GetEssenceState();
	void RestoreLastEssenceState();

	bool IsUserAnEssence();

	bool UninstallGame(bool showMessages = true);
	bool UninstallEssenceGame(bool showMessages = true);

	void SetIsoRegion(iso::ISO_Region region);
	iso::ISO_Region GetIsoRegion();

	btfl::Settings GetSettings();

	wxString GetInstalledGameVersion();
	wxString GetInstalledLauncherVersion();
	wxString GetInstalledEssenceGameVersion();

	void SetLatestGameVersion(const wxString& version);
	wxString GetLatestGameVersion();

	void SetLatestEssenceGameVersion(const wxString& version);
	wxString GetLatestEssenceGameVersion();

	void SetEncryptedEssencePassword(const wxString& password);
	wxString GetEncryptedEssencePassword();
	void SetUserEssencePassword(const wxString& password);
	wxString GetUserEssencePassword();

	bool ShouldAutoUpdateGame();

	inline wxString GetStorageURL() {
		return utils::crypto::GetDecryptedString("iwur;20uby/jjwiwcxthsepquhov/fpp0CndehvuL8380DUIM0Gkmht2nctwfu0"); 
	}

	void DoUpdateLauncher();
	void DeleteInstallerIfPresent();

	void UpdateDatabase(const btfl::SQLEntry& sqlEntry);

	const wxFileName& GetIsoFileName();
	void SetIsoFileName(const wxFileName& fileName);

	const wxFileName& GetInstallFileName();
	void SetInstallPath(const wxString& installPath);

	const wxFileName& GetEssenceInstallFileName();
	void SetEssenceInstallPath(const wxString& installPath);

	bool HasUserAgreedToDisclaimer();
	void AgreeToDisclaimer();
}

#endif