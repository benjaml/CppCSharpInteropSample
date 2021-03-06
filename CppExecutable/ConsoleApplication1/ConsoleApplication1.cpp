// ConsoleApplication1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <windows.h>

typedef void(__stdcall *StringParameterFunc)(char *dotNetDllPath);
typedef void(__stdcall *IntParameterFunc)(int value);
typedef int(__stdcall *IntParameterAndReturnFunc)(int value);
typedef void(__stdcall *TwoIntParameterFunc)(int value1, int value2);
typedef int(__stdcall *TwoIntParameterAndIntReturnFunc)(int value1, int value2);
typedef void(__stdcall *ThreeIntParameterFunc)(int value1, int value2, int value3);
typedef void(__stdcall *NoParameterFunc)();
typedef int(__stdcall *IntReturnFunc)();

LPCWSTR CppDllPath = L"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll";
char* DotNetDllPath = "D:\\Workspace\\KEA\\InteropSample\\CSharpDll\\HostedApplication\\bin\\Debug\\netcoreapp2.0\\HostedApplication.dll";


#using <System.dll>

using namespace System;
using namespace System::IO;
using namespace System::Security::Permissions;


int DefaultAppDomainId;

public ref class Watcher
{
private:
	static StringParameterFunc ReloadCallBack;
	// Define the event handlers.
	static void OnChanged(Object^, FileSystemEventArgs^ e)
	{
		// Specify what is done when a file is changed, created, or deleted.
		ReloadCallBack(DotNetDllPath);
		Console::WriteLine("File: {0} {1}", e->FullPath, e->ChangeType);
	}

	static void OnRenamed(Object^, RenamedEventArgs^ e)
	{
		// Specify what is done when a file is renamed.
		Console::WriteLine("File: {0} renamed to {1}", e->OldFullPath, e->FullPath);
	}

public:
	[PermissionSet(SecurityAction::Demand, Name = "FullTrust")]
	int static run(StringParameterFunc reloadCallback)
	{
		array<String^>^args = System::Environment::GetCommandLineArgs();
		
		// Create a new FileSystemWatcher and set its properties.
		FileSystemWatcher^ watcher = gcnew FileSystemWatcher;

		//GetEnvironmentVariable(L"SolutionDir", path, MAX_PATH);
		watcher->Path = L"D:\\Workspace\\KEA\\InteropSample\\CSharpDll\\HostedApplication\\bin\\Debug\\netcoreapp2.0";

		// Watch for changes in LastAccess and LastWrite times, and
		// the renaming of files or directories. 
		watcher->NotifyFilter = static_cast<NotifyFilters>(NotifyFilters::LastWrite);

		// Only watch text files.
		watcher->Filter = "*.dll";

		// Add event handlers.
		watcher->Changed += gcnew FileSystemEventHandler(Watcher::OnChanged);
		Watcher::ReloadCallBack = reloadCallback;

		// Begin watching.
		watcher->EnableRaisingEvents = true;

		return 0;
	}
};

int main()
{

	HINSTANCE hGetProcIDDLL = LoadLibrary(CppDllPath);

	if (!hGetProcIDDLL) {
		std::cout << "could not load the dynamic library" << std::endl;
		return EXIT_FAILURE;
	}

	// Init CLR
	StringParameterFunc Init = (StringParameterFunc)GetProcAddress(hGetProcIDDLL, "Init");
	if (!Init) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// Create player
	TwoIntParameterFunc CreateCharacter = (TwoIntParameterFunc)GetProcAddress(hGetProcIDDLL, "CreateCharacter");
	if (!CreateCharacter) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// Take damage
	ThreeIntParameterFunc TakeDamage = (ThreeIntParameterFunc)GetProcAddress(hGetProcIDDLL, "CharacterTakeDamage");
	if (!TakeDamage) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// Display Character status
	IntParameterFunc DisplayCharacterStatus = (IntParameterFunc)GetProcAddress(hGetProcIDDLL, "DisplayCharacterStatus");
	if (!DisplayCharacterStatus) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// Heal
	TwoIntParameterFunc CharacterHeal = (TwoIntParameterFunc)GetProcAddress(hGetProcIDDLL, "CharacterHeal");
	if (!CharacterHeal) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// Heal
	StringParameterFunc ReloadAppDomain = (StringParameterFunc)GetProcAddress(hGetProcIDDLL, "ReloadAppDomain");
	if (!ReloadAppDomain) {
		std::cout << "could not locate the function" << std::endl;
		return EXIT_FAILURE;
	}

	// This will watch any change in the dll folder to reload the appDomain
	Watcher::run(ReloadAppDomain);
	std::string entrypointExecutable;
	Init(DotNetDllPath);

	printf("Create Character\n");


	CreateCharacter(0, DefaultAppDomainId);
	DisplayCharacterStatus(0);
	printf("Take 25 damage\n");
	TakeDamage(0, 25, DefaultAppDomainId);
	DisplayCharacterStatus(0);
	printf("Heal\n");
	CharacterHeal(0, DefaultAppDomainId);
	DisplayCharacterStatus(0);

	system("pause");

	CreateCharacter(1, DefaultAppDomainId);
	DisplayCharacterStatus(1);
	printf("Take 25 damage\n");
	TakeDamage(1, 25, DefaultAppDomainId);
	DisplayCharacterStatus(1);
	printf("Heal\n");
	CharacterHeal(1, DefaultAppDomainId);
	DisplayCharacterStatus(1);

	return EXIT_SUCCESS;
}
