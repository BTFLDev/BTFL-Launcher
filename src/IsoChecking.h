#ifndef ISOCHECK_H_
#define ISOCHECK_H_
#pragma once

#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/filename.h>

#include <atomic>

#include <digestpp.hpp>

#include "IsoHashes.h"
#include "Utils.h"

#include "wxmemdbg.h"

namespace iso
{
	const int NUMBER_OF_ISOS = 4;
	enum ISO_Region
	{
		ISO_Usa_Brazil,
		ISO_Europe_Australia,
		ISO_Japan,
		ISO_Korea,
		ISO_PS3_Usa_Brazil,
		ISO_PS3_Europe_Australia,

		ISO_Invalid
	};

	inline ISO_Region GetIsoRegion(const wxString& userHash)
	{
		for ( ISO_Region region = ISO_Usa_Brazil; region != ISO_Invalid; region = ISO_Region(region + 1) )
		{
			wxString hash;
			switch ( region )
			{
			case ISO_Usa_Brazil:
				hash = iso_us_97472;
				break;

			case ISO_Europe_Australia:
				hash = iso_eu_au_53326;
				break;

			case ISO_Japan:
				hash = iso_jp_19335;
				break;

			case ISO_Korea:
				hash = iso_ko_20061;
				break;
				
			case ISO_PS3_Usa_Brazil:
				hash = iso_ps3_98259;
				break;

			case ISO_PS3_Europe_Australia:
				hash = iso_ps3_53990;
				break;

			case ISO_Invalid:
				continue;
			}

			if ( hash == utils::crypto::GetEncryptedString(userHash) )
				return region;
		}
		
		return ISO_Invalid;
	}

	inline wxString GetFileHash(const wxString& filePath, std::atomic<int>& gaugeUpdater, wxMutex& gaugeUpdaterMutex)
	{
		wxFileInputStream stream(filePath);
		if ( !stream.IsOk() )
			return "";

		wxULongLong fullLength = 0;
		{
			wxFileName fileName(filePath);
			fullLength = fileName.GetSize();
		}

		if ( fullLength == 0 )
			fullLength = stream.GetLength();

		const size_t READ_LENGTH = 100000000;
		unsigned char* buffer;

		int index = 0;
		double gaugeSteps = (95 / (fullLength.ToDouble() / READ_LENGTH));

		digestpp::sha1 hasher;

		while ( stream.CanRead() )
		{
			buffer = new unsigned char[READ_LENGTH];
			stream.Read(buffer, READ_LENGTH);

			hasher.absorb(buffer, stream.LastRead());
			delete[] buffer;

			wxMutexLocker locker(gaugeUpdaterMutex);
			gaugeUpdater = (rand() % int(gaugeSteps)) + (gaugeSteps * index++);
		}

		return hasher.hexdigest();
	}

	inline wxString GetGameNameFromRegion(iso::ISO_Region region)
	{
		switch ( region )
		{
		case iso::ISO_Japan:
		case iso::ISO_Korea:
			return "Wander To Kyozou";
		case iso::ISO_PS3_Usa_Brazil:
		case iso::ISO_PS3_Europe_Australia:
			return "The ICO & Shadow Of The Colossus Collection";
		case iso::ISO_Invalid:
			return "Invalid ISO";
		default:
			return "Shadow Of The Colossus";
		}
	}

	inline wxString GetRegionAcronym(iso::ISO_Region region)
	{
		switch ( region )
		{
		case iso::ISO_Usa_Brazil:
			return "PlayStation 2 ISO (NTSC)";
		case iso::ISO_Europe_Australia:
			return "PlayStation 2 ISO (PAL)";
		case iso::ISO_Japan:
		case iso::ISO_Korea:
			return "PlayStation 2 ISO (NTSC-J)";
		case iso::ISO_PS3_Usa_Brazil:
			return "PlayStation 3 ISO (NTSC)";
		case iso::ISO_PS3_Europe_Australia:
			return "PlayStation 3 ISO (PAL)";
		case iso::ISO_Invalid:
			return "-";
		}
	}
};

#endif