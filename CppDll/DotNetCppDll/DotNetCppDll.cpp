
// DotNetCppDll.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "src/DotNetRuntime.h"
#include "src/Character.h"
#include "src/World.h"

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)


// Typedef for dot net delegates
typedef void (STDMETHODCALLTYPE NoArgsNoReturnMethod)();
typedef void (STDMETHODCALLTYPE IntParameterMethod)(int value);
typedef void (STDMETHODCALLTYPE TwoIntParameterMethod)(int value1, int value2);

#pragma region dotnetfunctions

EXTERN_DLL_EXPORT void ConstructCharacter(int id, int maxLife, int maxMana)
{
	std::shared_ptr<Character> ret = std::make_shared<Character>(maxLife, maxMana);
	World::GetInstance()->CharacterMap.insert(std::pair<int, std::shared_ptr<Character>>(id, ret));
}

EXTERN_DLL_EXPORT void CharacterAddLife(int id, int value)
{
	World::GetInstance()->CharacterMap.at(id)->CharacterLife += value;
}

EXTERN_DLL_EXPORT void CharacterRemoveLife(int id, int value)
{
	World::GetInstance()->CharacterMap.at(id)->CharacterLife -= value;
}

EXTERN_DLL_EXPORT void CharacterAddMana(int id, int value)
{
	World::GetInstance()->CharacterMap.at(id)->CharacterMana += value;
}

EXTERN_DLL_EXPORT void CharacterRemoveMana(int id, int value)
{
	World::GetInstance()->CharacterMap.at(id)->CharacterMana -= value;
}

#pragma endregion

#pragma region cppfunctions

EXTERN_DLL_EXPORT void Test()
{
	if (DotNetRuntime::GetInstance() == nullptr)
	{
		printf("CLR environment is not running try to call 'Init' before 'Test'\n");
		return;
	}
	void* func = DotNetRuntime::GetInstance()->DelegateMap.at("Character.Test");
	if (func == nullptr)
	{
		printf("Delegate for %s not found\n", "Character.Test");
		return;
	}
	try
	{
		((NoArgsNoReturnMethod*)func)();
	}
	catch (...)
	{
		//printf("Exception : %s", e.what());
	}
}


EXTERN_DLL_EXPORT void Init(const char *dotNetDllPath, const char* entrypointExecutable)
{
	printf("Create dot net runtime\n");
	DotNetRuntime::MakeInstance(dotNetDllPath, entrypointExecutable);
	World::MakeInstance();
}

EXTERN_DLL_EXPORT void CreateCharacter(int id, int domainId)
{
	if (DotNetRuntime::GetInstance() == nullptr)
	{
		printf("CLR environment is not running try to call 'Init', then 'CreateAppDomain' before 'CreateCharacter'\n");
		return;
	}
	void* func = DotNetRuntime::GetInstance()->DelegateMap.at("Character.CreateCharacter");
	if (func == nullptr)
	{
		printf("Delegate for %s not found\n", "Character.CreateCharacter");
		return;
	}
	try
	{
		((IntParameterMethod*)func)(id);
	}
	catch (const std::exception& e)
	{
		printf("Exception : %s", e.what());
	}
}

EXTERN_DLL_EXPORT void ReloadAppDomain()
{
	/*if (DotNetRuntime::GetInstance() == nullptr)
	{
		printf("CLR environment is not running try to call 'Init', then 'CreateAppDomain' before 'ReloadAppDomain'\n");
		return -1;
	}
	return DotNetRuntime::GetInstance()->ReloadAppDomain(id);*/
}

EXTERN_DLL_EXPORT void DisplayCharacterStatus(int id)
{
	printf("%s\n",World::GetInstance()->CharacterMap.at(id)->ToString().c_str());
}

EXTERN_DLL_EXPORT void CharacterTakeDamage(int id, int damage, int domainId) 
{
	if (DotNetRuntime::GetInstance() == nullptr)
	{
		printf("CLR environment is not running try to call 'Init', then 'CreateAppDomain' before 'CharacterTakeDamage'\n");
		return;
	}
	void* func = DotNetRuntime::GetInstance()->DelegateMap.at("Character.TakeDamage");
	if (func == nullptr)
	{
		printf("Delegate for %s not found\n", "Character.TakeDamage");
		return;
	}
	((TwoIntParameterMethod*)func)(id, damage);
}

EXTERN_DLL_EXPORT void CharacterHeal(int id, int domainId)
{
	if (DotNetRuntime::GetInstance() == nullptr)
	{
		printf("CLR environment is not running try to call 'Init' before 'CharacterHeal'\n");
		return;
	}
	void* func = DotNetRuntime::GetInstance()->DelegateMap.at("Character.Heal");
	if (func == nullptr)
	{
		printf("Delegate for %s not found\n", "Character.Heal");
		return;
	}
	((IntParameterMethod*)func)(id);
}
#pragma endregion
