
using System.Drawing;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
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

public class Class1
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static void Func()
    {
        Console.WriteLine("Hello from .NET!");
    }

    [UnmanagedCallersOnly]
    public static unsafe void SayName(char *name, int size)
    {
        Console.WriteLine("We're now in C# :D");
        Console.WriteLine($"Size of the string passed: {size}");
        string nameStr = Marshal.PtrToStringAnsi((IntPtr)name, size);
        Console.WriteLine($"Hi there {nameStr}, look, we're interpolating with a parameter!");
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

    // Receive a native char* and copy the .NET version string into it.
    [UnmanagedCallersOnly]
    public static unsafe void GetDotNetVersion(byte *buffer, int bufferLength)
    {
        string versionString = RuntimeInformation.FrameworkDescription;
        int length = Math.Min(versionString.Length, bufferLength - 1);
        fixed(char *pVersionString = versionString)
        {
            for (int i = 0; i < length; i++)
            {
                buffer[i] = (byte)pVersionString[i];
            }
        }
        buffer[length] = 0;
    }

    [UnmanagedCallersOnly]
    public static unsafe void GetStruct(TestStruct *testStruct)
    {
        // Convert to a ref struct to access the fields.
        ref TestStruct testStructRef = ref Unsafe.AsRef<TestStruct>(testStruct);
        HandleStruct(ref testStructRef);
    }

    [UnmanagedCallersOnly]
    public static unsafe char *GetCsharpString()
    {
        const int SIZE = 50;
        Span<char> buffer = stackalloc char[SIZE];
        Random r = new Random();
        for (int i = 0; i < SIZE; i++)
        {
            buffer[i] = (char)r.Next(65, 91);
        }
        string testedBuffer = buffer.ToString();
        Console.WriteLine($"C# says that the resulting string is: {testedBuffer}");
        return testedBuffer.ToUnmanagedString();
    }

    private static void HandleStruct(ref TestStruct testStruct)
    {
        testStruct.a = 42;
        testStruct.b = 43;
    }
}
