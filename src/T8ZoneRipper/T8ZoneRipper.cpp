// Precompiled 
#include "stdafx.h"
// CASC Inclues
#include "CascLib.h"
#include "CascCommon.h"
// WraithX Includes
#include "FileSystems.h"
#include "Strings.h"

// Extracts CASC files, included split up entries
void ExtractFile(const std::string ResultingFile, CASC_FIND_DATA CASCFindData, HANDLE StorageHandle)
{
	// Bytes Read
	DWORD BytesRead = 0;

	// File Handle
	HANDLE FileHandle;

	// Open the parent file
	if (CascOpenFile(StorageHandle, CASCFindData.szFileName, CASC_LOCALE_ALL, 0, &FileHandle))
	{
		// CASC File Name
		auto FileName = std::string(CASCFindData.szFileName);

		// Info
		printf("> Extracting: %s\n", CASCFindData.szFileName);

		// Create Directory
		FileSystems::CreateDirectory(FileSystems::GetDirectoryName(ResultingFile));

		// Destination File
		FILE* Destination;

		//
		auto Result = fopen_s(&Destination, ResultingFile.c_str(), "wb");

		// Check for result
		if (Result == 0)
		{
			// Write Initial file
			while (true)
			{
				uint8_t* Buffer = new uint8_t[0x2000000];
				// Write if we have read
				if (CascReadFile(FileHandle, Buffer, 0x2000000, &BytesRead))
					fwrite(Buffer, BytesRead, 1, Destination);
				// Clean Up
				delete[] Buffer;
				// Done
				if (BytesRead == 0) break;
			}

			// Close Handle
			CascCloseFile(FileHandle);
			// Check for split files
			for (uint8_t i = 2; i < MAXBYTE; i++)
			{
				// Sub File Path
				auto SplitFilePath = Strings::Format("%s_%i", FileName.c_str(), i).c_str();

				// Check if it exists, if it doesn't, stop here
				if (!CascOpenFile(StorageHandle, SplitFilePath, CASC_LOCALE_ALL, 0, &FileHandle))
					break;

				printf(">	Extracting: %s\n", SplitFilePath);

				// Write Remaining files
				while (true)
				{
					uint8_t* Buffer = new uint8_t[0x2000000];
					// Write if we have read
					if (CascReadFile(FileHandle, Buffer, 0x2000000, &BytesRead))
						fwrite(Buffer, BytesRead, 1, Destination);
					// Clean Up
					delete[] Buffer;
					// If 0, complete
					if (BytesRead == 0) break;
				}

				// Close Handle
				CascCloseFile(FileHandle);
			}
		}
		else
		{
			// Allocate 
			char ErrorBuffer[0xFF];
			// Copy error into buffer
			strerror_s(ErrorBuffer, 0xFF, Result);
			// Print
			printf("> Error: %s\n", ErrorBuffer);
		}
	}

	// Close all handles we created
	_fcloseall();
}

int main()
{
	// Current Directory
	std::string Directory = FileSystems::GetApplicationPath();

	// Handles
	HANDLE StorageHandle;
	HANDLE FoundHandle;

	// File was found or not
	bool FileFound = true;

	// Counts
	DWORD TotalFileCount = 0;
	
	// Found Data Information
	CASC_FIND_DATA CASCFindData;

	// Valid Files
	std::map<std::string, CASC_FIND_DATA> ValidFiles;

	// Initial Print
	printf("> ---------------------------\n");
	printf("> T8ZoneRipper by Scobalula\n");
	printf("> Version 0.1.0.0\n");
	printf("> Rips and merges T8ZoneFiles\n");
	printf("> ---------------------------\n");

	printf("> Attempting to open CASC Storage: %s\n", Directory.c_str());

	// Attempt to open the storage
	if (!CascOpenStorage(Directory.c_str(), 0, &StorageHandle))
	{
		printf("> Failed to open storage, make sure this exe is in Bo4's folder");
	}
	else
	{
		printf("> Opened storage successfully, preparing to extract Zone..\n");

		// Get storage information
		CascGetStorageInfo(StorageHandle, CascStorageFileCount, &TotalFileCount, sizeof(TotalFileCount), NULL);

		// Get the first file
		FoundHandle = CascFindFirstFile(StorageHandle, "*", &CASCFindData, NULL);

		// Check is it valud
		if (FoundHandle != NULL)
		{
			// Loop while we have a file
			while (FileFound)
			{
				// CASC File Name
				auto FileName = std::string(CASCFindData.szFileName);

				// As far as we're concerned, any file that results in a valid split, is not one we want, nor is one that's not apart of zone
				if (Strings::SplitString(FileSystems::GetExtension(FileName), '_').size() == 1 && Strings::SplitString(FileName, '\\')[0] == "zone")
				{
					// Add to map, without CASC Data
					ValidFiles[FileSystems::CombinePath(Directory, std::string(CASCFindData.szFileName))] = CASCFindData;
				}

				// Process next CASC File
				FileFound = CascFindNextFile(FoundHandle, &CASCFindData);
			}

			// Close Search
			CascFindClose(FoundHandle);
		}

		// Loop over our valid files
		for (auto const& ValidFile : ValidFiles)
		{
			ExtractFile(ValidFile.first, ValidFile.second, StorageHandle);
		}

		// Close the storage
		CascCloseStorage(StorageHandle);
	}

	std::cin.get();
	return 0;
}

