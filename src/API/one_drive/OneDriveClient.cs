using Azure.Core;
using Azure.Identity;
using Microsoft.Graph;
using Types;

namespace OneDrive;

public class OneDriveClient : APIClient
{
    private const string TOKEN_CACHE_NAME = "OneDriveTokenCache";
    private const string AUTH_RECORD_PATH = "token.json";
    private GraphServiceClient _client;

    public OneDriveClient(OneDriveConfig config)
    {
        // Adapted from: https://learn.microsoft.com/en-us/dotnet/api/azure.identity.tokencachepersistenceoptions?view=azure-dotnet
        var options = new InteractiveBrowserCredentialOptions
        {
            ClientId = config.ClientId,
            TenantId = config.TenantId,
            RedirectUri = new Uri(config.RedirectUri),
            TokenCachePersistenceOptions = new TokenCachePersistenceOptions
            {
                Name = TOKEN_CACHE_NAME,
                UnsafeAllowUnencryptedStorage = true
            }
        };

        // Check if an AuthenticationRecord exists on disk.
        AuthenticationRecord authRecord;
        InteractiveBrowserCredential credential;
        if (System.IO.File.Exists(AUTH_RECORD_PATH))
        {
            // If it does exist, load it from disk and deserialize it.
            using var authRecordStream = new FileStream(AUTH_RECORD_PATH, FileMode.Open, FileAccess.Read);
            authRecord = AuthenticationRecord.Deserialize(authRecordStream);
            options.AuthenticationRecord = authRecord;
            credential = new InteractiveBrowserCredential(options);
        }
        else
        {
            // Otherwise, get one and serialize it to disk
            credential = new InteractiveBrowserCredential(options);
            authRecord = credential.Authenticate(new TokenRequestContext(config.Scopes));
            using var authRecordStream = new FileStream(AUTH_RECORD_PATH, FileMode.Create, FileAccess.Write);
            authRecord.Serialize(authRecordStream);
        }

        _client = new GraphServiceClient(credential);
    }

    public override async Task DeleteFilesAsync(string[] paths)
    {
        foreach (var path in paths)
        {
            // TODO: Should we support deleting the root directory? I feel like this is a bad idea
            if (PathIsRoot(path)) throw new InvalidOperationException("You cannot delete the root directory.");

            await _client.Me.Drive.Root.ItemWithPath(path).Request().DeleteAsync();
        }
    }

    public override async Task DownloadFilesToDirAsync(string[] srcPaths, string dstPath)
    {
        foreach (var path in srcPaths)
        {
            // Get the download URL. This URL is preauthenticated and has a short TTL.
            var item = await _client.Me.Drive.Root.ItemWithPath(path).Request().GetAsync();
            item.AdditionalData.TryGetValue("@microsoft.graph.downloadUrl", out var downloadUrlObj);
            string? downloadUrl = downloadUrlObj?.ToString();

            if (downloadUrl is null) throw new InvalidOperationException($"Item with path '{path}' doesn't have a download url for some reason.");
            if (ItemIsDirectory(item)) throw new InvalidOperationException($"Item with path '{path}' is a directory and downloading directories is not supported.");

            // Create a file stream to contain the downloaded file.
            using var fileStream = System.IO.File.Create(Path.Combine(dstPath, item.Name));

            // Download the file content
            var req = new HttpRequestMessage(HttpMethod.Get, downloadUrl);
            var response = await _client.HttpProvider.SendAsync(req);

            // Copy into destination
            // NOTE: This will fail for very large files... I have not pinpointed the exact size though
            await response.Content.CopyToAsync(fileStream);
        }
    }

    public override async Task<Types.File[]> ListFilesInDirAsync(string path)
    {
        // TODO: Error Handling?
        // If path points to an API file it just returns an empty array
        // If path is invalid then it causes a timeout error
        var request = PathIsRoot(path) ?
            _client.Me.Drive.Root.Children.Request() :
            _client.Me.Drive.Root.ItemWithPath(path).Children.Request();

        var children = await request.GetAsync();

        var result = children.Select(c => new Types.File
        {
            Name = c.Name,
            IsDir = ItemIsDirectory(c),
            Size = (ulong)(c.Size ?? 0),
            Modified = c.LastModifiedDateTime?.ToUnixTimeSeconds() ?? 0
        });

        return result.ToArray();
    }

    public override async Task UploadFilesToDirAsync(string[] srcPaths, string dstPath)
    {
        foreach (var path in srcPaths)
        {
            // Following code based on https://learn.microsoft.com/en-us/graph/sdks/large-file-upload?tabs=csharp
            using var fileStream = System.IO.File.OpenRead(path);

            // Use properties to specify the conflict behavior
            // in this case, replace
            var uploadProps = new DriveItemUploadableProperties
            {
                AdditionalData = new Dictionary<string, object>
                {
                    { "@microsoft.graph.conflictBehavior", "replace" }
                }
            };

            // Create the upload session
            var apiPath = Path.Combine(dstPath, Path.GetFileName(path));
            var uploadSession = await _client.Me.Drive.Root
                .ItemWithPath(apiPath)
                .CreateUploadSession(uploadProps)
                .Request()
                .PostAsync();

            // Max slice size must be a multiple of 320 KiB
            int maxSliceSize = 320 * 1024;
            var fileUploadTask = new LargeFileUploadTask<DriveItem>(uploadSession, fileStream, maxSliceSize);

            var totalLength = fileStream.Length;
            // Create a callback that is invoked after each slice is uploaded
            IProgress<long> progress = new Progress<long>(prog =>
            {
                Log($"Uploaded {prog} bytes of {totalLength} bytes");
            });

            // Upload the file
            var uploadResult = await fileUploadTask.UploadAsync(progress);

            Log(uploadResult.UploadSucceeded ?
                $"Upload complete, item ID: {uploadResult.ItemResponse.Id}" :
                "Upload failed");
        }
    }

    private bool PathIsRoot(string path) => path == string.Empty || path == "/";

    private bool ItemIsDirectory(DriveItem item) => item.Folder != null;

    public override string ToString()
    {
        return "OneDriveClient";
    }
}

