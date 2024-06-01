using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Test;

public class ApiTest
{
    [UnmanagedCallersOnly]
    public static void CallObjectApi()
    {
        Random r = new Random();
        int randomPage = r.Next(1, 15);
        HttpClient client = new HttpClient();
        HttpRequestMessage req = new HttpRequestMessage(HttpMethod.Get, $"https://api.restful-api.dev/objects/{randomPage}");
        HttpResponseMessage resp = client.Send(req);
        Console.WriteLine($"[C#] Request Status: {resp.StatusCode}");
        if (!resp.IsSuccessStatusCode)
        {
            return;
        }
        Span<byte> buffer = stackalloc byte[800];
        resp.Content.ReadAsStream().Read(buffer);
        string content = System.Text.Encoding.UTF8.GetString(buffer);
        Console.WriteLine($"[C#] - Response from server:\n{content}");    
    }
}