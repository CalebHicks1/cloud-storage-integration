package main

import (
	"fmt"
	"log"
	"os"
	"types"

	"context"

	azidentity "github.com/Azure/azure-sdk-for-go/sdk/azidentity"
	a "github.com/microsoft/kiota-authentication-azure-go"
	msgraphsdk "github.com/microsoftgraph/msgraph-sdk-go"
)

type OneDriveClient struct {
	types.APIClient
	Srv  *msgraphsdk.GraphServiceClient
	Name string
}

func main() {
	// Set the mechanism for handling auth link
	// TODO: Figure out ideal handler
	os.Setenv("BROWSER", "echo")

	// input := bufio.NewScanner(os.Stdin)

	// fmt.Println("Creating credential")
	// input.Scan()

	cred, err := azidentity.NewInteractiveBrowserCredential(&azidentity.InteractiveBrowserCredentialOptions{
		TenantID: "consumers",
		ClientID: "47eb8dd1-1d88-4906-bd34-5dc476ec70d4",
		//RedirectURL: "http://localhost:8080",
	})

	if err != nil {
		log.Fatal(err)
	}

	// token, err := cred.GetToken(context.Background(), policy.TokenRequestOptions{
	// 	Scopes: []string{"User.Read", "Files.ReadWrite.All"},
	// })

	// fmt.Printf("%#v\n", token)

	// if err != nil {
	// 	log.Fatal(err)
	// }

	// fmt.Println("Creating auth provider from credential")
	// input.Scan()

	auth, err := a.NewAzureIdentityAuthenticationProviderWithScopes(cred, []string{"User.Read", "Files.ReadWrite.All"})

	if err != nil {
		log.Fatal(err)
	}

	// fmt.Println("Creating request adapter with auth")
	// input.Scan()

	//auth.GetAuthorizationTokenProvider().GetAuthorizationToken()
	adapter, err := msgraphsdk.NewGraphRequestAdapter(auth)

	if err != nil {
		log.Fatal(err)
	}

	// fmt.Println("Creating client with request adapter")
	// input.Scan()

	client := msgraphsdk.NewGraphServiceClient(adapter)

	// fmt.Println("Making request")
	// input.Scan()

	// Make the actual request
	//query := drive.DriveRequestBuilder
	//query :=

	// reqConfig := drive.DriveRequestBuilderGetRequestConfiguration{
	// 	QueryParameters: &drive.DriveRequestBuilderGetQueryParameters{
	// 		//Expand: []string{"items"},
	// 	},
	// }

	//request := drive.NewDriveRequestBuilder("", adapter)

	//info, err := request.CreateGetRequestInformation(context.Background(), &reqConfig)

	if err != nil {
		log.Fatal(err)
	}

	//info.
	ctx := context.Background()

	drive, err := client.Me().Drive().Get(ctx, nil)

	if err != nil {

	}

	id := *drive.GetId()

	result, err := client.DrivesById(id).Items().Get(ctx, nil)

	//result, err := client.Me().Drive().Get(context.Background(), &reqConfig)

	if err != nil {
		log.Fatal(err)
	}

	//for item := result.GetItems()

	fmt.Printf("Found data : %#v\n", result)

	// if err != nil {
	// 	log.Fatal("Error creating credentials: %v\n", err)
	// }

	// fmt.Println("Success! Cred is %v", cred)

}
