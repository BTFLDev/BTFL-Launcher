#include "StateManaging.h"
#include "MainFrame.h"
#include "IsoHashes.h"
#include "MyApp.h"

#include <wx/xml/xml.h>
#include <wx/sstream.h>
#include <wx/filesys.h>
#include <wx/fs_inet.h>
#include <wx/msgdlg.h>
#include <wx/mimetype.h>
#include <wx/webrequest.h>
#include <wx/progdlg.h>

#include <thread>

#include "IsoChecking.h"
#include "wxmemdbg.h"

btfl::SQLDatabase::SQLDatabase(wxFileName& path)
{
	EnableForeignKeySupport(true);
	if ( path.IsOk() && !path.IsDir() )
	{
		Open(path.GetFullPath());
	}
}

bool btfl::SQLDatabase::Init()
{
	if ( !TableExists("user_data") )
	{
		CreateAllTables();
		return false;
	}

	return true;
}

void btfl::SQLDatabase::CreateAllTables()
{
	wxArrayString tUserData;
	tUserData.Add("user_state INTEGER");
	tUserData.Add("user_essence_state INTEGER");
	tUserData.Add("has_agreed_to_disclaimer INTEGER");
	tUserData.Add("essence_password");
	tUserData.Add("iso_file_path TEXT");
	tUserData.Add("iso_region INTEGER");

	wxArrayString tLauncherData;
	tLauncherData.Add("install_path TEXT");
	tLauncherData.Add("essence_install_path TEXT");
	tLauncherData.Add("installed_game_version TEXT");
	tLauncherData.Add("installed_essence_game_version TEXT");
	tLauncherData.Add("settings_xml TEXT");

	try
	{
		Begin();

		CreateTable("user_data", tUserData);
		CreateTable("launcher_data", tLauncherData);

		Commit();
	}
	catch ( wxSQLite3Exception& e )
	{
		wxMessageBox(e.GetMessage());
	}
}

bool btfl::SQLDatabase::CreateTable(const wxString& tableName, const wxArrayString& arguments,
	bool ifNotExists)
{
	if ( arguments.IsEmpty() )
	{
		wxMessageBox("You can't create a table with no arguments!");
		return false;
	}

	wxString update("CREATE TABLE ");
	if ( ifNotExists )
		update << "IF NOT EXISTS ";

	update << tableName;
	update << " (";

	bool first = true;

	for ( auto& str : arguments )
	{
		if ( !first )
			update << ", ";
		else
			first = false;

		update << str;
	}

	update << ");";
	ExecuteUpdate(update);

	return true;
}

bool btfl::SQLDatabase::InsertSQLEntry(const btfl::SQLEntry& sqlEntry)
{
	wxSQLite3Statement statement = ConstructInsertStatement(sqlEntry);
	statement.ExecuteUpdate();
	return true;
}

bool btfl::SQLDatabase::UpdateSQLEntry(const btfl::SQLEntry& sqlEntry)
{
	try
	{
		wxSQLite3Statement statement = ConstructUpdateStatement(sqlEntry, 1);
		statement.ExecuteUpdate();
	}
	catch ( wxSQLite3Exception& e )
	{
		wxMessageBox("Exception thrown - " + e.GetMessage());
	}

	return true;
}

wxSQLite3Statement btfl::SQLDatabase::ConstructInsertStatement(const btfl::SQLEntry& sqlEntry)
{
	wxString insert("INSERT INTO ");
	insert << sqlEntry.tableName << " (";

	wxString columnNames;
	wxString valueNames;

	bool first = true;

	wxSQLite3StatementBuffer buffer;

	for ( const std::pair<const wxString, int>& it : sqlEntry.integers )
	{
		if ( !first )
		{
			columnNames << ", ";
			valueNames << ", ";
		}
		else
			first = false;

		columnNames << it.first;
		valueNames << it.second;
	}

	for ( const std::pair<const wxString, wxString>& it : sqlEntry.strings )
	{
		if ( !first )
		{
			columnNames << ", ";
			valueNames << ", ";
		}
		else
			first = false;

		columnNames << it.first;
		valueNames << "'" << buffer.Format("%q", (const char*)it.second) << "'";
	}

	insert << columnNames << ") VALUES (" << valueNames << ");";

	try
	{
		wxSQLite3Statement statement = PrepareStatement(insert);
		return statement;
	}
	catch ( wxSQLite3Exception& e )
	{
		wxMessageBox(e.GetMessage());
	}

	return wxSQLite3Statement();
}

wxSQLite3Statement btfl::SQLDatabase::ConstructUpdateStatement(const btfl::SQLEntry& sqlEntry, int id)
{
	wxString update("UPDATE ");
	update << sqlEntry.tableName << " SET ";

	bool first = true;
	wxSQLite3StatementBuffer buffer;

	for ( const std::pair<const wxString, int>& it : sqlEntry.integers )
	{
		if ( !first )
			update << ", ";
		else
			first = false;

		update << it.first << " = " << it.second;
	}

	for ( const std::pair<const wxString, wxString>& it : sqlEntry.strings )
	{
		if ( !first )
			update << ", ";
		else
			first = false;

		update << it.first << " = '" << buffer.Format("%q", (const char*)it.second) << "'";
	}

	update << " WHERE rowid = " << id << ";";

	try
	{
		return PrepareStatement(update);
	}
	catch ( wxSQLite3Exception& e )
	{
		wxMessageBox(e.GetMessage());
	}

	return wxSQLite3Statement();
}


////////////////////////////////////////////////////////////////
/////////////////////// Separate Functions /////////////////////
////////////////////////////////////////////////////////////////

btfl::SQLDatabase* pDatabase;


bool ShouldSaveState(btfl::LauncherState state)
{
	if ( state == btfl::STATE_VerifyingIso || state == btfl::STATE_InstallingGame || state == btfl::STATE_UpdatingGame )
		return false;

	return true;
}

bool ShouldSaveEssenceState(btfl::LauncherEssenceState state)
{
	return state != btfl::STATE_InstallingGameEssence && state != btfl::STATE_UpdatingGameEssence;
}

void CleanUpOldBuild()
{
	wxArrayString paths;
	paths.Add("Assets/gh.png");
	paths.Add("Assets/Launch Graphic TEMP.png");
	paths.Add("Assets/Launch Graphic TEMP@2x.png");
	paths.Add("Assets/PhoenixArenaPablo.png");
	paths.Add("Assets/Background/PhoenixArenaPablo.png");
	paths.Add("Assets/RocArenaPablo.png");
	paths.Add("Assets/PrimaryButton.png");
	paths.Add("Assets/Background/Main Page Roc@2x.png");
	paths.Add("Assets/Background/Subpage@2x.png");
	paths.Add("Assets/Containers/Back.png");
	paths.Add("Assets/Containers/Back@2x.png");
	paths.Add("Assets/Containers/FilePath.png");
	paths.Add("Assets/Containers/Primary Button.png");
	paths.Add("Assets/Containers/Primary Button@2x.png");
	paths.Add("Assets/Containers/Windows Controls.png");
	paths.Add("Assets/Containers/Windows Controls@2x.png");
	paths.Add("Assets/Fonts/Lora-Bold.ttf");
	paths.Add("Assets/Fonts/Lora-BoldItalic.ttf");
	paths.Add("Assets/Fonts/Lora-Italic.ttf");
	paths.Add("Assets/Fonts/Lora-Medium.ttf");
	paths.Add("Assets/Fonts/Lora-MediumItalic.ttf");
	paths.Add("Assets/Fonts/Lora-SemiBold.ttf");
	paths.Add("Assets/Fonts/Lora-SemiBoldItalic.ttf");
	paths.Add("Assets/Icon/Arrow Left@2x.png");
	paths.Add("Assets/Icon/Browse.png");
	paths.Add("Assets/Icon/Check.png");
	paths.Add("Assets/Icon/Close@2x.png");
	paths.Add("Assets/Icon/Discord.png");
	paths.Add("Assets/Icon/Download.png");
	paths.Add("Assets/Icon/Facebook.png");
	paths.Add("Assets/Icon/Failed.png");
	paths.Add("Assets/Icon/Folder.png");
	paths.Add("Assets/Icon/Help@2x.png");
	paths.Add("Assets/Icon/Minimize@2x.png");
	paths.Add("Assets/Icon/Reddit.png");
	paths.Add("Assets/Icon/Settigs Outlined (Small).png");
	paths.Add("Assets/Icon/Settings.png");
	paths.Add("Assets/Icon/Twitter.png");
	paths.Add("Assets/Icon/Uninstall (Small).png");
	paths.Add("Assets/Icon/Update.png");
	paths.Add("Assets/Icon/Verify (Small).png");
	paths.Add("Assets/Icon/Verify.png");
	paths.Add("Assets/Icon/Website.png");
	paths.Add("Assets/Icon/YouTube.png");
	
	for ( wxString& str : paths )
	{
		if ( wxFileName::Exists(str) )
		{
			wxFileName fileName(str);
			fileName.SetPermissions(wxS_DEFAULT);
			wxRemoveFile(str);
		}
	}

	try
	{
		!pDatabase->ExecuteScalar("SELECT EXISTS(SELECT essence_password FROM user_data WHERE rowid = 1)");
	}
	catch ( wxSQLite3Exception e )
	{
		pDatabase->ExecuteUpdate("ALTER TABLE launcher_data ADD COLUMN essence_install_path TEXT;");
		pDatabase->ExecuteUpdate("ALTER TABLE launcher_data ADD COLUMN installed_essence_game_version TEXT;");
		pDatabase->ExecuteUpdate("ALTER TABLE user_data ADD COLUMN user_essence_state INTEGER;");
		pDatabase->ExecuteUpdate("ALTER TABLE user_data ADD COLUMN essence_password INTEGER;");

		btfl::SetEssenceInstallPath(wxGetCwd() + "/");
	}
}

class InstallerDownloader;

btfl::LauncherState currentState = btfl::LauncherState::STATE_Initial;
btfl::LauncherState lastState = btfl::LauncherState::STATE_Initial;
btfl::LauncherEssenceState currentEssenceState = btfl::LauncherEssenceState::STATE_NoneEssence;
btfl::LauncherEssenceState lastEssenceState = btfl::LauncherEssenceState::STATE_NoneEssence;
wxFileName isoFileName;
bool bHasUserAgreedToDisclaimer = false;
iso::ISO_Region isoRegion = iso::ISO_Region::ISO_Invalid;

wxFileName installFileName;
wxFileName essenceInstallFileName;

wxString sInstalledGameVersion;
wxString sInstalledEssenceGameVersion;
wxString sInstalledLauncherVersion = "v1.0.15";
wxString sLatestGameVersion;
wxString sLatestEssenceGameVersion;

wxString sEncryptedEssencePassword;
wxString sUserEssencePassword;

MainFrame* pMainFrame;

InstallerDownloader* installerDownloader;

class InstallerDownloader : public wxEvtHandler
{
	wxWebRequest m_installerRequest;
	wxProgressDialog* m_pProgressDialog = nullptr;

public:
	InstallerDownloader()
	{
		Bind(wxEVT_WEBREQUEST_STATE, &InstallerDownloader::OnWebRequestState, this);
	}

	void Start()
	{
		m_installerRequest = wxWebSession::GetDefault().CreateRequest(
			this, btfl::GetStorageURL() + "launcher/installer.exe", 123321
		);
		if ( !m_installerRequest.IsOk() )
			return;

		wxWebSession::GetDefault().SetTempDir(wxGetCwd());
		m_installerRequest.SetStorage(wxWebRequest::Storage_File);
		m_installerRequest.SetHeader(
			utils::crypto::GetDecryptedString("Bxujpuj}bvjro"),
			utils::crypto::GetDecryptedString("urlgo#hkqaofv[mKr7ijR|GhH\\Spm48ygpzY4nKT4m6MhN"));
		m_installerRequest.Start();

		StartProgress();
		pMainFrame->Hide();
	}

	void OnWebRequestState(wxWebRequestEvent& event)
	{
		switch ( event.GetState() )
		{
		case wxWebRequest::State_Completed:
		{
			EndProgress();
			wxFileName fileName(event.GetDataFile());
			fileName.SetName("Temp-Installer");
			fileName.SetExt("exe");

			bool success = false;
			if ( wxRenameFile(event.GetDataFile(), fileName.GetFullPath()) )
			{
				wxFileType* pFileType = wxTheMimeTypesManager->GetFileTypeFromExtension("exe");
				wxString sCommand = pFileType->GetOpenCommand(fileName.GetFullName());
				delete pFileType;

				if ( wxExecute(sCommand) != -1 )
				{
					pMainFrame->Close();
					success = true;
				}
			}

			if ( success )
				break;
		}
		case wxWebRequest::State_Unauthorized:
		case wxWebRequest::State_Failed:
		case wxWebRequest::State_Cancelled:
			EndProgress();
			pMainFrame->Show();
			wxMessageBox("There was a problem when updating the application. Please try again later");
			break;
		}
	}

private:
	void StartProgress()
	{
		m_pProgressDialog = new wxProgressDialog("Downloading update...", "This may take a few seconds...");
		m_pProgressDialog->SetIcon(wxICON(aaaaBTFLIconNoText));
		m_pProgressDialog->Pulse();
	}
	
	void EndProgress() 
	{
		m_pProgressDialog->Hide();
		m_pProgressDialog->Destroy();
	}
};

void btfl::Init()
{
	wxBitmap shape("Assets/FrameShape.png", wxBITMAP_TYPE_PNG);

	pMainFrame = new MainFrame(nullptr, -1, "Beyond The Forbidden Lands - Launcher",
		wxDefaultPosition, shape.GetSize(), wxFRAME_SHAPED | wxBORDER_NONE);
	pMainFrame->Hide();
	pMainFrame->SetShape(wxRegion(shape, wxColour(255, 255, 255)));
	pMainFrame->CenterOnScreen();
	pMainFrame->Layout();
	pMainFrame->Update();

	wxSQLite3Database::InitializeSQLite();
	pDatabase = new btfl::SQLDatabase();
	pDatabase->Open("./LauncherData.db", utils::crypto::GetDecryptedString(db_encryption_key));
	
	if ( !pDatabase->Init() )
	{
		btfl::SQLEntry userEntry("user_data");
		userEntry.integers["user_state"] = currentState;
		userEntry.integers["user_essence_state"] = currentEssenceState;
		userEntry.integers["has_agreed_to_disclaimer"] = false;
		userEntry.integers["iso_region"] = iso::ISO_Region::ISO_Invalid;
		userEntry.strings["iso_file_path"] = isoFileName.GetFullPath();

		btfl::SQLEntry launcherEntry("launcher_data");
		launcherEntry.strings["install_path"] = installFileName.GetFullPath();
		launcherEntry.strings["essence_install_path"] = essenceInstallFileName.GetFullPath();
		launcherEntry.strings["installed_game_version"] = sInstalledGameVersion;
		launcherEntry.strings["installed_essence_game_version"] = sInstalledEssenceGameVersion;
		launcherEntry.strings["settings_xml"] = "";

		pDatabase->InsertSQLEntry(userEntry);
		pDatabase->InsertSQLEntry(launcherEntry);

		SetInstallPath(wxGetCwd() + "/");
		SetEssenceInstallPath(wxGetCwd() + "/");

		btfl::SetState(btfl::LauncherState::STATE_ToSelectIso);
	}
	else
	{
		CleanUpOldBuild();
		btfl::LoadLauncher(pDatabase);
	}

	pMainFrame->Show();

	installerDownloader = new InstallerDownloader;

	pMainFrame->GetMainPanel()->DoLookForLauncherUpdates();
	pMainFrame->GetMainPanel()->DoLookForGameUpdates();
	pMainFrame->GetMainPanel()->DoLookForEssenceGameUpdates();
	pMainFrame->GetMainPanel()->DoCheckEssencePassword();

	btfl::DeleteInstallerIfPresent();

	if ( currentState == btfl::STATE_ToPlayGame && !wxFileName::Exists(installFileName.GetFullPath() + "BTFL.exe") )
	{
		btfl::SetState(btfl::STATE_ToInstallGame);
	}

	if ( currentEssenceState == btfl::STATE_ToPlayGameEssence && !wxFileName::Exists(essenceInstallFileName.GetFullPath() + "BTFL_Preview.exe") )
	{
		btfl::SetEssenceState(btfl::STATE_ToInstallGameEssence);
	}
}

void btfl::ShutDown()
{
	pDatabase->Close();
	wxSQLite3Database::ShutdownSQLite();
	delete pDatabase;
	delete installerDownloader;
	pDatabase = nullptr;
	installerDownloader = nullptr;
}

bool btfl::ShouldShutdown()
{
	return ShouldSaveState(currentState);
}

void btfl::SetState(btfl::LauncherState state)
{
	if ( state == currentState )
		return;

	lastState = currentState;

	currentState = state;
	pMainFrame->SetState(state);

	if ( ShouldSaveState(state) )
	{
		btfl::SQLEntry sqlEntry("user_data");
		sqlEntry.integers["user_state"] = currentState;

		UpdateDatabase(sqlEntry);
	}
}

btfl::LauncherState btfl::GetState()
{
	return currentState;
}

void btfl::RestoreLastState()
{
	btfl::SetState(lastState);
}

void btfl::SetEssenceState(btfl::LauncherEssenceState state)
{
	if ( state == currentEssenceState )
		return;

	lastEssenceState = currentEssenceState;

	currentEssenceState = state;
	pMainFrame->SetEssenceState(state);

	if ( ShouldSaveEssenceState(state) )
	{
		btfl::SQLEntry sqlEntry("user_data");
		sqlEntry.integers["user_essence_state"] = currentEssenceState;

		UpdateDatabase(sqlEntry);
	}
}

btfl::LauncherEssenceState btfl::GetEssenceState()
{
	return currentEssenceState;
}

void btfl::RestoreLastEssenceState()
{
	btfl::SetEssenceState(lastEssenceState);
}

bool btfl::IsUserAnEssence()
{
	return currentEssenceState != btfl::STATE_NoneEssence;
}

bool btfl::UninstallGame(bool showMessages)
{
	if ( !wxFileName::Exists(installFileName.GetFullPath()) )
	{
		if ( showMessages )
			wxMessageBox("The game isn't installed or has been moved. Uninstallation unsuccessful.");
		return false;
	}

	if ( wxFileName::Rmdir(installFileName.GetFullPath(), wxPATH_RMDIR_RECURSIVE) )
	{
		if ( showMessages )
			wxMessageBox("The game has been successfully uninstalled.");
		btfl::SetState(btfl::STATE_ToInstallGame);
		return true;
	}
	else
	{
		if ( showMessages )
			wxMessageBox("Something went wrong. Uninstallation unsuccessful");
		return false;
	}
}

bool btfl::UninstallEssenceGame(bool showMessages)
{
	if ( !wxFileName::Exists(essenceInstallFileName.GetFullPath()) )
	{
		if ( showMessages )
			wxMessageBox("The game isn't installed or has been moved. Uninstallation unsuccessful.");
		return false;
	}

	if ( wxFileName::Rmdir(essenceInstallFileName.GetFullPath(), wxPATH_RMDIR_RECURSIVE) )
	{
		if ( showMessages )
			wxMessageBox("The game has been successfully uninstalled.");
		btfl::SetEssenceState(btfl::STATE_ToInstallGameEssence);
		return true;
	}
	else
	{
		if ( showMessages )
			wxMessageBox("Something went wrong. Uninstallation unsuccessful");
		return false;
	}
}

void btfl::SetIsoRegion(iso::ISO_Region region)
{
	isoRegion = region;

	btfl::SQLEntry sqlEntry("user_data");
	sqlEntry.integers["iso_region"] = region;
	
	UpdateDatabase(sqlEntry);
}

iso::ISO_Region btfl::GetIsoRegion()
{
	return isoRegion;
}

btfl::Settings btfl::GetSettings()
{
	return pMainFrame->GetSecondaryPanel()->GetSettings();
}

wxString btfl::GetInstalledGameVersion()
{
	return sInstalledGameVersion;
}

wxString btfl::GetInstalledLauncherVersion()
{
	return sInstalledLauncherVersion;
}

wxString btfl::GetInstalledEssenceGameVersion()
{
	return sInstalledEssenceGameVersion;
}

void btfl::SetLatestGameVersion(const wxString& version)
{
	sLatestGameVersion = version;
}

wxString btfl::GetLatestGameVersion()
{
	return sLatestGameVersion;
}

void btfl::SetLatestEssenceGameVersion(const wxString& version)
{
	sLatestEssenceGameVersion = version;
}

wxString btfl::GetLatestEssenceGameVersion()
{
	return sLatestEssenceGameVersion;
}

void btfl::SetEncryptedEssencePassword(const wxString& password)
{
	sEncryptedEssencePassword = password;
}

wxString btfl::GetEncryptedEssencePassword()
{
	return sEncryptedEssencePassword;
}

void btfl::SetUserEssencePassword(const wxString& password)
{
	sUserEssencePassword = password;

	btfl::SQLEntry sqlEntry("user_data");
	sqlEntry.strings["essence_password"] = sUserEssencePassword;
	btfl::UpdateDatabase(sqlEntry);
}

wxString btfl::GetUserEssencePassword()
{
	return sUserEssencePassword;
}


bool btfl::ShouldAutoUpdateGame()
{
	return btfl::GetSettings().bLookForUpdates;
}

void btfl::DoUpdateLauncher()
{
	installerDownloader->Start();
}

void btfl::DeleteInstallerIfPresent()
{
	wxString sPath(wxGetCwd() + "/Temp-Installer.exe");
	if ( !wxFileName::Exists(sPath) )
		return;

	wxFileName fileName(sPath);
	if ( fileName.SetPermissions(wxS_DEFAULT) )
		wxRemoveFile(sPath);
}

void btfl::LoadLauncher(btfl::SQLDatabase* database)
{
	wxSQLite3ResultSet result = database->ExecuteQuery("SELECT * FROM user_data WHERE rowid = 1;");
	if ( result.NextRow() )
	{
		isoFileName.Assign(result.GetAsString("iso_file_path"));
		isoRegion = (iso::ISO_Region)result.GetInt("iso_region");
		bHasUserAgreedToDisclaimer = result.GetInt("has_agreed_to_disclaimer");
		sUserEssencePassword = result.GetAsString("essence_password");

		SetState((btfl::LauncherState)result.GetInt("user_state"));
		SetEssenceState((btfl::LauncherEssenceState)result.GetInt("user_essence_state"));
	}

	result = database->ExecuteQuery("SELECT * FROM launcher_data WHERE rowid = 1;");
	if ( result.NextRow() )
	{
		installFileName.Assign(result.GetAsString("install_path"));
		essenceInstallFileName.Assign(result.GetAsString("essence_install_path"));
		sInstalledGameVersion = result.GetAsString("installed_game_version");
		sInstalledEssenceGameVersion = result.GetAsString("installed_essence_game_version");

		wxStringInputStream settingsStream(result.GetAsString("settings_xml"));
		if ( settingsStream.GetSize() )
		{
			wxXmlDocument settingsDoc(settingsStream);

			btfl::Settings settings;
			settings.DeserializeObject(settingsDoc.GetRoot()->GetChildren());
			pMainFrame->GetSecondaryPanel()->SetSettings(settings);
		}
	}
}

void btfl::SaveSettings(btfl::Settings& settings)
{
	wxXmlDocument settingsDoc;
	wxXmlNode* pSettingsRootNode = new wxXmlNode(nullptr, wxXML_ELEMENT_NODE, "settings_xml");
	settingsDoc.SetRoot(pSettingsRootNode);
	pSettingsRootNode->AddChild(settings.SerializeObject(nullptr));

	wxStringOutputStream settingsStream;
	settingsDoc.Save(settingsStream);

	btfl::SQLEntry sqlEntry("launcher_data");
	sqlEntry.strings["settings_xml"] = settingsStream.GetString();

	btfl::UpdateDatabase(sqlEntry);
}

void btfl::UpdateDatabase(const btfl::SQLEntry& sqlEntry)
{
	std::thread thread([sqlEntry]()
		{
			pDatabase->UpdateSQLEntry(sqlEntry);
		}
	);
	thread.detach();
}

const wxFileName& btfl::GetIsoFileName() 
{
	return isoFileName;
}

void btfl::SetIsoFileName(const wxFileName& fileName)
{
	isoFileName = fileName;

	btfl::SQLEntry sqlEntry("user_data");
	sqlEntry.strings["iso_file_path"] = fileName.GetFullPath();
	btfl::UpdateDatabase(sqlEntry);
}

const wxFileName& btfl::GetInstallFileName()
{
	return installFileName;
}

void btfl::SetInstallPath(const wxString& installPath)
{
	wxFileName newFileName(installPath);
	newFileName.AppendDir("Beyond The Forbidden Lands");

	if ( currentState == btfl::STATE_ToPlayGame || currentState == btfl::STATE_ToUpdateGame )
	{
		wxBusyCursor busyCursor;
		if ( !wxFileName::Exists(newFileName.GetFullPath()) )
			wxFileName::Mkdir(newFileName.GetFullPath());

		if ( !wxRenameFile(installFileName.GetFullPath(), newFileName.GetFullPath()) )
			return;
	}

	installFileName = newFileName;

	btfl::SQLEntry sqlEntry("launcher_data");
	sqlEntry.strings["install_path"] = installFileName.GetFullPath();
	UpdateDatabase(sqlEntry);
}

const wxFileName& btfl::GetEssenceInstallFileName()
{
	return essenceInstallFileName;
}

void btfl::SetEssenceInstallPath(const wxString& installPath)
{
	wxFileName newFileName(installPath);
	newFileName.AppendDir("Beyond The Forbidden Lands Preview");

	if ( currentEssenceState == btfl::STATE_ToPlayGameEssence || currentEssenceState == btfl::STATE_ToUpdateGameEssence )
	{
		wxBusyCursor busyCursor;
		if ( !wxFileName::Exists(newFileName.GetFullPath()) )
			wxFileName::Mkdir(newFileName.GetFullPath());

		if ( !wxRenameFile(essenceInstallFileName.GetFullPath(), newFileName.GetFullPath()) )
			return;
	}

	essenceInstallFileName = newFileName;

	btfl::SQLEntry sqlEntry("launcher_data");
	sqlEntry.strings["essence_install_path"] = essenceInstallFileName.GetFullPath();
	UpdateDatabase(sqlEntry);
}

bool btfl::HasUserAgreedToDisclaimer()
{
	return bHasUserAgreedToDisclaimer;
}

void btfl::AgreeToDisclaimer()
{
	bHasUserAgreedToDisclaimer = true;
	
	btfl::SQLEntry sqlEntry("user_data");
	sqlEntry.integers["has_agreed_to_disclaimer"] = true;
	btfl::UpdateDatabase(sqlEntry);
}