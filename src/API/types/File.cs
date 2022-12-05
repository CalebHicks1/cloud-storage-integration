namespace Types;

/// <summary>
/// The valid file output for the API clients
/// </summary>
public class File
{
    // The null! silences nullable type warnings
    public string Name { get; set; } = null!;
    public bool IsDir { get; set; }
    public ulong Size { get; set; }
    public long Modified { get; set; }
}