
/*
 * setup:
 * npm install express nconf express-busboy
 * npm install supervisor -g
 *
 * run:
 * supervisor -w mockserver.js mockserver.js
 *
 */


var express = require('express');
var busboy = require('express-busboy');
var app = express();

busboy.extend(app);

var nconf = require('nconf');

nconf.file(__dirname + '\\config.json')
     .file('server', __dirname + '\\mockserver.config.json');
console.log("Home: " + __dirname);

//var bob = nconf.get();
//console.log(JSON.stringify(bob));

nconf.defaults({
    "wifi_host":"mock",
    "wifi_ssid":"",
    "wifi_password":"",
    "min_temp":30,
    "max_temp":80,
    "target_temp":40,
    "min_on":5,
    "min_off":5,
    "threshold":2,
    "server":{
        "name":"Mock",
        "baseurl":"http://localhost:3001",
        "port":3001
    }
});

app.use(express.static(__dirname));

//app.use(bodyParser.json());

app.route('/appconfig').all(function(req, res) {
    console.log(req);
    console.log(req.body);
});

app.route('/netconfig').all(function(req, res) {
    console.log(req.body);
});

app.route('/config').get(function(req, res) {
    res.json(nconf.get());
});

app.route('/hello').get(function (req, res)
{
    res.send("<html><body><h1>Hello!</h1><p>Sincerely,<br>" + nconf.get('server').name + "</p></body></html>");
});

app.listen(nconf.get('server').port);
