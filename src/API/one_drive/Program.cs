//using Newtonsoft.Json;
using OneDrive;

// Commented this out because it was problematic to find the config.json
//const string CONFIG_FILENAME = "config.json";

// // Search for config file
// var currentDir = Directory.GetCurrentDirectory();
// var configPath = Path.Combine(currentDir, CONFIG_FILENAME);
// if (!File.Exists(configPath)) throw new ApplicationException($"Could not find configuration file '{CONFIG_FILENAME}' in '{currentDir}'.");

// // If found, read and parse
// var content = File.ReadAllText(configPath);
// var config = JsonConvert.DeserializeObject<OneDriveConfig>(content);
// if (config == null) throw new ApplicationException($"Could not parse configuration file '{configPath}'.");

// New config init
var config = new OneDriveConfig
{
    Scopes = new string[] {
        "User.Read",
        "Files.ReadWrite.All"
    },
    ClientId = "47eb8dd1-1d88-4906-bd34-5dc476ec70d4",
    TenantId = "consumers",
    RedirectUri = "http://localhost/",
    Debug = true
};

// Init client
var client = new OneDriveClient(config);
await client.ServeAsync(config.Debug);