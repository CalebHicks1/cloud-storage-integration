OneDrive Notes:
 - [Authentication](https://learn.microsoft.com/en-us/graph/auth/auth-concepts)
   - Need a token from MS Identity to work
      - Must be registered in the [Azure portal](https://portal.azure.com/)
         - Google Drive test user has it registered
            - App ID: c1431d98-37e1-4b6e-8fc4-27213365500e
            - Redirect URI: https://login.microsoftonline.com/common/oauth2/nativeclient
            - Client Secret (Expires in 12 months): W.Q8Q~rhe761PzgBr~6xzzzsL2T08w1zQ5hR2aYi
         - Currently has Files.ReadWrite.All and User.Read permissions
            - The logging in user has to consent I think


The [MSGraph SDK](https://github.com/microsoftgraph/msgraph-sdk-go) uses xdg-open to open the
link. xdg-open might need some configuration (it did on my WSL install):
- apt install xdg-utils
- export BROWSER="echo"
   - The BROWSER environment variable is the name of the program to use to open the link that
   the library generates. I use echo so that the link is output to the command line, but if you
   are using a GUI then set it to something else or even skip this step to use the default
   browser

It appears that the user will have to log in every time that they use the library...
It doesn't save the token because offline_access permission is not working


cloudstoragetest94@outlook.com
myTestUserPassword