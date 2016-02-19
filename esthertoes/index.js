var AWS = require('aws-sdk');

var esDomain = {
    protocol: 'https',
    endpoint: '4eba48c5d2c17c66b52433b0a61dc227.us-east-1.aws.found.io',
    port: '9243',
    user: 'admin',
    password: 'juopmg3bltn323ao9m',
    index: 'beatles',
    doctype: 'segment'
};


function postDocumentToES(doc, context) {
    var endpoint = new AWS.Endpoint({
        hostname: esDomain.endpoint,
        port: esDomain.port,
        protocol: esDomain.protocol
    });

    var headers = {
        Authorization: 'Basic ' + base64(esDomain.user + ':' + esDomain.password)
    };

    var req = new AWS.HttpRequest({
        endpoint: endpoint,
        headers: headers,
        method: 'POST',
        path: '/' + esDomain.index + '/' + esDomain.doctype,
        body: doc
    });

    var send = new AWS.NodeHttpClient();

    send.handleRequest(req, null, function(httpResp) {
        var body = '';
        httpResp.on('data', function(chunk) {
            numDocsAdded++;
            console.log('All ' + numDocsAdded + ' records added to ES.');
            context.succeed();
        });
    }, function(err) {
        console.log('Error: ' + err);
        context.fail();
    });
}

function esthertoes(bucket, key, context) {
    var s3Stream = s3.getObject({Bucket: bucket, Key: key}).createReadStream();

    s3Stream.on('data', function(parsedEntry) {
        postDocumentToES(parsedEntry, context);
    });

    s3Stream.on('error', function() {
        console.log('Error getting object "' + key + '" from bucket "' + bucket + '".');
        context.fail();
    });
}

exports.handler = function(event, context) {
    event.Records.forEach( function( record) {
        var bucket = record.s3.bucket.name;
        var objKey = decodeURIComponent(record.s3.object.key.replace(/\+/g, ' '));
        esthertoes(bucket, objKey, context);
    });
}
