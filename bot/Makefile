TARGET=bottle-bot

all: fmt clean build

clean:
	rm -rf $(TARGET)
	rm -rf *.prof
	rm -rf c.out
fmt:
	go fmt ./...

generate_version:
	rm -rf version
	git describe > version

LDFLAGS="-X 'main.BuildTime=$(shell date)'"

build:
	go build -o $(TARGET) -ldflags=$(LDFLAGS) main.go

pry_build:
	${GOPATH}/bin/go-pry build -o $(TARGET) -ldflags=$(LDFLAGS) main.go

run: clean build launch 

launch:
	./$(TARGET)

build_linux:
	GOOS=linux GOARCH=amd64 go build -o $(TARGET) -ldflags=$(LDFLAGS) main.go

install_dependencies:
	go get -v ./...

install_go_tools:
	go get github.com/stretchr/testify
	go get honnef.co/go/tools/cmd/unused
	go get -u golang.org/x/lint/golint
	go get -u github.com/kisielk/errcheck

test:
	go test -coverprofile=c.out ./...

lint:
	golint $(shell go list ./... | grep -v /vendor)
	go vet $(shell go list ./... | grep -v /vendor)
	unused $(shell go list ./... | grep -v /vendor)
	errcheck -ignorepkg vendor/.*
