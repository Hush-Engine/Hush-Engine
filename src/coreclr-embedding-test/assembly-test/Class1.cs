
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Test;

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

  // Receive a native char* and copy the .NET version string into it.
  [UnmanagedCallersOnly]
  public static unsafe void GetDotNetVersion(byte* buffer, int bufferLength)
  {
    string versionString = RuntimeInformation.FrameworkDescription;
    int length = Math.Min(versionString.Length, bufferLength - 1);
    fixed (char* pVersionString = versionString)
    {
      for (int i = 0; i < length; i++)
      {
        buffer[i] = (byte)pVersionString[i];
      }
    }
    buffer[length] = 0;
  }

  [UnmanagedCallersOnly]
  public static unsafe void GetStruct(TestStruct* testStruct)
  {
    // Convert to a ref struct to access the fields.
    ref TestStruct testStructRef = ref Unsafe.AsRef<TestStruct>(testStruct);
    HandleStruct(ref testStructRef);
  }

  private static void HandleStruct(ref TestStruct testStruct)
  {
    testStruct.a = 42;
    testStruct.b = 43;
  }
}