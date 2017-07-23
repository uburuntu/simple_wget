# Simple wget
Simple file downloader as test task

## How to build
* Install [curl](https://curl.haxx.se/) and [OpenSSL](https://www.openssl.org/)
* Run ``make`` or compile manually:
```sh
g++ -std=c++14 main.cpp -lcurl -lssl -lcrypto -o simple_wget
```

## How to use
```sh
./simple_wget <list of URLs>
```
For example:
```sh
./simple_wget https://www.openssl.org/source/openssl-1.0.2l.tar.gz https://vk.com/images/stickers/56/512.png
```
