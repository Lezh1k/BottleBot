from golang:1.11
workdir /go/src/github.com/Lezh1k/BottleBot/bot
copy . .
run go list -f {{.Deps}} | sed 's/[][]//g' > dependencies

from golang:1.11
label maintainer="rock@maddevs.io"
workdir /go
copy --from=0 /go/src/github.com/Lezh1k/BottleBot/bot/dependencies .
run go get $(cat dependencies)
workdir /go/src/github.com/Lezh1k/BottleBot/bot
copy . .
run make install_dependencies
run make
cmd ["./bottle-bot"]

