#pragma once
#include <memory>
#include <map>
#include "Character.h"

class World
{
	static std::unique_ptr<World> instance;
public:
	static World* GetInstance()
	{
		return instance.get();
	}

	static void MakeInstance()
	{
		instance = std::make_unique<World>();
	}

	std::map<int, std::shared_ptr<Character>> CharacterMap;

	World()
	{
		CharacterMap = std::map<int, std::shared_ptr<Character>>();
	}
};
std::unique_ptr<World> World::instance;