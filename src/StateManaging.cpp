#include "StateManaging.h"
#include "MainFrame.h"
#include "IsoHashes.h"
#include "IsoChecking.h"

#include <wx/xml/xml.h>
#include <wx/sstream.h>

#include <thread>

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
	tLauncherData.Add("installed_version TEXT");
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
btfl::LauncherState currentState = btfl::LauncherState::STATE_Initial;
wxFileName isoFileName;
bool bHasUserAgreedToDisclaimer = false;
iso::ISO_Region isoRegion = iso::ISO_Region::ISO_Invalid;

wxFileName installFileName("./Game");
wxString sInstalledVersion;

MainFrame* pMainFrame;

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
		launcherEntry.strings["installed_version"] = sInstalledVersion;
		launcherEntry.strings["settings_xml"] = "";

		pDatabase->InsertSQLEntry(userEntry);
		pDatabase->InsertSQLEntry(launcherEntry);

		btfl::SetState(btfl::LauncherState::STATE_ToSelectIso);
	}
	else
	{
		btfl::LoadLauncher(pDatabase);
	}

	pMainFrame->Show();
}

void btfl::ShutDown()
{
	pDatabase->Close();
	wxSQLite3Database::ShutdownSQLite();
	delete pDatabase;
	pDatabase = nullptr;
}

void btfl::SetState(btfl::LauncherState state)
{
	if ( state == currentState )
		return;

	pMainFrame->SetState(state);
	currentState = state;

	btfl::SQLEntry sqlEntry("user_data");
	sqlEntry.integers["user_state"] = currentState;
	
	UpdateDatabase(sqlEntry);
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
		sInstalledVersion = result.GetAsString("installed_version");

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
			pDatabase->Begin();
			pDatabase->UpdateSQLEntry(sqlEntry);
			pDatabase->Commit();
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
	installFileName.Assign(installPath);

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