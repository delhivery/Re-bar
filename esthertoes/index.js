var AWS = require('aws-sdk');
var LineStream = require('byline').LineStream;
var s3 = new AWS.S3();
var elasticsearch = require('elasticsearch');
var stream = require('stream');

var numDocsAdded = 0;

var esDomain = {
    endpoint: '4eba48c5d2c17c66b52433b0a61dc227.us-east-1.aws.found.io',
    port: 9243,
    user: 'admin',
    password: 'juopmg3bltn323ao9m',
    index: 'beatles',
    doctype: 'segment'
};

var client = new elasticsearch.Client({
    host: {
        protocol: 'https',
        host: esDomain.endpoint,
        port: esDomain.port,
        auth: esDomain.user + ':' + esDomain.password,
    }
})


function postDocumentToES(doc, wbn, context) {
    doc = JSON.parse(doc);
    data = [];

    doc.forEach( function( record) {
        record['wbn'] = wbn;
        data.push({update: {_index: esDomain.index, _type: esDomain.doctype, _id: wbn + record.idx}});
        data.push({doc: record, doc_as_upsert: true});
    });

    client.bulk({
        body: data
    }, function(error, response) {
        if (error !== undefined) {
            console.log('Error: ', error);
        }
        else if (response.errors != false) {
            console.log('Error: ', JSON.stringify(response));
        }
        else {
            context.succeed()
        }
    });
}

function esthertoes(bucket, key, context, lineStream, recordStream) {
    var s3Stream = s3.getObject({Bucket: bucket, Key: key}).createReadStream();

    s3Stream
        .pipe(lineStream)
        .pipe(recordStream)
        .on('data', function(parsedEntry) {
            postDocumentToES(parsedEntry, key, context);
        });

    s3Stream.on('error', function() {
        console.log('Error getting object "' + key + '" from bucket "' + bucket + '".');
        context.fail();
    });
}

exports.handler = function(event, context) {
    var lineStream = new LineStream();
    var recordStream = new stream.Transform({objectMode: true})

    recordStream._transform = function(line, encoding, done) {
        var serializedRecord = line.toString();
        this.push(serializedRecord);
        done();
    }

    event.Records.forEach( function( record) {
        var bucket = record.s3.bucket.name;
        var objKey = decodeURIComponent(record.s3.object.key.replace(/\+/g, ' '));
        esthertoes(bucket, objKey, context, lineStream, recordStream);
    });
}
