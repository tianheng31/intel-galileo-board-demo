var http = require("http");
var dgram = require("dgram");
var server = dgram.createSocket("udp4");
var socket = dgram.createSocket("udp4");

socket.bind(function() {
    socket.setBroadcast(true);
});

var harbin = new Buffer(33);
var beijing = new Buffer(33);

http.get('http://www.weather.com.cn/data/sk/101050101.html',
function(res) {
    var json = '';
    res.on('data',
    function(d) {
        json += d;
    });
    res.on('end',
    function() {
        // 收到的json数据
        json = JSON.parse(json);
        harbin.write("Harbin temp is  " + json["weatherinfo"]["temp"] + "               ");
        console.log(json["weatherinfo"]["temp"]);
    });
});

http.get('http://www.weather.com.cn/data/sk/101010100.html',
function(res) {
    var json = '';
    res.on('data',
    function(d) {
        json += d;
    });
    res.on('end',
    function() {
        // 收到的json数据
        json = JSON.parse(json);
        beijing.write("Beijing temp is " + json["weatherinfo"]["temp"] + "              ");
        console.log(json["weatherinfo"]["temp"]);
    });
});

server.on("error",
function(err) {
    console.log("server error:\n" + err.stack);
    server.close();
});

server.on("message",
function(msg, rinfo) {
    console.log("server got: " + msg + " from " + rinfo.address + ":" + rinfo.port);
    if (msg == "101050101") socket.send(harbin, 0, harbin.length, 2000, '127.0.0.1',
    function(err, bytes) {
        //socket.close();
    });

    if (msg == "101010100") socket.send(beijing, 0, beijing.length, 2000, '127.0.0.1',
    function(err, bytes) {
        //socket.close();
    });

}).on('error',
function(e) {
    console.error(e);
});

server.on("listening",
function() {
    var address = server.address();
    console.log("server listening " + address.address + ":" + address.port);
});

server.bind(2010);