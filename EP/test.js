
var ScanReader = require('./reader').ScanReader;
var async = require('async');
require('events').EventEmitter.prototype._maxListeners = 100;
var util = require('util');
var Utils = require('./utils');
var fs = require('fs');
var Q = require('q');
var deepcopy = require("deepcopy");
var Client = require('./client').Client;
var request = require('request');
var prompt = require('prompt');

var HOST = '127.0.0.1'
var PORT = 9000
var LOCKED = []
Records = require('./event.json').Records

var CLIENT = new Client(HOST, PORT)
var PATH = 'tests/%s'
var EPPATH = '/home/kuldeeprishi/projects/Re-bar/expresso/tests/%s'
var OUTPUT_KEYS = ['idx', 'st', 'src', 'dst', 'conn', 'rmk']

PACKAGE_URI = 'https://hq.delhivery.com/api/p/info/%s/.json'
HEADERS = {
    'Authorization':'Token 1113c14cce8e165a763777257d8cf6652b448f0b'
}


var sortObjectByKey = function(obj){
    var keys = [];
    var sorted_obj = {};

    for(var key in obj){
        if(obj.hasOwnProperty(key)){
            keys.push(key);
        }
    }

    // sort keys
    keys.sort();

    // create new array based on Sorted Keys
    keys.forEach( function(key, index) {
        sorted_obj[key] = obj[key]
    });
    return sorted_obj;
};


function getKinesisData(pkgs){
    var data = []
    pkgs.forEach( function(pkg, index) {
        pkg['s'].forEach( function(scan, index) {
            var p = deepcopy(pkg)
            delete p['s']
            delete p['cs']
            p['cs'] = scan
            data.push(p)
        });

    });
    return data
}


function loadData(wbn){
    var deferred = Q.defer()
    var options = {
      url: util.format(PACKAGE_URI, wbn),
      headers: HEADERS
    };

    function cb(error, response, body) {
      if (!error && response.statusCode == 200) {
        var info = JSON.parse(body);
        data = getKinesisData(info)
        deferred.resolve(data)
      } else {
        console.log('ERROR: '+ err)
        deferred.fail([])
      }
    }

    request(options, cb);

    return deferred.promise
}


function generateGraph(records) {
    var deferred = Q.defer()
    async.eachSeries(records, function(record, callbackFromForEach){
        // var record = Utils.decode_record(record)
        var wbn = record['wbn']
        if (LOCKED.indexOf(wbn) >= 0){
            callbackFromForEach()
        } else {
            // if (Utils.isReturnPackage(record)){
            //     callbackFromForEach()
            // } else {
            var scanner = new ScanReader(CLIENT, HOST, PORT, true)
            result = scanner.read(record)
            result.then(function(res){
                callbackFromForEach()
            })
            // }
        }
    }, function(){
        deferred.resolve(true)
    })
    return deferred.promise
}


function readOutput (wbn) {
    // load node data
    try {
        var data = fs.readFileSync(util.format(PATH, wbn))
        var data_string = data.toString()
        data = JSON.parse(data_string)
    } catch(err) {
        data = []
    }


    try {
        var expdata = fs.readFileSync(util.format(EPPATH, wbn))
        var exp_data_string = expdata.toString()
        expdata = JSON.parse(exp_data_string)
    } catch(err) {
        expdata = []
    }

    var datalen = data.length
    var expdatalen = expdata.length
    var len = datalen > expdatalen ? datalen : expdatalen


    console.log('MATCH\t'+OUTPUT_KEYS.join('\t'))

    for (var i = 0; i < len; i++){
        var out = ''

        try {
            var match = JSON.stringify(sortObjectByKey(data[i][0])) === JSON.stringify(sortObjectByKey(expdata[i][0]))
        } catch(e) {
            match = false
        }

        out += util.format('%s\t', match)
        OUTPUT_KEYS.forEach( function(key, index) {
            out += util.format('%s\t', data[i][key])
        });
        console.log(out)
    }

    if (datalen !== expdatalen){
        console.log(util.format('FAIL: OUTPUT LENGTH MISMATCH %s != %s', datalen, expdatalen))
    } else {
        console.log('SUCCESS')
    }
    process.exit()
}


function test (wbn){
    var deferred = Q.defer()
    var path = util.format(PATH, wbn)
    try {
        fs.accessSync(path)
        fs.unlinkSync(path)
    } catch(e) {
        console.log('')
    }

    load = loadData(wbn)
    load.then(function(records){
        // console.log('LEN of Records '+ records.length)
        graph = generateGraph(records)
        graph.then(function (response) {
            deferred.resolve(true)
        })
    })

    return deferred.promise
}


prompt.start();

prompt.get(['waybill'], function (err, result) {
    console.log('  waybill: ' + result.waybill);
    testresult = test(result.waybill)
    testresult.then(function () {
        readOutput(result.waybill)
    })
});

// var wbn = 195087939466
// testresult = test(wbn)
// testresult.then(function () {
//     readOutput(wbn)
// })


