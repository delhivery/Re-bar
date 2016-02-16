require('buffer');
var async = require('async');
require('events').EventEmitter.prototype._maxListeners = 100;
var Client = require('./client').Client;
var Parser = require('./parser').Parser;
var ScanReader = require('./reader').ScanReader;
var Utils = require('./utils');
var util = require('util')
var pushToKinesis = require('./kinesis').pushToKinesis;
var config = require('./config.json')


Object.keys(config).forEach( function(config_key, index) {
    if (config[config_key] === "") {
        var err = 'Configuration Not Defined for: '+ config_key
        console.log(err)
        process.exit()
    }
});

MODES = ['RCSP', 'STSP']
VERTEX_NAME_CODE_MAPPING = require('./mapper.json')


var CLIENT = new Client(config.HOST, config.PORT)
var RECORDS_TO_PROCESS = []
var LOCKED = []


function lockwbn (record) {
    var wbn = record['wbn']
    // console.log('Locking wbn '+wbn)
    if (LOCKED.indexOf(wbn) < 0){
        LOCKED.push(wbn)
    }
}


function unlockwbn (record) {
    var wbn = record['wbn']
    // console.log('UnLocking wbn '+wbn)
    if (LOCKED.indexOf(wbn) >= 0){
        LOCKED.pop(wbn)
    }
    if (RECORDS_TO_PROCESS.indexOf(record) >= 0){
        RECORDS_TO_PROCESS.pop(record)
    }
}

function isLocked (record) {
    var wbn = record['wbn']
    return LOCKED.indexOf(wbn) >= 0
}


function processScan (record, callback) {
    var scanner = new ScanReader(CLIENT, config.HOST, config.PORT, true)
    result = scanner.read(record)
    result.then(function(res){
        unlockwbn(record)
        callback()
    })
}

// handles data recieved from kinesis
function handlePayload(records, callback) {
    async.eachSeries(records, function(record, callbackFromForEach){
        var record = Utils.decode_record(record)
        var wbn = record['wbn']
        if (Utils.isReturnPackage(record)){
            callbackFromForEach()
        } else {
            console.log('Processing '+ wbn)
            var scanner = new ScanReader(CLIENT, config.HOST, config.PORT, true)
            result = scanner.read(record)
            result.then(function(res){
                callbackFromForEach()
            })
        }
    }, function(){
        callback('Done');
    })
};


// main handler for this lambda function
exports.handler = function(event, context) {
    handlePayload(event.Records, function(response){
        console.log('Done exiting lambda')
        context.done(null, response)
    })
};

