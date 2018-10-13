// Precompiled 
#include "stdafx.h"
// CASC Inclues
#include "CascLib.h"
#include "CascCommon.h"
// WraithX Includes
#include "FileSystems.h"
#include "Strings.h"

// List of CASC Files and their Meta Data
std::map<std::string, CASC_FIND_DATA> CASCFiles;

// Extracts CASC files, included split up entries
void ExtractFile(const std::string ResultingFile, CASC_FIND_DATA CASCFindData, HANDLE StorageHandle)
{
	// Info
	printf("> Extracting: %s\n", CASCFindData.szFileName);
	// Bytes Read
	DWORD BytesRead = 0;

	// File Handle
	HANDLE FileHandle;

	// Open the parent file
	if (CascOpenFile(StorageHandle, CASCFindData.szFileName, CASC_LOCALE_ALL, 0, &FileHandle))
	{
		// Total File Size
		uint64_t FileSize = CASCFindData.dwFileSize;

		// Check file size (don't waste time with 0 size files)
		if (FileSize == 0)
		{
			// Skip
			printf("> Skipping file, zero size.\n");
			return;
		}

		// Number of split files
		uint8_t SplitCount = 0;

		// Determine File Size and how many files we have from all split files
		for (uint8_t i = 2; i < MAXBYTE; i++)
		{
			// Sub File Path
			auto SplitFilePath = Strings::Format("%s_%i", ResultingFile.c_str(), i);

			// Check if it exists, if it doesn't, stop here
			if (CASCFiles.find(SplitFilePath) == CASCFiles.end())
				break;

			// Append it
			FileSize += CASCFiles[SplitFilePath].dwFileSize;

			// Incrememnt our split count
			SplitCount++;
		}

		// Determine if this file exists, and use file size to determine if we should skip it
		if (FileSystems::FileExists(ResultingFile))
		{
			// File Information
			WIN32_FILE_ATTRIBUTE_DATA fInfo;

			// Get File Attributes
			if (GetFileAttributesExA(ResultingFile.c_str(), GetFileExInfoStandard, &fInfo))
			{
				// Check file size
				if ((fInfo.nFileSizeHigh * 4294967296) + fInfo.nFileSizeLow == FileSize)
				{
					// Skip
					printf("> Skipping file, same size as on disk.\n");
					return;
				}
			}
		}

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
			for (uint8_t i = 0; i < SplitCount; i++)
			{
				// Sub File Path
				auto SplitFilePath = Strings::Format("%s_%i", CASCFindData.szFileName, i + 2);

				// Open the CASC file, skip if we failed
				if (!CascOpenFile(StorageHandle, SplitFilePath.c_str(), CASC_LOCALE_ALL, 0, &FileHandle))
					break;

				printf(">	Extracting: %s\n", SplitFilePath.c_str());

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

	// Info
	printf("> Extracted to: %s\n", ResultingFile.c_str());
	// Close all handles we created
	_fcloseall();
}

int main(int argc, char** argv)
{
#if _DEBUG
	// Debug (Specify our folder explicitly)
	std::string Directory = "C:\\BattleLib\\Call of Duty Black Ops 4";
#else
	// Current Directory
	std::string Directory = FileSystems::GetApplicationPath();
#endif

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

				// Only take zone files
				if (Strings::SplitString(FileName, '\\')[0] == "zone")
					// Add to map, with CASC Data
					CASCFiles[FileSystems::CombinePath(Directory, std::string(CASCFindData.szFileName))] = CASCFindData;

				// Process next CASC File
				FileFound = CascFindNextFile(FoundHandle, &CASCFindData);
			}

			// Close Search
			CascFindClose(FoundHandle);
		}

		// Loop over our valid files
		for (auto const& CASCFile : CASCFiles)
			// As far as we're concerned, any file that results in a valid split, is not one we want to (it'll be merged)
			if (Strings::SplitString(FileSystems::GetExtension(CASCFile.first), '_').size() == 1)
				// Ship to extractor
				ExtractFile(CASCFile.first, CASCFile.second, StorageHandle);

		// Close the storage
		CascCloseStorage(StorageHandle);
	}

	// Wait for exit
	printf("> Extraction complete.");
	std::cin.get();
	return 0;
}

