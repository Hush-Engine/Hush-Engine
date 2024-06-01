
using System.Drawing;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Xml.Linq;

namespace Test;
file class Win32Methods
{
    [DllImport("msvcrt.dll", SetLastError = false)]
    public static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
}

[StructLayout(LayoutKind.Sequential)]
public struct TestStruct
{
    public int a;
    public int b;
}

public enum LogLevel
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
}

public class Class1
{
    public static unsafe delegate *unmanaged[Stdcall]<uint, char *, void> LogHandler;
    #region NOT DEMO
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void Func()
    {
        Console.WriteLine("Hello from .NET!");
    }

    [UnmanagedCallersOnly]
    public static int SumTest(int a, int b)
    {
        return a + b;
    }

    [UnmanagedCallersOnly]
    public static float SqrRootTest(float x, float y)
    {
        return MathF.Pow(x, y);
    }

    [UnmanagedCallersOnly]
    public static unsafe void MultipleArgs(int a, int b)
    {
        Console.WriteLine($"From C#... Sum: {a + b}");
    }


    [UnmanagedCallersOnly]
    public static unsafe void GetStruct(TestStruct* testStruct)
    {
        // Convert to a ref struct to access the fields.
        ref TestStruct testStructRef = ref Unsafe.AsRef<TestStruct>(testStruct);
        HandleStruct(ref testStructRef);
    }
    #endregion

    // Receive a native char* and copy the .NET version string into it.
    [UnmanagedCallersOnly]
    public static void GetDotNetVersion()
    {
        string versionString = RuntimeInformation.FrameworkDescription;
        Console.WriteLine($"[C#] - We're running on {versionString}");
    }

    [UnmanagedCallersOnly]
    public static unsafe void SayName(char* name)
    {
        string converted = Marshal.PtrToStringAnsi((IntPtr)name);
        if (converted == null)
        {
            throw new Exception("Null string passed");
        }
        Console.WriteLine($"[C#] - Hello there {converted}");
    }

    [UnmanagedCallersOnly]
    public static unsafe char *CalculateHash(char* input)
    {
        string converted = Marshal.PtrToStringAnsi((IntPtr)input);
        if (converted == null)
        {
            throw new Exception("Null string passed");
        }
        return CalculateMD5(converted).ToUnmanagedString();
    }


    public static string CalculateMD5(string input)
    {
        using (MD5 md5 = MD5.Create())
        {
            byte[] inputBytes = Encoding.UTF8.GetBytes(input);
            byte[] hashBytes = md5.ComputeHash(inputBytes);
            return BitConverter.ToString(hashBytes).Replace("-", "").ToLower();
        }
    }

    //Also not demo


    [UnmanagedCallersOnly]
    public static unsafe void DeallocateString(char *str)
    {
        Marshal.FreeHGlobal((IntPtr)str);
    }

    public static void LogMessage(LogLevel level, string message)
    {
        unsafe
        {
            uint logLevel = (uint)level;
            char *unmanagedString = message.ToUnmanagedString();
            LogHandler(logLevel, unmanagedString);
            Marshal.FreeHGlobal((IntPtr)unmanagedString);
        }
    }

    [UnmanagedCallersOnly]
    public static unsafe void SetLogHandler(delegate *unmanaged[Stdcall]<uint, char *, void> logHandler)
    {
        LogHandler = logHandler;
    }

    private static void HandleStruct(ref TestStruct testStruct)
    {
        testStruct.a = 42;
        testStruct.b = 43;
    }
}
