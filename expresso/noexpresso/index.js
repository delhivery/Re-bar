/**
 * Main Lambda function
 *
 * @module index
 */


require('buffer');
require('events').EventEmitter.prototype._maxListeners = 200;

var async = require('async');
var Client = require('./client').Client;
var ScanReader = require('./reader').ScanReader;
var Utils = require('./utils');
var pushToKinesis = require('./kinesis').pushToKinesis;
var config = require('./config.json')


Object.keys(config).forEach( function(config_key, index) {
    if (config[config_key] === "") {
        var err = 'Configuration Not Defined for: '+ config_key
        console.log(err)
        process.exit()
    }
});


var CLIENT = new Client(config.HOST, config.PORT)


/**
 * handlePayload from kinesis
 *
 * @method handlePayload
 * @return null
 */
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
        console.log('Done')
        context.done(null, response)
    })
};

