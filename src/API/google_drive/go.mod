module google_drive

go 1.18

require (
	github.com/gabriel-vasile/mimetype v1.4.1
	golang.org/x/oauth2 v0.0.0-20220822191816-0ebed06d0094
	google.golang.org/api v0.95.0
)

require (
	golang.org/x/crypto v0.0.0-20200622213623-75b288015ac9 // indirect
	gopkg.in/check.v1 v1.0.0-20180628173108-788fd7840127 // indirect
	types v1.0.0
)

replace types v1.0.0 => ../types
