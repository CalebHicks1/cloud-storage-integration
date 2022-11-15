using Newtonsoft.Json;

namespace Types;

/// <summary>
/// Represnts the input JSON to the API clients
/// </summary>
public class Command
{
    [JsonProperty("command")]
    public string Type { get; set; } = null!;

    [JsonProperty("path")]
    public string? Path { get; set; }

    [JsonProperty("files")]
    public string[]? Files { get; set; }
}