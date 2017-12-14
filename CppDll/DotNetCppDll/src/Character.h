#pragma once
#include "../stdafx.h"
#include <string>

class Character
{
public:
	int CharacterLife;
	int CharacterMana;
	int HealManaCost;
	int HealPower;

	Character(int life, int mana)
	{
		CharacterLife = life;
		CharacterMana = mana;
	}

	std::string ToString()
	{
		return "CharacterLife : " + std::to_string(CharacterLife) + " and CharacterMana : " + std::to_string(CharacterMana) + "\n";
	}
};