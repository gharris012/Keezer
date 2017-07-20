
/*
 * setup:
 * npm install express body-parser socket.io nconf supervisor
 *
 * run:
 * supervisor -w buildserver.js buildserver.js
 *
 */


var express = require('express');
var bodyParser = require('body-parser');
var app = express();
var server = require('http').createServer(app);
var io = require('socket.io')(server);
var assert = require('assert');
var fs = require('fs');

var nconf = require('nconf');

nconf.file(__dirname + '\\mockserver.config.json');
console.log("Home: " + __dirname)

nconf.defaults({
    "name":"mock",
    "wifi_ssid":"",
    "wifi_password":"",
    "min":32,
    "max":80,
    "target":38,
    "min_on":5,
    "min_off":5,
    "threshold":2,
    "state":0,
    "server":{
        "baseurl":"http://localhost:3001",
        "port":3001
    }
});

app.use(express.static(__dirname));

app.use(bodyParser.json());

app.route('/appconfig').all(function(req, res) {
    console.log(req.body);
});

app.route('/netconfig').all(function(req, res) {
    console.log(req.body);
});

app.route('/config').get(function(req, res) {
    res.json(nconf.get());
});

app.route('/').get(function (req, res)
{
    res.send("<html><body><h1>Hello!</h1><p>Sincerely,<br>" + nconf.get('name') + "</p></body></html>");
});

app.listen(nconf.get('server').port);
