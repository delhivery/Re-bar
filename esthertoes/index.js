var aws = require('aws-sdk')
var s3 = new aws.S3()

var esDomain = {
  endpoint: 'b687fbbb4069aa3ce8a0e783e9c119fc.us-east-1.aws.found.io',
  index: 'sierra',
  docType: 'segment'
}

var endpoint = new aws.Endpoint(esDomain.endpoint)
endpoint.protocol = 'https:'

function postDocumentToES (segments, waybill, context) {
  var data = []
  waybill = waybill.split('.')[0]

  segments.forEach(function (segment) {
    if (segment.idx !== 0 && segment.idx !== '0') {
      segment['wbn'] = waybill
      var segmentIdx = waybill + '_' + segment.idx
      var upd_record = { 'update': {'_id': segmentIdx, '_type': esDomain.docType, '_index': esDomain.index, '_retry_on_conflict': 3} }
      var dat_record = { 'doc': segment, 'doc_as_upsert': true }
      data.push(JSON.stringify(upd_record))
      data.push(JSON.stringify(dat_record))
    }
  })

  var req = new aws.HttpRequest(endpoint)
  req.path = '/_bulk'
  req.body = data.join('\n') + '\n'
  req.headers['Host'] = endpoint.host
  req.headers['Authorization'] = 'Basic ' + new Buffer('lambda:8jdRgjs$sPfs^T<*').toString('base64')

  var send = new aws.NodeHttpClient()
  send.handleRequest(req, null, function (httpResp) {
    var body = ''

    httpResp.on('data', function (chunk) {
      body += chunk
    })

    httpResp.on('end', function (chunk) {
      body += chunk
      console.log('Added segments to ES', body)
      context.succeed()
    })
  }, function (err) {
    console.log('Error: ' + err)
    context.fail()
  })
}

exports.handler = function (event, context) {
  event.Records.forEach(function (record) {
    var bucket = record.s3.bucket.name
    var objKey = decodeURIComponent(record.s3.object.key.replace(/\+/g, ' '))

    s3.getObject({Bucket: bucket, Key: objKey}, function (err, data) {
      if (err) {
        console.log(err)
        context.fail()
      } else {
        var segments = JSON.parse(data.Body.toString('UTF-8'))
        postDocumentToES(segments, objKey, context)
      }
    })
  })
}
