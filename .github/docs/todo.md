## Downloading on build, not on run


in `bridge.go`

```go
//go:generate GPP_FETCH_TAG=v0.1.0 go run ./cmd/fetch-prebuilt
```

in `cmd/fetch-prebuilt/main.go`

```go
package main
func main(){ if err:=fetchPrebuilt(getTag()); err!=nil{ panic(err) } }

# put the fetch code here
```

Then

```bash
go generate ./bridge
go build ./...
```
