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
	tUserData.Add("has_agreed_to_disclaimer INTEGER");
	tUserData.Add("iso_file_path TEXT");
	tUserData.Add("iso_region INTEGER");

	wxArrayString tLauncherData;
	tLauncherData.Add("install_path TEXT");
	tLauncherData.Add("installed_game_version TEXT");
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


bool ShouldSaveState(btfl::LauncherState state)
{
	if ( state == btfl::STATE_VerifyingIso || state == btfl::STATE_InstallingGame || state == btfl::STATE_UpdatingGame )
		return false;

	return true;
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
}

class InstallerDownloader;

btfl::SQLDatabase* pDatabase;
btfl::LauncherState currentState = btfl::LauncherState::STATE_Initial;
btfl::LauncherState lastState = btfl::LauncherState::STATE_Initial;
wxFileName isoFileName;
bool bHasUserAgreedToDisclaimer = false;
iso::ISO_Region isoRegion = iso::ISO_Region::ISO_Invalid;

wxFileName installFileName;
wxString sInstalledGameVersion;
wxString sInstalledLauncherVersion = "v1.0.11";
wxString sLatestGameVersion;

MainFrame* pMainFrame;

InstallerDownloader* installerDownloader;

class InstallerDownloader : public wxEvtHandler
{
	wxWebRequest m_installerRequest;
	wxBusyCursor* m_pBusyCursor = nullptr;

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

		m_pBusyCursor = new wxBusyCursor;
		pMainFrame->Hide();
	}

	void OnWebRequestState(wxWebRequestEvent& event)
	{
		switch ( event.GetState() )
		{
		case wxWebRequest::State_Completed:
		{
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
			pMainFrame->Show();
			wxDELETE(m_pBusyCursor);
			wxMessageBox("There was a problem when updating the application. Please try again later");
			break;
		}
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
		userEntry.integers["has_agreed_to_disclaimer"] = false;
		userEntry.integers["iso_region"] = iso::ISO_Region::ISO_Invalid;
		userEntry.strings["iso_file_path"] = isoFileName.GetFullPath();

		btfl::SQLEntry launcherEntry("launcher_data");
		launcherEntry.strings["install_path"] = installFileName.GetFullPath();;
		launcherEntry.strings["installed_game_version"] = sInstalledGameVersion;
		launcherEntry.strings["settings_xml"] = "";

		pDatabase->InsertSQLEntry(userEntry);
		pDatabase->InsertSQLEntry(launcherEntry);

		SetInstallPath(wxGetCwd() + "/");

		btfl::SetState(btfl::LauncherState::STATE_ToSelectIso);
	}
	else
	{
		btfl::LoadLauncher(pDatabase);
	}

	pMainFrame->Show();

	installerDownloader = new InstallerDownloader;

	pMainFrame->GetMainPanel()->DoLookForLauncherUpdates();
	pMainFrame->GetMainPanel()->DoLookForGameUpdates();


	btfl::DeleteInstallerIfPresent();

	if ( currentState == btfl::STATE_ToPlayGame && !wxFileName::Exists(installFileName.GetFullPath() + "BTFL.exe") )
	{
		btfl::SetState(btfl::STATE_ToInstallGame);
	}

	CleanUpOldBuild();
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

void btfl::SetLatestGameVerstion(const wxString& version)
{
	sLatestGameVersion = version;
}

wxString btfl::GetLatestGameVersion()
{
	return sLatestGameVersion;
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

		SetState((btfl::LauncherState)result.GetInt("user_state"));
	}

	result = database->ExecuteQuery("SELECT * FROM launcher_data WHERE rowid = 1;");
	if ( result.NextRow() )
	{
		installFileName.Assign(result.GetAsString("install_path"));
		sInstalledGameVersion = result.GetAsString("installed_game_version");

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