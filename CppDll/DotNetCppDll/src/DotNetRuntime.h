#pragma once
#pragma warning(disable : 4996) //_CRT_SECURE_NO_WARNINGS
#include "../stdafx.h"
#include "mscoree.h"
#include <memory> 
#include <assert.h>
#include <list>
#include <stdlib.h>
#include <wchar.h>
#include <map>
#include <set>
#include <fstream>
#include "CoreCLR/coreclrhost.h"
#include "AppDomain.h"

// The host must be able to find CoreCLR.dll to start the runtime.
// This string is used as a common, known location for a centrally installed CoreCLR.dll on Windows.
// If your customers will have the CoreCLR.dll installed elsewhere, this will, of course, need modified.
// Some hosts will carry the runtime and Framework with them locally so that necessary files like CoreCLR.dll 
// are easy to find and known to be good versions. Other hosts may expect that the apps they run will include
// CoreCLR.dll next to them. Still others may rely on an environment variable being set to indicate where
// the library can be found. In the end, every host will have its own heuristics for finding the core runtime
// library, but it must always be findable in order to start the CLR.

//static const char *coreCLRInstallDirectory = "D:\\Workspace\\clr\\coreclr\\bin\\obj\\Windows_NT.x64.Debug\\src\\dlls\\mscoree\\coreclr\\Debug";
//static const char *coreCLRInstallDirectory = "%programfiles%\\dotnet\\shared\\Microsoft.NETCore.App\\2.0.3";
static const char *coreCLRInstallDirectory = "C:\\Users\\Stage\\Downloads\\dotnet-runtime-latest-win-x64\\shared\\Microsoft.NETCore.App\\2.1.0-preview1-26012-06";

// Main clr library to load
// Note that on Linux and Mac platforms, this library will
// be called libcoreclr.so or libcoreclr.dylib, respectively
static const char* coreCLRDll = "coreclr.dll";

//static const char* targetAppPath = "D:\\Workspace\\clr\\coreclr\\bin\\Product\\Windows_NT.x64.Debug";
static const char* targetAppPath = coreCLRInstallDirectory;

// Name of the environment variable controlling server GC.
// If set to 1, server GC is enabled on startup. If 0, server GC is
// disabled. Server GC is off by default.
static const char* serverGcVar = "CORECLR_SERVER_GC";

// Name of environment variable to control "System.Globalization.Invariant"
// Set to 1 for Globalization Invariant mode to be true. Default is false.
static const char* globalizationInvariantVar = "CORECLR_GLOBAL_INVARIANT";

#define symlinkEntrypointExecutable "/proc/curproc/exe"


class DotNetRuntime
{
	static std::unique_ptr<DotNetRuntime> instance;
public:
	static DotNetRuntime* GetInstance()
	{
		return instance.get();
	}

	static void MakeInstance(char* dotNetDllPath)
	{
		instance = std::make_unique<DotNetRuntime>(dotNetDllPath);
	}

	static void ReloadAppDomain(char* dotNetDllPath)
	{
		instance.reset(nullptr);
		instance = std::make_unique<DotNetRuntime>(dotNetDllPath);
	}

	std::map<std::string, void*> DelegateMap;

	DotNetRuntime(char* dotNetDllPath)
	{
		ExecuteManagedAssembly(targetAppPath, dotNetDllPath);
	}

	DotNetRuntime(const DotNetRuntime& copy) = delete;

	~DotNetRuntime()
	{
		ShutdownClr();
	}


	// variables
	void* hostHandle;
	unsigned int domainId;
	

	bool GetEntrypointExecutableAbsolutePath(std::string& entrypointExecutable)
	{
		char* hostPath = new char[MAX_PATH + 1];
		if (::GetModuleFileNameA(NULL, hostPath, MAX_PATH) == 0)
		{
			return false;
		}

		entrypointExecutable.clear();
		entrypointExecutable.append(hostPath);

		return true;
	}

	std::string CreateDllCopy(char* dotNetDllPath)
	{
		char currentDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, currentDir);
		std::string path = std::string(currentDir) + "\\..\\tmp";
		std::string t = std::string(dotNetDllPath);
		t = t.substr(t.find_last_of("\\") + 1, t.length());
		std::string newPath(path + "\\" + t);
		if (CreateDirectoryA(path.c_str(), NULL) ||
			ERROR_ALREADY_EXISTS == GetLastError())
		{
			// CopyFile(...)
			std::ifstream  src(dotNetDllPath, std::ios::binary);
			std::ofstream  dst(newPath, std::ios::binary);

			dst << src.rdbuf();
		}
		else
		{
			// Failed to create directory.
			printf("Failed to create dll directory\n");
			return nullptr;
		}
		return newPath;
	}

	const char* GetEnvValueBoolean(const char* envVariable)
	{
		char* envValue = getenv(envVariable);
		if (envValue == nullptr)
		{
			envValue = "0";
		}
		// CoreCLR expects strings "true" and "false" instead of "1" and "0".
		return (std::strcmp(envValue, "1") == 0 || _stricmp(envValue, "true") == 0) ? "true" : "false";
	}

	int ExecuteManagedAssembly(
		const char* clrFilesAbsolutePath,
		char* managedAssemblyAbsolutePath)
	{

		std::string tmp = CreateDllCopy(managedAssemblyAbsolutePath);
		managedAssemblyAbsolutePath = &tmp[0];

		std::string coreClrDllPath(coreCLRInstallDirectory);
		coreClrDllPath.append("/");
		coreClrDllPath.append(coreCLRDll);

		if (coreClrDllPath.length() >= MAX_PATH)
		{
			fprintf(stderr, "Absolute path to libcoreclr.so too long\n");
			return -1;
		}

		int tpaSize = 100 * MAX_PATH; // Starting size for our TPA (Trusted Platform Assemblies) list
		char* trustedPlatformAssemblies = new char[tpaSize];
		trustedPlatformAssemblies[0] = L'\0';

		// Extensions to probe for when finding TPA list files
		char *tpaExtensions[] = {
			"*.dll",
			"*.exe",
			"*.winmd"
		};

		// Probe next to CoreCLR.dll for any files matching the extensions from tpaExtensions and
		// add them to the TPA list. In a real host, this would likely be extracted into a separate function
		// and perhaps also run on other directories of interest.
		for (int i = 0; i < _countof(tpaExtensions); i++)
		{
			// Construct the file name search pattern
			char searchPath[MAX_PATH];
			strcpy_s(searchPath, MAX_PATH, clrFilesAbsolutePath);
			strcat_s(searchPath, MAX_PATH, "\\");
			strcat_s(searchPath, MAX_PATH, tpaExtensions[i]);

			// Find files matching the search pattern
			WIN32_FIND_DATAA findData;
			HANDLE fileHandle = FindFirstFileA(searchPath, &findData);

			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Construct the full path of the trusted assembly
					char pathToAdd[MAX_PATH];
					strcpy_s(pathToAdd, MAX_PATH, clrFilesAbsolutePath);
					strcat_s(pathToAdd, MAX_PATH, "\\");
					strcat_s(pathToAdd, MAX_PATH, findData.cFileName);

					// Check to see if TPA list needs expanded
					if (strlen(pathToAdd) + (3) + strlen(trustedPlatformAssemblies) >= tpaSize)
					{
						// Expand, if needed
						tpaSize *= 2;
						char* newTPAList = new char[tpaSize];
						strcpy_s(newTPAList, tpaSize, trustedPlatformAssemblies);
						trustedPlatformAssemblies = newTPAList;
					}

					// Add the assembly to the list and delimited with a semi-colon
					strcat_s(trustedPlatformAssemblies, tpaSize, pathToAdd);
					strcat_s(trustedPlatformAssemblies, tpaSize, ";");

					// Note that the CLR does not guarantee which assembly will be loaded if an assembly
					// is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
					// extensions. Therefore, a real host should probably add items to the list in priority order and only
					// add a file if it's not already present on the list.
					//
					// For this simple sample, though, and because we're only loading TPA assemblies from a single path,
					// we can ignore that complication.
				} while (FindNextFileA(fileHandle, &findData));
				FindClose(fileHandle);
			}
		}
		strcat_s(trustedPlatformAssemblies, tpaSize, managedAssemblyAbsolutePath);
		strcat_s(trustedPlatformAssemblies, tpaSize, ";");

		// APP_PATHS
		// App paths are directories to probe in for assemblies which are not one of the well-known Framework assemblies
		// included in the TPA list.
		//
		// For this simple sample, we just include the directory the target application is in.
		// More complex hosts may want to also check the current working directory or other
		// locations known to contain application assets.
		char appPaths[MAX_PATH * 50];

		// Just use the targetApp provided by the user and remove the file name
		strcpy_s(appPaths, managedAssemblyAbsolutePath);

		//APP_NI_PATHS
		// App (NI) paths are the paths that will be probed for native images not found on the TPA list. 
		// It will typically be similar to the app paths.
		// For this sample, we probe next to the app and in a hypothetical directory of the same name with 'NI' suffixed to the end.
		char appNiPaths[MAX_PATH * 50];
		strcpy_s(appNiPaths, clrFilesAbsolutePath);
		/*strcat_s(appNiPaths, MAX_PATH * 50, ";");
		strcat_s(appNiPaths, MAX_PATH * 50, clrFilesAbsolutePath);
		strcat_s(appNiPaths, MAX_PATH * 50, "NI");*/

		// NATIVE_DLL_SEARCH_DIRECTORIES
		// Native dll search directories are paths that the runtime will probe for native DLLs called via PInvoke
		char nativeDllSearchDirectories[MAX_PATH * 50];
		strcpy_s(nativeDllSearchDirectories, appPaths);
		strcat_s(nativeDllSearchDirectories, MAX_PATH * 50, ";");
		strcat_s(nativeDllSearchDirectories, MAX_PATH * 50, clrFilesAbsolutePath);



		HMODULE coreclrLib = LoadLibraryExA(coreClrDllPath.c_str(), NULL, 0);
		if (coreclrLib == nullptr)
		{
			fprintf(stderr, "dlopen failed to open the libcoreclr.so with error\n");
			return -1;
		}
		coreclr_initialize_ptr initializeCoreCLR = (coreclr_initialize_ptr)GetProcAddress(coreclrLib, "coreclr_initialize");
		coreclr_create_delegate_ptr createDelegateCoreCLR = (coreclr_create_delegate_ptr)GetProcAddress(coreclrLib, "coreclr_create_delegate");
		coreclr_execute_assembly_ptr executeAssembly = (coreclr_execute_assembly_ptr)GetProcAddress(coreclrLib, "coreclr_execute_assembly");
		
		if (initializeCoreCLR == nullptr)
		{
			fprintf(stderr, "Function coreclr_initialize not found in the libcoreclr.so\n");
			return -1;
		}
		else if (createDelegateCoreCLR == nullptr)
		{
			fprintf(stderr, "Function coreclr_create_delegate not found in the libcoreclr.so\n");
			return -1;
		}
		else if (executeAssembly == nullptr)
		{
			fprintf(stderr, "Function coreclr_execute_assembly not found in the libcoreclr.so\n");
			return -1;
		}
		// Check whether we are enabling server GC (off by default)
		const char* useServerGc = GetEnvValueBoolean(serverGcVar);

		// Check Globalization Invariant mode (false by default)
		const char* globalizationInvariant = GetEnvValueBoolean(globalizationInvariantVar);

		// Allowed property names:
		// APPBASE
		// - The base path of the application from which the exe and other assemblies will be loaded
		//
		// TRUSTED_PLATFORM_ASSEMBLIES
		// - The list of complete paths to each of the fully trusted assemblies
		//
		// APP_PATHS
		// - The list of paths which will be probed by the assembly loader
		//
		// APP_NI_PATHS
		// - The list of additional paths that the assembly loader will probe for ngen images
		//
		// NATIVE_DLL_SEARCH_DIRECTORIES
		// - The list of paths that will be probed for native DLLs called by PInvoke
		//
		const char *propertyKeys[] = {
			"TRUSTED_PLATFORM_ASSEMBLIES",
			"APP_PATHS",
			"APP_NI_PATHS",
			"NATIVE_DLL_SEARCH_DIRECTORIES",
			"System.GC.Server",
			"System.Globalization.Invariant",
		};

		const char *propertyValues[] = {
			// TRUSTED_PLATFORM_ASSEMBLIES
			trustedPlatformAssemblies,
			// APP_PATHS
			appPaths,
			// APP_NI_PATHS
			appNiPaths,
			// NATIVE_DLL_SEARCH_DIRECTORIES
			nativeDllSearchDirectories,
			// System.GC.Server
			useServerGc,
			// System.Globalization.Invariant
			globalizationInvariant,
		};


		std::string entryPointExecutablePath;
		if (!GetEntrypointExecutableAbsolutePath(entryPointExecutablePath))
		{
			printf("Could not get full path to current executable");
			return E_FAIL;
		}
		int st = -1;

		st = initializeCoreCLR(
			entryPointExecutablePath.c_str(),
			"unixcorerun",
			sizeof(propertyKeys) / sizeof(propertyKeys[0]),
			propertyKeys,
			propertyValues,
			&hostHandle,
			&domainId);

		if (!SUCCEEDED(st))
		{
			fprintf(stderr, "coreclr_initialize failed - status: 0x%08x\n", st);
			return -1;
		}

		st = AddDelegates(domainId, hostHandle, createDelegateCoreCLR);

		if (!SUCCEEDED(st))
		{
			fprintf(stderr, "failed to create delegates\n");
			return -1;
		}
		int exitCode;
		st = executeAssembly(
			hostHandle,
			domainId,
			0,
			{},
			managedAssemblyAbsolutePath,
			(unsigned int*)&exitCode);
		if (!SUCCEEDED(st))
		{
			fprintf(stderr, "executeAssembly failed - status: 0x%08x\n", st);
			return -1;
		}
		if (!SUCCEEDED(exitCode))
		{
			fprintf(stderr, "Assembly exited with code %d\n", exitCode);
			return -1;
		}

		return 0;
	}

	int ShutdownClr()
	{
		std::string coreClrDllPath(coreCLRInstallDirectory);
		coreClrDllPath.append("/");
		coreClrDllPath.append(coreCLRDll);

		if (coreClrDllPath.length() >= MAX_PATH)
		{
			fprintf(stderr, "Absolute path to libcoreclr.so too long\n");
			return -1;
		}
		HMODULE coreclrLib = LoadLibraryExA(coreClrDllPath.c_str(), NULL, 0);
		if (coreclrLib == nullptr)
		{
			fprintf(stderr, "dlopen failed to open the libcoreclr.so with error\n");
			return -1;
		}
		coreclr_shutdown_2_ptr shutdownCoreCLR = (coreclr_shutdown_2_ptr)GetProcAddress(coreclrLib, "coreclr_shutdown_2");

		if (shutdownCoreCLR == nullptr)
		{
			fprintf(stderr, "Function coreclr_shutdown_2 not found in the libcoreclr.so\n");
			return -1;
		}

		int latchedExitCode = 0;
		int st = shutdownCoreCLR(hostHandle, domainId, &latchedExitCode);
		if (!SUCCEEDED(st))
		{
			fprintf(stderr, "coreclr_shutdown failed - status: 0x%08x\n", st);
			return -1;
		}
		printf("CLR shutdown \n");
		return latchedExitCode;
	}

	int AddDelegates(int domainId, void* hostHandle, coreclr_create_delegate_ptr createDelegateCoreCLR)
	{
		DelegateMap = std::map<std::string, void*>();
		int ret = 0;
		ret += AddDelegate("Character.CreateCharacter", domainId, hostHandle, createDelegateCoreCLR);
		ret += AddDelegate("Character.TakeDamage", domainId, hostHandle, createDelegateCoreCLR);
		ret += AddDelegate("Character.Heal", domainId, hostHandle, createDelegateCoreCLR);
		return (ret < 0) ? -1 : 0;
	}

	int AddDelegate(std::string route, int domainId, void* hostHandle, coreclr_create_delegate_ptr createDelegateCo)
	{
		std::string delimiter = ".";
		std::string className = route.substr(0, route.find(delimiter));
		std::string methodName = route.substr(route.find(delimiter) + 1, route.length() - 1);
		void *delegateFunc = NULL;
		int st = createDelegateCo(
			hostHandle,
			domainId,
			"HostedApplication",	// entryPointAssemblyName
			("HostedApplication."+className).c_str(),      // entryPointTypeName
			methodName.c_str(),     // entryPointMethodName
			&delegateFunc);
		if (FAILED(st))
		{
			printf("ERROR - Failed to create delegate.\nError code:%x\n", st);
			return -1;
		}
		else
		{
			printf("Delegate for %s created\n", route.c_str());
		}
		DelegateMap.insert(std::pair<std::string, void*>(route, delegateFunc));
		return 0;
	}
};
std::unique_ptr<DotNetRuntime> DotNetRuntime::instance;


	/*
	std::map<int, AppDomain*> appDomains;

	// Step 3
	ICLRRuntimeHost2* runtimeHost;
	std::string targetAppPathName;
	std::string coreRootName;
	// Step 6
	DWORD domainId;

	int ReloadAppDomain(int domainId)
	{
		HRESULT hr = runtimeHost->UnloadAppDomain(domainId, true);
		AppDomain* newAppDomain = new AppDomain(coreRootName, targetAppPathName, runtimeHost);
		appDomains.insert(std::pair<int, AppDomain*>(newAppDomain->DomainId, newAppDomain));

		printf("\nReloaded\n");
		return newAppDomain->DomainId;
	}

	int AddAppDomain()
	{
		AppDomain* newAppDomain = new AppDomain(coreRootName, targetAppPathName, runtimeHost);
		appDomains.insert(std::pair<int, AppDomain*>(newAppDomain->DomainId, newAppDomain));
		return newAppDomain->DomainId;
	}

private:

	int CreateDotNetRuntime(char* dotNetDllPath)
	{
		printf("Sample CoreCLR Host\n\n");

		std::string tmp = CreateDllCopy(dotNetDllPath);
		dotNetDllPath = &tmp[0];
		//
		// STEP 1: Get the app to be run from the command line
		//

		// <Snippet1>
		// The managed application to run should be the first command-line parameter.
		// Subsequent command line parameters will be passed to the managed app later in this host.
		char targetApp[MAX_PATH];
		GetFullPathNameW(dotNetDllPath, MAX_PATH, targetApp, NULL);
		// </Snippet1>
		// Also note the directory the target app is in, as it will be referenced later.
		// The directory is determined by simply truncating the target app's full path
		// at the last path delimiter (\)

		char targetAppPath[MAX_PATH];
		wcscpy_s(targetAppPath, targetApp);
		size_t i = strlen(targetAppPath - 1);
		while (i >= 0 && targetAppPath[i] != L'\\') i--;
		targetAppPath[i] = L'\0';

		targetAppPathName = targetAppPath;
		//
		// STEP 2: Find and load CoreCLR.dll
		//
		HMODULE coreCLRModule;

		// Look in %CORE_ROOT%
		char coreRoot[MAX_PATH];
		size_t outSize;
		_wgetenv_s(&outSize, coreRoot, MAX_PATH, "CORE_ROOT");
		coreCLRModule = LoadCoreCLR(coreRoot);
		// If CoreCLR.dll wasn't in %CORE_ROOT%, look next to the target app
		if (!coreCLRModule)
		{
			wcscpy_s(coreRoot, MAX_PATH, targetAppPath);
			coreCLRModule = LoadCoreCLR(coreRoot);
		}

		// If CoreCLR.dll wasn't in %CORE_ROOT% or next to the app,
		// look in the common 1.1.0 install directory
		if (!coreCLRModule)
		{
			::ExpandEnvironmentStringsW(coreCLRInstallDirectory, coreRoot, MAX_PATH);
			coreCLRModule = LoadCoreCLR(coreRoot);
		}

		if (!coreCLRModule)
		{
			printf("ERROR - CoreCLR.dll could not be found");

			system("pause");
			return -1;
		}
		else
		{
			wprintf("CoreCLR loaded from %s\n", coreRoot);
			coreRootName = coreRoot;
		}

		coreclr_initialize_ptr initializeCoreCLR = (coreclr_initialize_ptr)GetProcAddress(coreCLRModule, "coreclr_initialize");
		coreclr_execute_assembly_ptr executeAssembly = (coreclr_execute_assembly_ptr)GetProcAddress(coreCLRModule, "coreclr_execute_assembly");
		coreclr_shutdown_2_ptr shutdownCoreCLR = (coreclr_shutdown_2_ptr)GetProcAddress(coreCLRModule, "coreclr_shutdown_2");

		if (initializeCoreCLR == nullptr)
		{
			fprintf(stderr, "Function coreclr_initialize not found in the libcoreclr.so\n");
			return -1;
		}
		else if (executeAssembly == nullptr)
		{
			fprintf(stderr, "Function coreclr_execute_assembly not found in the libcoreclr.so\n");
			return -1;
		}
		else if (shutdownCoreCLR == nullptr)
		{
			fprintf(stderr, "Function coreclr_shutdown_2 not found in the libcoreclr.so\n");
			return -1;
		}

		// Check whether we are enabling server GC (off by default)
		const char* useServerGc = GetEnvValueBoolean(serverGcVar);

		// Check Globalization Invariant mode (false by default)
		const char* globalizationInvariant = GetEnvValueBoolean(globalizationInvariantVar);

		// Allowed property names:
		// APPBASE
		// - The base path of the application from which the exe and other assemblies will be loaded
		//
		// TRUSTED_PLATFORM_ASSEMBLIES
		// - The list of complete paths to each of the fully trusted assemblies
		//
		// APP_PATHS
		// - The list of paths which will be probed by the assembly loader
		//
		// APP_NI_PATHS
		// - The list of additional paths that the assembly loader will probe for ngen images
		//
		// NATIVE_DLL_SEARCH_DIRECTORIES
		// - The list of paths that will be probed for native DLLs called by PInvoke
		//
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
		char* trustedPlatformAssemblies = new char[tpaSize];
		trustedPlatformAssemblies[0] = L'\0';

		// Extensions to probe for when finding TPA list files
		char *tpaExtensions[] = {
			"*.dl",
			"*.exe",
			"*.winmd"
		};

		// Probe next to CoreCLR.dll for any files matching the extensions from tpaExtensions and
		// add them to the TPA list. In a real host, this would likely be extracted into a separate function
		// and perhaps also run on other directories of interest.
		for (int i = 0; i < _countof(tpaExtensions); i++)
		{
			// Construct the file name search pattern
			char searchPath[MAX_PATH];
			wcscpy_s(searchPath, MAX_PATH, coreRootName.c_str());
			wcscat_s(searchPath, MAX_PATH, "\\");
			wcscat_s(searchPath, MAX_PATH, tpaExtensions[i]);

			// Find files matching the search pattern
			WIN32_FIND_DATAW findData;
			HANDLE fileHandle = FindFirstFileW(searchPath, &findData);

			if (fileHandle != INVALID_HANDLE_VALUE)
			{
				do
				{
					// Construct the full path of the trusted assembly
					char pathToAdd[MAX_PATH];
					wcscpy_s(pathToAdd, MAX_PATH, coreRootName.c_str());
					wcscat_s(pathToAdd, MAX_PATH, "\\");
					wcscat_s(pathToAdd, MAX_PATH, findData.cFileName);

					// Check to see if TPA list needs expanded
					if (strlen(pathToAdd) + (3) + strlen(trustedPlatformAssemblies) >= tpaSize)
					{
						// Expand, if needed
						tpaSize *= 2;
						char* newTPAList = new char[tpaSize];
						wcscpy_s(newTPAList, tpaSize, trustedPlatformAssemblies);
						trustedPlatformAssemblies = newTPAList;
					}

					// Add the assembly to the list and delimited with a semi-colon
					wcscat_s(trustedPlatformAssemblies, tpaSize, pathToAdd);
					wcscat_s(trustedPlatformAssemblies, tpaSize, ";");

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
		char appPaths[MAX_PATH * 50];

		// Just use the targetApp provided by the user and remove the file name
		wcscpy_s(appPaths, targetAppPathName.c_str());

		//APP_NI_PATHS
		// App (NI) paths are the paths that will be probed for native images not found on the TPA list.
		// It will typically be similar to the app paths.
		// For this sample, we probe next to the app and in a hypothetical directory of the same name with 'NI' suffixed to the end.
		char appNiPaths[MAX_PATH * 50];
		wcscpy_s(appNiPaths, targetAppPathName.c_str());
		wcscat_s(appNiPaths, MAX_PATH * 50, ";");
		wcscat_s(appNiPaths, MAX_PATH * 50, targetAppPathName.c_str());
		wcscat_s(appNiPaths, MAX_PATH * 50, "NI");

		// NATIVE_DLL_SEARCH_DIRECTORIES
		// Native dll search directories are paths that the runtime will probe for native DLLs called via PInvoke
		char nativeDllSearchDirectories[MAX_PATH * 50];
		wcscpy_s(nativeDllSearchDirectories, appPaths);
		wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, ";");
		wcscat_s(nativeDllSearchDirectories, MAX_PATH * 50, coreRootName.c_str());


		// PLATFORM_RESOURCE_ROOTS
		// Platform resource roots are paths to probe in for resource assemblies (in culture-specific sub-directories)
		char platformResourceRoots[MAX_PATH * 50];
		wcscpy_s(platformResourceRoots, appPaths);

		// AppDomainCompatSwitch
		// Specifies compatibility behavior for the app domain. This indicates which compatibility
		// quirks to apply if an assembly doesn't have an explicit Target Framework Moniker. If a TFM is
		// present on an assembly, the runtime will always attempt to use quirks appropriate to the version
		// of the TFM.
		//
		// Typically the latest behavior is desired, but some hosts may want to default to older Silverlight
		// or Windows Phone behaviors for compatibility reasons.
		char* appDomainCompatSwitch = "UseLatestBehaviorWhenTFMNotSpecified";
		// </Snippet6>

		//
		// STEP 6: Create the AppDomain
		//

		// <Snippet7>
		DWORD domainId;

		// Setup key/value pairs for AppDomain  properties
		const char* propertyKeys[] = {
			"TRUSTED_PLATFORM_ASSEMBLIES",
			"APP_PATHS",
			"APP_NI_PATHS",
			"NATIVE_DLL_SEARCH_DIRECTORIES",
			"PLATFORM_RESOURCE_ROOTS",
			"AppDomainCompatSwitch"
		};


		// Property values which were constructed in step 5
		const char* propertyValues[] = {
			trustedPlatformAssemblies,
			appPaths,
			appNiPaths,
			nativeDllSearchDirectories,
			platformResourceRoots,
			appDomainCompatSwitch
		};

		// Create the AppDomain
		HRESULT hr = runtimeHost->CreateAppDomainWithManager(
			"Sample Host AppDomain",		// Friendly AD name
			appDomainFlags,
			NULL,							// Optional AppDomain manager assembly name
			NULL,							// Optional AppDomain manager type (including namespace)
			sizeof(propertyKeys) / sizeof(char*),
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
			printf("AppDomain %d created\n\n", domainId);
		}

		/*
		//
		// STEP 3: Get ICLRRuntimeHost2 instance
		//

		// <Snippet3>


		/*
		FnGetCLRRuntimeHost pfnGetCLRRuntimeHost =
			(FnGetCLRRuntimeHost)::GetProcAddress(coreCLRModule, "GetCLRRuntimeHost");

		if (!pfnGetCLRRuntimeHost)
		{
			printf("ERROR - GetCLRRuntimeHost not found");
			system("pause");
			return -1;
		}

		// Get the hosting interface
		HRESULT hr = pfnGetCLRRuntimeHost(IID_ICLRRuntimeHost2, (IUnknown**)&runtimeHost);
		// </Snippet3>

		if (FAILED(hr))
		{
			printf("ERROR - Failed to get ICLRRuntimeHost2 instance.\nError code:%x\n", hr);
			system("pause");
			return -1;
		}

		//
		// STEP 4: Set desired startup flags and start the CLR
		//

		// <Snippet4>
		hr = runtimeHost->SetStartupFlags(
			// These startup flags control runtime-wide behaviors.
			// A complete list of STARTUP_FLAGS can be found in mscoree.h,
			// but some of the more common ones are listed below.
			static_cast<STARTUP_FLAGS>(
				//STARTUP_FLAGS::STARTUP_SERVER_GC |								// Use server GC
				//STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN |		// Maximize domain-neutral loading
				//STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST |	// Domain-neutral loading for strongly-named assemblies
				//STARTUP_FLAGS::STARTUP_CONCURRENT_GC |						// Use concurrent GC
				//STARTUP_FLAGS::STARTUP_SINGLE_APPDOMAIN |					// All code executes in the default AppDomain
																			// (required to use the runtimeHost->ExecuteAssembly helper function)
				STARTUP_FLAGS::STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN	// Prevents domain-neutral loading
				)
		);
		// </Snippet4>


		if (FAILED(hr))
		{
			printf("ERROR - Failed to set startup flags.\nError code:%x\n", hr);
			system("pause");
			return -1;
		}

		hr = runtimeHost->Start();
		if (FAILED(hr))
		{
			printf("ERROR - Failed to start the runtime.\nError code:%x\n", hr);
			system("pause");
			return -1;
		}
		else
		{
			printf("Runtime started\n");
		}
		return 0;
	}



	

	int DestroyDotNetRuntime()
	{
		//
		// STEP 8: Clean up
		//

		// <Snippet9>
		runtimeHost->UnloadAppDomain(1, true);
		runtimeHost->Stop();
		runtimeHost->Release();

		printf("\nDone\n");
		return 0;
	}

	// Check for CoreCLR.dll in a given path and load it, if possible
	HMODULE LoadCoreCLR(const char* directoryPath)
	{
		char coreDllPath[MAX_PATH];
		wcscpy_s(coreDllPath, MAX_PATH, directoryPath);
		wcscat_s(coreDllPath, MAX_PATH, "\\");
		wcscat_s(coreDllPath, MAX_PATH, coreCLRDll);

		// <Snippet2>
		HMODULE ret = LoadLibraryEx(coreDllPath, NULL, 0);
		// </Snippet2>
		return ret;
	}


};*/
