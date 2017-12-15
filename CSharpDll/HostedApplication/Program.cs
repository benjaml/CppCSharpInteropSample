using System;
using System.Runtime.InteropServices;

namespace HostedApplication
{

    public delegate void characterCreation(int maxLife, int maxMana);
    delegate void MyDelegate(IntPtr foo);

    class Character
    {
        public static int CharacterMaxLife = 100; 
        public static int CharacterMaxMana = 100; 
        public static int HealManaCost = 25; 
        public static int HealPower = 25;

        static void CreateCharacter(int id)
        {
            APIImport.ConstructCharacter(id, CharacterMaxLife, CharacterMaxMana);     
        }

        static void TakeDamage(int id, int value)
        { 
            APIImport.CharacterRemoveLife(id, value);                    
        }     
              
        static void Heal(int id)   
        { 
            APIImport.CharacterAddLife(id, HealPower);
            APIImport.CharacterRemoveMana(id, HealManaCost);           
        }

    };

    class Program
    {
        static void Main(string[] args)
        {
            Console.WriteLine("Hello World!");
        }
        
    }

}
