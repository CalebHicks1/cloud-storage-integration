# OneDrive Client
It is written in C# and uses the [msgraph sdk](https://github.com/microsoftgraph/msgraph-sdk-dotnet)

# Description of Files
- `old_go_client_dont_use`
    - This is the old client that I tried to write in Go
    - Don't use it, it is here solely for archival purposes
- `old.json`
    - Supposed to be a configuration file. I was having trouble getting the client to read from it though so in interest of time I just hardcoded the configuration details in the program. To look at any of the configuration parameters, look into Program.cs
- `*.cs` and `.csproj`
    - The actual code. Note that it depends on code in the types folder. You shouldn't have to worry about building the types code though, the compiler should find it

# Building and Running
There is a Makefile with several methods
- To build:
    - `make`
    - This builds the client but doesn't run it. You can then run it by doing `dotnet run` in the same directory as `OneDrive.csproj`
- To run:
    - `make run`
    - Builds and runs
- To clean:
    - `make clean`
    - Building the code makes several directories with the binary and the debug info. This removes them from the one_drive and types directories 

