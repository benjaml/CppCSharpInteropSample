#pragma once

#include "../stdafx.h"
#include "mscoree.h"
#include <string>
#include <map>



class AppDomain
{
public:
	AppDomain(std::wstring coreRootName, std::wstring targetAppPathName, ICLRRuntimeHost2* runtimeHost)
	{
		LoadAppDomain(coreRootName, targetAppPathName, runtimeHost);
	}
	~AppDomain() 
	{
		printf("delete appdomain");
	}

	int LoadAppDomain(std::wstring coreRootName, std::wstring targetAppPathName, ICLRRuntimeHost2* runtimeHost);
	int AddDelegates(int domainId, ICLRRuntimeHost2* runtimeHost);
	int AddDelegate(std::wstring route, int domainId, ICLRRuntimeHost2* runtimeHost);

	std::map<std::wstring, void *> DelegateMap;
	int DomainId;
private:

};

int AppDomain::LoadAppDomain(std::wstring coreRootName, std::wstring targetAppPathName, ICLRRuntimeHost2* runtimeHost)
{
	//
	// STEP 5: Prepare properties for the AppDomain we will create
	//

	// Flags
	// <Snippet5>
	int appDomainFlags =
		// APPDOMAIN_FORCE_TRIVIAL_WAIT_OPERATIONS |		// Do not pump messages during wait
		// APPDOMAIN_SECURITY_SANDBOXED |					// Causes assemblies not from the TPA list to be loaded as partially trusted
		APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS |			// Enable platform-specific assemblies to run
		APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP |	// Allow PInvoking from non-TPA assemblies
		APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT;			// Entirely disables transparency checks 
															// </Snippet5>


	// <Snippet6>
	// TRUSTED_PLATFORM_ASSEMBLIES
	// "Trusted Platform Assemblies" are prioritized by the loader and always loaded with full trust.
	// A common pattern is to include any assemblies next to CoreCLR.dll as platform assemblies.
	// More sophisticated hosts may also include their own Framework extensions (such as AppDomain managers)
	// in this list.
	int tpaSize = 100 * MAX_PATH; // Starting size for our TPA (Trusted Platform Assemblies) list
	wchar_t* trustedPlatformAssemblies = new wchar_t[tpaSize];
	trustedPlatformAssemblies[0] = L'\0';

	// Extensions to probe for when finding TPA list files
	wchar_t *tpaExtensions[] = {
		L"*.dll",
		L"*.exe",
		L"*.winmd"
	};

	// Probe next to CoreCLR.dll for any files matching the extensions from tpaExtensions and
	// add them to the TPA list. In a real host, this would likely be extracted into a separate function
	// and perhaps also run on other directories of interest.
	for (int i = 0; i < _countof(tpaExtensions); i++)
	{
		// Construct the file name search pattern
		wchar_t searchPath[MAX_PATH];
		wcscpy_s(searchPath, MAX_PATH, coreRootName.c_str());
		wcscat_s(searchPath, MAX_PATH, L"\\");
		wcscat_s(searchPath, MAX_PATH, tpaExtensions[i]);

		// Find files matching the search pattern
		WIN32_FIND_DATAW findData;
		HANDLE fileHandle = FindFirstFileW(searchPath, &findData);

		if (fileHandle != INVALID_HANDLE_VALUE)
		{
			do
			{
				// Construct the full path of the trusted assembly
				wchar_t pathToAdd[MAX_PATH];
				wcscpy_s(pathToAdd, MAX_PATH, coreRootName.c_str());
				wcscat_s(pathToAdd, MAX_PATH, L"\\");
				wcscat_s(pathToAdd, MAX_PATH, findData.cFileName);

				// Check to see if TPA list needs expanded
				if (wcslen(pathToAdd) + (3) + wcslen(trustedPlatformAssemblies) >= tpaSize)
				{
					// Expand, if needed
					tpaSize *= 2;
					wchar_t* newTPAList = new wchar_t[tpaSize];
					wcscpy_s(newTPAList, tpaSize, trustedPlatformAssemblies);
					trustedPlatformAssemblies = newTPAList;
				}

				// Add the assembly to the list and delimited with a semi-colon
				wcscat_s(trustedPlatformAssemblies, tpaSize, pathToAdd);
				wcscat_s(trustedPlatformAssemblies, tpaSize, L";");

				// Note that the CLR does not guarantee which assembly will be loaded if an assembly
				// is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
				// extensions. Therefore, a real host should probably add items to the list in priority order and only
				// add a file if it's not already present on the list.
				//
				// For this simple sample, though, and because we're only loading TPA assemblies from a single path,
				// we can ignore that complication.
			} while (FindNextFileW(fileHandle, &findData));
			FindClose(fileHandle);
		}
	}

	// APP_PATHS
	// App paths are directories to probe in for assemblies which are not one of the well-known Framework assemblies
	// included in the TPA list.
	//
	// For this simple sample, we just include the directory the target application is in.
	// More complex hosts may want to also check the current working directory or other
	// locations known to contain application assets.
	wchar_t appPaths[MAX_PATH * 50];

	// Just use the targetApp provided by the user and remove the file name
	wcscpy_s(appPaths, targetAppPathName.c_str());

	//APP_NI_PATHS
	// App (NI) paths are the paths that will be probed for native images not found on the TPA list. 
	// It will typically be similar to the app paths.
	// For this sample, we probe next to the app and in a hypothetical directory of the same name with 'NI' suffixed to the end.
	wchar_t appNiPaths[MAX_PATH * 50];
	wcscpy_s(appNiPaths, targetAppPathName.c_str());
	wcscat_s(appNiPaths, MAX_PATH * 50, L";");
	wcscat_s(appNiPaths, MAX_PATH * 50, targetAppPathName.c_str());
	wcscat_s(appNiPaths, MAX_PATH * 50, L"NI");

	// NATIVE_DLL_SEARCH_DIRECTORIES
	// Native dll search directories are paths that the runtime will probe for native DLLs called via PInvoke
	wchar_t nativeDllSearchDirectories[MAX_PATH * 50];
	wcscpy_s(nativeDllSearchDirectories, appPaths);
	wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, L";");
	wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, coreRootName.c_str());


	// PLATFORM_RESOURCE_ROOTS
	// Platform resource roots are paths to probe in for resource assemblies (in culture-specific sub-directories)
	wchar_t platformResourceRoots[MAX_PATH * 50];
	wcscpy_s(platformResourceRoots, appPaths);

	// AppDomainCompatSwitch
	// Specifies compatibility behavior for the app domain. This indicates which compatibility
	// quirks to apply if an assembly doesn't have an explicit Target Framework Moniker. If a TFM is
	// present on an assembly, the runtime will always attempt to use quirks appropriate to the version
	// of the TFM.
	// 
	// Typically the latest behavior is desired, but some hosts may want to default to older Silverlight
	// or Windows Phone behaviors for compatibility reasons.
	wchar_t* appDomainCompatSwitch = L"UseLatestBehaviorWhenTFMNotSpecified";
	// </Snippet6>

	//
	// STEP 6: Create the AppDomain
	//

	// <Snippet7>
	DWORD domainId;

	// Setup key/value pairs for AppDomain  properties
	const wchar_t* propertyKeys[] = {
		L"TRUSTED_PLATFORM_ASSEMBLIES",
		L"APP_PATHS",
		L"APP_NI_PATHS",
		L"NATIVE_DLL_SEARCH_DIRECTORIES",
		L"PLATFORM_RESOURCE_ROOTS",
		L"AppDomainCompatSwitch"
	};


	// Property values which were constructed in step 5
	const wchar_t* propertyValues[] = {
		trustedPlatformAssemblies,
		appPaths,
		appNiPaths,
		nativeDllSearchDirectories,
		platformResourceRoots,
		appDomainCompatSwitch
	};

	// Create the AppDomain
	HRESULT hr = runtimeHost->CreateAppDomainWithManager(
		L"Sample Host AppDomain",		// Friendly AD name
		appDomainFlags,
		NULL,							// Optional AppDomain manager assembly name
		NULL,							// Optional AppDomain manager type (including namespace)
		sizeof(propertyKeys) / sizeof(wchar_t*),
		propertyKeys,
		propertyValues,
		&domainId);

	// </Snippet7>
	if (FAILED(hr))
	{
		printf("ERROR - Failed to create AppDomain.\nError code:%x\n", hr);
		system("pause");
		return -1;
	}
	else
	{
		DomainId = domainId;
		printf("AppDomain %d created\n\n", domainId);
	}

	return AddDelegates(domainId, runtimeHost);
}

int AppDomain::AddDelegates(int domainId, ICLRRuntimeHost2* runtimeHost)
{
	DelegateMap = std::map<std::wstring, void*>();
	int ret = 0;
	/*ret += AddDelegate(L"Character.CreateCharacter", domainId, runtimeHost);
	ret += AddDelegate(L"Character.TakeDamage", domainId, runtimeHost);
	ret += AddDelegate(L"Character.Heal", domainId, runtimeHost);*/
	return (ret < 0) ? -1 : 0;
}

int AppDomain::AddDelegate(std::wstring route, int domainId, ICLRRuntimeHost2* runtimeHost)
{
	std::wstring delimiter = L".";
	std::wstring className = route.substr(0, route.find(delimiter));
	std::wstring methodName = route.substr(route.find(delimiter) + 1, route.length() - 1);
	void *delegateFunc = NULL;
	HRESULT hr = runtimeHost->CreateDelegate(
		domainId,
		L"HostedApplication",  // Target managed assembly
		(L"HostedApplication." + className).c_str(),            // Target managed type
		methodName.c_str(),                                  // Target entry point (static method)*/
		(INT_PTR*)&delegateFunc);
	if (FAILED(hr))
	{
		printf("ERROR - Failed to create delegate.\nError code:%x\n", hr);
		return -1;
	}
	DelegateMap.insert(std::pair<std::wstring, void*>(route, delegateFunc));
	return 0;
}
