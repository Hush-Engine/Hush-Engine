using System.Runtime.InteropServices;

internal static class StringExtensions
{

    /// <summary>
    /// Extension method to convert a C# into a char* that can be used in C++ with ANSI encoding
    /// </summary>
    /// <param name="input">The string instance</param>
    /// <returns>A char* in the global heap, encoded in ANSI</returns>
    public static unsafe char *ToUnmanagedString(this string input)
    {
        return (char *)Marshal.StringToHGlobalAnsi(input);
    }
}
