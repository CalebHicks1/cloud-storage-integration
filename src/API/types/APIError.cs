using Newtonsoft.Json;

namespace Types;

/// <summary>
/// API client error codes
/// </summary>
public enum ErrorCode
{
    /// <summary>
    /// Command succeeded.
    /// </summary>
    NoError,
    /// <summary>
    /// General error that tells the parent that the underlying API client failed.
    /// </summary>
    ClientFailed,
    /// <summary>
    /// General command related error that tells the parent that the command failed.
    /// </summary>
    CommandFailed,
    /// <summary>
    /// Tells the parent that the value of the command key is invalid.
    /// </summary>
    InvalidCommand,
    /// <summary>
    /// Tells the parent that the input passed to STDIN couldn't be parsed.
    /// </summary>
    InvalidInput,
    /// <summary>
    /// Tells the parent that an invalid file was given when trying to upload or download.
    /// </summary>
    FileNotFound,
    /// <summary>
    /// Tells the parent that the input pipe was broken/closed before a shutdown command.
    /// </summary>
    EOF
}

/// <summary>
/// Contains a message describing the error and an ErrorCode for the user
/// </summary>
public class APIError
{
    [JsonProperty("code")]
    public ErrorCode Code { get; set; }

    [JsonProperty("message")]
    public string Message { get; set; } = null!;
}
