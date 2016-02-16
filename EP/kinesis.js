var Q = require('q');
var AWS = require('aws-sdk');
var config = require('./config.json')

AWS.config.update({
        accessKeyId: config.AWS_ACCESS_KEY_ID,
        secretAccessKey: config.AWS_SECRET_ACCESS_KEY,
        region: config.AWS_REGION
});


var kinesis = new AWS.Kinesis({apiVersion: '2013-12-02'});


var pushToKinesis = function(data){
    var deferred = Q.defer()
    var params = {
      'Data': JSON.stringify(data),
      'PartitionKey': 'wbn',
      'StreamName': config.KINESIS_STREAM,
    };

    kinesis.putRecord(params, function(err, data) {
        if (err){
            console.error(err, err.stack);
            deferred.resolve(false)
        }
        else{
            deferred.resolve(true)
        }
    });

    return deferred.promise;

}


exports.pushToKinesis = pushToKinesis;


