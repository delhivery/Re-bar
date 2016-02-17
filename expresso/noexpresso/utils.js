/**
 * Utitlity functions for various components of NoExpresso
 *
 * @module utils
 */

var AWS = require('aws-sdk');
var Q = require('q');
var fs = require('fs');
var uuid = require('node-uuid');
var config = require('./config.json')

var VERTEX_NAME_CODE_MAPPING = require('./mapper.json')


/**
 * Base Class
 *
 * @class Class
 */
var Class = function(methods) {
    var klass = function() {
        this.initialize.apply(this, arguments);
    };

    for (var property in methods) {
       klass.prototype[property] = methods[property];
    }

    if (!klass.prototype.initialize) klass.prototype.initialize = function(){};

    return klass;
};


var INV_MAPPING = (function(){
    var _INV_MAPPING = {}
    for (var key in VERTEX_NAME_CODE_MAPPING){
        _INV_MAPPING[VERTEX_NAME_CODE_MAPPING[key]] = key
    }
    return _INV_MAPPING
})()


AWS.config.update({
        accessKeyId: config.AWS_ACCESS_KEY_ID,
        secretAccessKey: config.AWS_SECRET_ACCESS_KEY,
        region: config.AWS_REGION
});

var s3 = new AWS.S3();

/**
 * Convert an ISO datetime string to seconds since epoch
 *
 * @method iso_to_seconds
 */
var iso_to_seconds = function(iso_string){
    return parseInt( new Date(iso_string).getTime()/1000)
}


/**
 * Convert a path to a segment
 *
 * @method mod_path
 */
var mod_path = function(path, start_time, kwargs){
    var segments = [];

    path.forEach(function(path_segment){

        var departure = path_segment['departure_from_source']
        if (departure < 1000000) {
            departure = departure + start_time
        }

        var arrival = path_segment['arrival_at_source']

        if (arrival < 1000000) {
            arrival = arrival + start_time
        }

        var source = path_segment['source']
        var destination = path_segment['destination']
        var connection = path_segment['connection']

        var cost = path_segment['cost_reaching_source']
        var segment = {
            'src': source,
            'dst': destination,
            'conn': connection,
            'p_arr': arrival,
            'p_dep': departure,
            'cst': cost,
        }

        for (var key in kwargs){
            segment[key] = kwargs[key]
        }
        segments.push(segment)
    })
    return segments
}

/**
 * Convert a center name to center code
 *
 * @method center_name_to_code
 */
var center_name_to_code = function(cn_unicode){
    var name = cn_unicode
    if (cn_unicode != undefined && cn_unicode.indexOf(' (') > 0){
        name = cn_unicode.split(' (')[0]
    }
    return VERTEX_NAME_CODE_MAPPING[name] || null
}

/**
 * Convert center codes to user readeable format
 *
 * @method pretty
 */
var pretty = function(data){
    console.log(data)
}

/**
 * compares segments on basis of index (this as function to sort method)
 *
 * @method compare
 */
var compare = function(a,b) {
  if (a.idx < b.idx)
    return -1;
  else if (a.idx > b.idx)
    return 1;
  else
    return 0;
}

/**
 * Check if package is in return flow
 *
 * @method isReturnPackage
 */
function isReturnPackage(record){
    var cs = record['cs']
    if (['RT', 'PU', 'PP'].indexOf(cs['st']) >= 0 ){
        return true
    }
    if (['RTO', 'DTO'].indexOf(cs['ss']) >= 0){
        return true
    }
    return false
}

/**
 * decode kinesis encoded payload to json
 *
 * @method decode_record
 */
function decode_record(record) {
    var encodedPayload = record.kinesis.data;
    var rawPayload = new Buffer(encodedPayload, 'base64').toString('ascii');
    var data = JSON.parse(rawPayload);
    return data
}

/**
 * Load graph from local
 *
 * @method load_from_local
 */
var load_from_local = function(waybill){
    var deferred = Q.defer()
    path = 'tests/'+waybill
    data = []

    fs.readFile(path, function(err, data){
        if (err){
            deferred.resolve([])
        } else {
            var data = JSON.parse(data.toString());
            deferred.resolve(data);
        }
    })
    return deferred.promise;
}

/**
 * Write graph to local
 *
 * @method store_to_local
 */
var store_to_local = function(waybill, data){
    var deferred = Q.defer()
    path = 'tests/' + waybill
    data.sort(compare)
    data.forEach( function(elem, index) {
        console.log(waybill + ' ' + elem['idx'] + ' ' +elem['st'] + ' ' +elem['src'] + ' ' +elem['dst'] + ' '+ elem['conn'] +' ' +elem['rmk'])
    });
    console.log()

    fs.writeFile(path, JSON.stringify(data), 'utf-8', function(){
        deferred.resolve(JSON.stringify(data))
    });
    return deferred.promise;
}

/**
 * store graph to S3
 *
 * @method store_to_s3
 */
var store_to_s3 = function(waybill, data){
    var deferred = Q.defer()

    data.sort(compare)
    data.forEach( function(elem, index) {
        console.log(waybill + ' ' + elem['idx'] + ' ' +elem['st'] + ' ' +elem['src'] + ' ' +elem['dst'] + ' '+ elem['conn'] +' ' +elem['rmk'])
    });
    console.log()

    var params = {
        Bucket: config.BUCKET,
        Key: waybill,
        Body: JSON.stringify(data)
    };

    s3.putObject(params, function(err, data) {
        if (err) {
            console.log(err)
            throw err
        }
        else {
            deferred.resolve(JSON.stringify(data))
        }
     });
    return deferred.promise;
}

/**
 * Load graph data from s3
 *
 * @method load_from_s3
 */
var load_from_s3 = function(waybill){
    var deferred = Q.defer()
    var params = {
        Bucket: config.BUCKET,
        Key: waybill,
    };

    s3.getObject(params, function(err, data) {
        if (err) {
            deferred.resolve([])
        } else {
            deferred.resolve(JSON.parse(data.Body.toString()))
        }
    });

    return deferred.promise;
}

/**
 * Returns True if all secondary conditions are met by obj else False
 *
 * @method match
 */
var match = function(obj, secondary){
    for (var key in secondary){
        if (obj[key] != secondary[key]){
            return false
        }
    }
    return true
}


/**
 * Convert center codes to user readeable format
 *
 * @method prettify
 */
var prettify = function(segments){
    var tsegments = []

    segments.forEach(function(segment){
        var tsegment = {}
        for (var key in segment){
            tsegment[key] = segment[key]
        }
        tsegment['src'] = INV_MAPPING[tsegment['src']] || null
        tsegment['dst'] = INV_MAPPING[tsegment['dst']] || null
        tsegments.push(tsegment)
    })

    return tsegments
}




if (config.STORAGE == 'production'){
    exports.load_from_source = load_from_s3;
    exports.store_to_source = store_to_s3;
} else {
    exports.load_from_source = load_from_local;
    exports.store_to_source = store_to_local;
}





exports.iso_to_seconds = iso_to_seconds;
exports.mod_path = mod_path;
exports.center_name_to_code = center_name_to_code;
exports.pretty = pretty;
exports.isReturnPackage = isReturnPackage;
exports.decode_record = decode_record;
exports.prettify = prettify;
exports.match = match;
exports.Class = Class;

