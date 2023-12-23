using System.Runtime.InteropServices;

internal static class StringExtensions {

	public static unsafe char* ToUnmanagedString(this string input)
	{
		return (char*)Marshal.StringToHGlobalAnsi(input);
	}

}
