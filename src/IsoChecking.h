#ifndef ISOCHECK_H_
#define ISOCHECK_H_
#pragma once

#include <wx\wx.h>
#include <wx\wfstream.h>

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
		ISO_PS3,

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
				
			case ISO_Invalid:
				continue;
			}

			if ( hash == utils::crypto::GetEncryptedString(userHash) )
				return region;
		}
		
		return ISO_Invalid;
	}

	inline wxString GetFileHash(const wxString& filePath, std::atomic<int>& gaugeUpdater)
	{
		wxFileInputStream stream(filePath);
		if ( !stream.IsOk() )
			return "";

		size_t fullLength = stream.GetLength();
		const size_t READ_LENGTH = 100000000;
		unsigned char* buffer;

		int index = 0;
		double gaugeSteps = (95 / (double(fullLength) / READ_LENGTH));

		digestpp::sha1 hasher;

		while ( stream.CanRead() )
		{
			buffer = new unsigned char[READ_LENGTH];
			stream.Read(buffer, READ_LENGTH);

			hasher.absorb(buffer, stream.LastRead());
			delete[] buffer;

			gaugeUpdater = (rand() % int(gaugeSteps)) + (gaugeSteps * index++);
		}

		return hasher.hexdigest();
	}
};

#endif