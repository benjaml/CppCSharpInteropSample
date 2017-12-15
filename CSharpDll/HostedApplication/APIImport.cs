using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;

namespace HostedApplication
{
    public static class APIImport
    {
        [DllImport(@"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll", CallingConvention = CallingConvention.Winapi)]
        public static extern void ConstructCharacter(int id, int life, int mana); 
         
        [DllImport(@"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll", CallingConvention = CallingConvention.Winapi)]        
        public static extern void CharacterAddLife(int id, int value);

        [DllImport(@"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll", CallingConvention = CallingConvention.Winapi)]
        public static extern void CharacterRemoveLife(int id, int value);

        [DllImport(@"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll", CallingConvention = CallingConvention.Winapi)]
        public static extern void CharacterAddMana(int id, int value);

        [DllImport(@"D:\\Workspace\\KEA\\InteropSample\\CppExecutable\\x64\\Debug\\DotNetCppDll.dll", CallingConvention = CallingConvention.Winapi)]
        public static extern void CharacterRemoveMana(int id, int value);
    }
}
