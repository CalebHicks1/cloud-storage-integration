using Newtonsoft.Json;

namespace Types;

/// <summary>
/// Functions needed to satisfy API requests and debug
/// </summary>
public abstract class APIClient
{
    /// <summary>
    /// If true, logs status messages to STDERR
    /// </summary>
    private bool _debug;

    /// <summary>
    /// lists all files in a directory given by the <paramref name="path"/> field.
    /// </summary>
    /// <param name="path"></param>
    /// <returns></returns>
    public abstract Task<File[]> ListFilesInDirAsync(string path);

    /// <summary>
    /// uploads files given by <paramref name="srcPaths"/> to a directory given by the <paramref name="dstPath"/> field. If the file already exists, overwrite the file.
    /// </summary>
    /// <param name="dstPath"></param>
    /// <param name="srcPaths"></param>
    /// <returns></returns>
    public abstract Task UploadFilesToDirAsync(string[] srcPaths, string dstPath);

    /// <summary>
    /// downloads the files from the API given by <paramref name="srcPaths"/> (should be paths in the API not your local machine) in the file array to a local directory given by <paramref name="dstPath"/>.
    /// </summary>
    /// <param name="dstPath"></param>
    /// <param name="srcPaths"></param>
    public abstract Task DownloadFilesToDirAsync(string[] srcPaths, string dstPath);

    /// <summary>
    /// deletes files/folders (folders recursively delete) given by the entries in <paramref name="paths"/>.
    /// </summary>
    /// <param name="paths"></param>
    public abstract Task DeleteFilesAsync(string[] paths);

    /// <summary>
    /// Causes APIClient instance to serve requests on STDIN
    /// </summary>
    /// <param name="debug">when debug=true, additional info is printed to STDERR</param>
    public async Task ServeAsync(bool debug)
    {
        try
        {
            // Set debug level
            _debug = debug;

            // Begin serving requests
            Log("Starting to serve...");
            while (true)
            {
                // read and check if pipe has been closed
                string? line = Console.In.ReadLine();
                if (line == null)
                {
                    // EOF error
                    RespondError(ErrorCode.EOF, "No input from STDIN");
                    continue;
                }

                // parse command
                var command = JsonConvert.DeserializeObject<Command>(line);
                if (command == null)
                {
                    // INVALID_INPUT error
                    RespondError(ErrorCode.InvalidInput, $"Could not parse input '{line}'");
                    continue;
                }

                // Execute command
                Log($"Received command '{command}'");
                try
                {
                    // Note that we ignore possibly null values because it will be caught by the catch block
                    switch (command.Type)
                    {
                        case "list":
                            var files = await ListFilesInDirAsync(command.Path!);

                            var output = JsonConvert.SerializeObject(files);
                            
                            Console.Out.WriteLine(output);
                            
                            Log($"Read files from '{command.Path!}' successfully");
                            break;
                        case "upload":
                            await UploadFilesToDirAsync(command.Files!, command.Path!);
                            
                            RespondError(ErrorCode.NoError, $"File(s) '{string.Join(", ", command.Files!)}' uploaded successfully to '{command.Path}'");
                            break;
                        case "download":
                            await DownloadFilesToDirAsync(command.Files!, command.Path!);
                            
                            RespondError(ErrorCode.NoError, $"File(s) '{string.Join(", ", command.Files!)}' downloaded successfully to '{command.Path}'");
                            break;
                        case "delete":
                            await DeleteFilesAsync(command.Files!);
                            
                            RespondError(ErrorCode.NoError, $"File(s) '{string.Join(", ", command.Files!)}' deleted successfully");
                            break;
                        case "shutdown":
                            RespondError(ErrorCode.NoError, $"Client '{ToString()}' shutting down");
                            return;
                        default:
                            // INVALID_COMMAND error
                            RespondError(ErrorCode.InvalidCommand, $"Unrecognized command '{command}'");
                            continue;
                    }
                }
                catch (Exception e)
                {
                    // COMMAND_FAILED error
                    RespondError(ErrorCode.CommandFailed, $"Command '{command}' failed due to error '{e}'");
                    continue;
                }
            }
        }
        catch (Exception e)
        {
            // CLIENT_FAILED error
            RespondError(ErrorCode.ClientFailed, $"Client failed due to error '{e}'");
            return;
        }
    }

    /// <summary>
    /// Sends an error code to STDOUT
    /// </summary>
    /// <param name="code"></param>
    /// <param name="message"></param>
    private void RespondError(ErrorCode code, string message)
    {
        if (code == ErrorCode.NoError)
        {
            Log(message);
        }
        else
        {
            Log($"{code} error occured with message '{message}'", true);
        }

        var error = new APIError
        {
            Code = code,
            Message = message
        };

        var output = JsonConvert.SerializeObject(error);

        Console.Out.WriteLine(output);
    }

    /// <summary>
    /// Prints <paramref name="message"/> to STDERR if <see cref="_debug"/> is set
    /// </summary>
    /// <param name="message"></param>
    public void Log(string message, bool error = false)
    {
        if (_debug)
        {
            Console.Error.WriteLine($"[{ToString()}] {(error ? "ERROR:" : "INFO:")} {message}");
        }
    }
}
